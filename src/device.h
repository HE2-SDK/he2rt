#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#define VMA_DEBUG_LOG_FORMAT(format, ...) do { \
       printf((format), __VA_ARGS__); \
       printf("\n"); \
   } while(false)
#include <vk_mem_alloc.hpp>
#include <vk_mem_alloc_raii.hpp>
#include <optional>
#include "physical-device.h"
#include "acceleration-structures.h"
#include "buffer.h"
#include "image.h"
#include "sampler.h"

namespace he2rt {
	class Device {
    public:
        struct Queues {
            vk::raii::Queue graphics;
            vk::raii::Queue present;
        };

        PhysicalDevice& physicalDevice;
        Queues queues;

    public:
        vk::raii::Device device;

    private:
        vma::raii::Allocator allocator;
        vk::raii::CommandPool tempCommands;

    public:
        Device(PhysicalDevice& physicalDevice, vk::raii::Device&& device);

        Swapchain createSwapchain(const vk::Extent2D& desiredExtent, vk::SurfaceKHR surface);
        template<typename T> inline Buffer createBuffer(vk::BufferUsageFlags2 bufferUsage, vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto, vma::AllocationCreateFlags allocationCreateFlags = {}, vk::MemoryPropertyFlags memoryPropertyRequiredFlags = {}) {
            return createBuffer(sizeof(T), alignof(T), bufferUsage, memoryUsage, allocationCreateFlags, memoryPropertyRequiredFlags);
        }
        Buffer createBuffer(vk::DeviceSize size, vk::DeviceSize alignment, vk::BufferUsageFlags2 bufferUsage, vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto, vma::AllocationCreateFlags allocationCreateFlags = {}, vk::MemoryPropertyFlags memoryPropertyRequiredFlags = {});
        Buffer createBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const vma::AllocationCreateInfo& allocationCreateInfo, vk::DeviceSize alignment);
        Image createImage(const vk::ImageCreateInfo& imageCreateInfo, vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto, vma::AllocationCreateFlags allocationCreateFlags = {}, vk::MemoryPropertyFlags memoryPropertyRequiredFlags = {});
        Image createImage(const vk::ImageCreateInfo& imageCreateInfo, const vma::AllocationCreateInfo& allocationCreateInfo);
        Sampler createSampler(const vk::SamplerCreateInfo& samplerCreateInfo);
        AccelerationStructure createAS(vk::AccelerationStructureTypeKHR type, const vk::AccelerationStructureBuildSizesInfoKHR& buildSizes);
        AccelerationStructure buildAS(vk::AccelerationStructureTypeKHR type, const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo);
        AccelerationStructure buildBLAS(const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo);
        AccelerationStructure buildTLAS(const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo);
        vk::raii::Semaphore createSemaphore(uint64_t initialValue = 0);

        template<typename F>
        void runOneTimeCommandBuffer(F f) {
            auto cmdBuffers = device.allocateCommandBuffers({ tempCommands, vk::CommandBufferLevel::ePrimary, 1 });
            vk::CommandBuffer cmdBuffer{ cmdBuffers[0] };

            cmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
            f(cmdBuffer);
            cmdBuffer.end();

            auto fence = device.createFence({});

            std::array<vk::CommandBufferSubmitInfo, 1> submitInfos{ cmdBuffer };

            queues.graphics.submit2({ vk::SubmitInfo2{ {}, {}, submitInfos } }, fence);
            device.waitForFences({ fence }, VK_TRUE, UINT64_MAX);
        }
	};
}
