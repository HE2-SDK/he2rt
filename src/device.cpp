#include "device.h"
#include "instance.h"
#include "util.h"
#include <array>

namespace he2rt {
    Device::Device(PhysicalDevice& physicalDevice, vk::raii::Device&& device)
        : physicalDevice{ physicalDevice }
        , queues{ device.getQueue(physicalDevice.queueIndices.graphics, 0), device.getQueue(physicalDevice.queueIndices.present, 0) }
        , device { std::move(device) }
        , allocator{ physicalDevice.instance.instance, this->device, vma::AllocatorCreateInfo{ vma::AllocatorCreateFlagBits::eBufferDeviceAddress | vma::AllocatorCreateFlagBits::eKhrMaintenance4 | vma::AllocatorCreateFlagBits::eKhrMaintenance5, physicalDevice.physicalDevice, {}, {}, nullptr, nullptr, nullptr, nullptr, {}, VK_API_VERSION_1_4 } }
        , tempCommands{ this->device.createCommandPool({ {}, physicalDevice.queueIndices.graphics }) }
    {
	}

    Swapchain Device::createSwapchain(const vk::Extent2D& desiredExtent, vk::SurfaceKHR surface) {
        auto maybeDesc = physicalDevice.findSwapchain(desiredExtent, surface);

        if (!maybeDesc.has_value())
            throw std::runtime_error{ "can't find suitable swapchain" };
        
        auto description = *maybeDesc;

        unsigned int desiredImageCount = description.capabilities.minImageCount + 1;

        if (description.capabilities.minImageCount != 0)
            desiredImageCount = std::max(desiredImageCount, description.capabilities.maxImageCount);

        auto sameQueue = physicalDevice.queueIndices.graphics != physicalDevice.queueIndices.present;
        std::vector<unsigned int> queueIndices{ physicalDevice.queueIndices.graphics, physicalDevice.queueIndices.present };

        return { *this, device.createSwapchainKHR({
            {},
            surface,
            desiredImageCount,
            description.surfaceFormat.format,
            description.surfaceFormat.colorSpace,
            description.extent,
            1,
            vk::ImageUsageFlagBits::eTransferDst, //vk::ImageUsageFlagBits::eColorAttachment,
            sameQueue ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
            sameQueue ? (uint32_t)queueIndices.size() : 0,
            sameQueue ? queueIndices.data() : nullptr,
            description.capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            description.presentMode,
            vk::True,
            {},
        }), *maybeDesc };
    }

    Buffer Device::createBuffer(vk::DeviceSize size, vk::DeviceSize alignment, vk::BufferUsageFlags2 bufferUsage, vma::MemoryUsage memoryUsage, vma::AllocationCreateFlags allocationCreateFlags, vk::MemoryPropertyFlags memoryPropertyRequiredFlags) {
        vk::BufferCreateInfo createInfo{ {}, size, {}, vk::SharingMode::eExclusive };
        vk::BufferUsageFlags2CreateInfo usageCreateInfo{ bufferUsage | vk::BufferUsageFlagBits2::eShaderDeviceAddress };

        auto createInfoChain = vk::StructureChain{ createInfo, usageCreateInfo };

        return createBuffer(createInfoChain.get<vk::BufferCreateInfo>(), { allocationCreateFlags, memoryUsage, memoryPropertyRequiredFlags }, alignment);
    }

    Buffer Device::createBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const vma::AllocationCreateInfo& allocationCreateInfo, vk::DeviceSize alignment) {
        vma::AllocationInfo allocInfo{};

        auto buffer = allocator.createBufferWithAlignment(bufferCreateInfo, allocationCreateInfo, alignment, allocInfo);
        auto addr = device.getBufferAddress({ vk::Buffer{ buffer } });

        return { std::move(buffer), bufferCreateInfo.size, addr, allocInfo.pMappedData };
    }

    Image Device::createImage(const vk::ImageCreateInfo& imageCreateInfo, vma::MemoryUsage memoryUsage, vma::AllocationCreateFlags allocationCreateFlags, vk::MemoryPropertyFlags memoryPropertyRequiredFlags) {
        return createImage(imageCreateInfo, { allocationCreateFlags, memoryUsage, memoryPropertyRequiredFlags });
    }

    Image Device::createImage(const vk::ImageCreateInfo& imageCreateInfo, const vma::AllocationCreateInfo& allocationCreateInfo) {
        vma::AllocationInfo allocInfo{};

        auto image = allocator.createImage(imageCreateInfo, allocationCreateInfo, allocInfo);

        return { std::move(image), imageCreateInfo };
    }

    Sampler Device::createSampler(const vk::SamplerCreateInfo& samplerCreateInfo) {
        return { device.createSampler(samplerCreateInfo), samplerCreateInfo };
    }

    AccelerationStructure Device::createAS(vk::AccelerationStructureTypeKHR type, const vk::AccelerationStructureBuildSizesInfoKHR& buildSizes) {
        auto buffer = createBuffer(
            buildSizes.accelerationStructureSize,
            1,
            vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eAccelerationStructureStorageKHR,
            vma::MemoryUsage::eAutoPreferDevice
        );

        auto structure = device.createAccelerationStructureKHR({ {}, buffer.buffer, {}, buildSizes.accelerationStructureSize, type });
        auto deviceAddress = device.getAccelerationStructureAddressKHR({ vk::AccelerationStructureKHR{ structure } });

        return { std::move(buffer), std::move(structure), deviceAddress };
    }

    AccelerationStructure Device::buildAS(vk::AccelerationStructureTypeKHR type, const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo) {
        vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo{
            type,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            {},
            {},
            { geometry },
        };

        auto buildSizes = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, geometryInfo, { rangeInfo.primitiveCount });

        auto scratchBuffer = createBuffer(
            util::align(buildSizes.buildScratchSize, physicalDevice.accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment),
            physicalDevice.accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment,
            vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eAccelerationStructureStorageKHR
        );

        auto as = createAS(type, buildSizes);

        geometryInfo.dstAccelerationStructure = as.structure;
        geometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        runOneTimeCommandBuffer([&](auto& cmd) {
            cmd.buildAccelerationStructuresKHR({ geometryInfo }, { &rangeInfo });
        });

        return as;
    }

    AccelerationStructure Device::buildBLAS(const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo) {
        return buildAS(vk::AccelerationStructureTypeKHR::eBottomLevel, geometry, rangeInfo);
    }

    AccelerationStructure Device::buildTLAS(const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo) {
        return buildAS(vk::AccelerationStructureTypeKHR::eTopLevel, geometry, rangeInfo);
    }

    vk::raii::Semaphore Device::createSemaphore(uint64_t initialValue) {
        vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> createInfo{ { }, { vk::SemaphoreType::eTimeline, initialValue } };

        return device.createSemaphore(createInfo.get());
    }
}
