#include "device.h"
#include "instance.h"
#include "util.h"
#include <array>

namespace he2rt {
    Device::Device(PhysicalDevice& physicalDevice, vk::raii::Device&& device)
        : physicalDevice{ physicalDevice }
        , queues{ device.getQueue(physicalDevice.queueIndices.graphics, 0), device.getQueue(physicalDevice.queueIndices.present, 0) }
        , allocator{ std::move(vma::raii::Allocator{ physicalDevice.instance.instance, device, vma::AllocatorCreateInfo{ {}, physicalDevice.physicalDevice } }) }
        , device { std::move(device) }
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
            vk::ImageUsageFlagBits::eColorAttachment,
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

    AccelerationStructure Device::createAS(const vk::AccelerationStructureBuildSizesInfoKHR& buildSizes) {
        auto buffer = allocator.createBuffer(
            buildSizes.accelerationStructureSize,
            0,
            vk::BufferUsageFlagBits2::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits2::eShaderDeviceAddress,
            vma::MemoryUsage::eAutoPreferDevice
        );

        auto structure = device.createAccelerationStructureKHR({ {}, buffer.buffer, {}, buildSizes.accelerationStructureSize, vk::AccelerationStructureTypeKHR::eBottomLevel });
        auto deviceAddress = device.getAccelerationStructureAddressKHR({ vk::AccelerationStructureKHR{ structure } });

        return { std::move(buffer), std::move(structure), deviceAddress };
    }

    AccelerationStructure Device::buildAS(vk::AccelerationStructureTypeKHR type, const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo) {
        //TODO: fetch
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR properties{};

        vk::AccelerationStructureBuildGeometryInfoKHR geometryInfo{
            type,
            vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            vk::BuildAccelerationStructureModeKHR::eBuild,
            {},
            {},
            { geometry },
        };

        auto buildSizes = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, geometryInfo, { rangeInfo.primitiveCount });

        auto scratchBuffer = allocator.createBuffer(
            util::align(buildSizes.buildScratchSize, properties.minAccelerationStructureScratchOffsetAlignment),
            properties.minAccelerationStructureScratchOffsetAlignment,
            vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eShaderDeviceAddress | vk::BufferUsageFlagBits2::eAccelerationStructureStorageKHR,
            vma::MemoryUsage::eAuto
        );

        auto as = createAS(buildSizes);

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
