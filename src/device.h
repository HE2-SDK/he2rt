#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include "physical-device.h"
#include "acceleration-structures.h"
#include "allocator.h"

namespace he2rt {
	class Device {
    public:
        struct Queues {
            vk::raii::Queue graphics;
            vk::raii::Queue present;
        };

        PhysicalDevice& physicalDevice;
        Queues queues;
        Allocator allocator;

    public:
        vk::raii::Device device;

    private:
        vk::raii::CommandPool tempCommands;

    public:
        Device(PhysicalDevice& physicalDevice, vk::raii::Device&& device);

        Swapchain createSwapchain(const vk::Extent2D& desiredExtent, vk::SurfaceKHR surface);
        AccelerationStructure createAS(const vk::AccelerationStructureBuildSizesInfoKHR& buildSizes);
        AccelerationStructure buildAS(vk::AccelerationStructureTypeKHR type, const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo);
        AccelerationStructure buildBLAS(const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo);
        AccelerationStructure buildTLAS(const vk::AccelerationStructureGeometryKHR& geometry, const vk::AccelerationStructureBuildRangeInfoKHR& rangeInfo);
        vk::raii::Semaphore createSemaphore(uint64_t initialValue = 0);

        template<typename F>
        void runOneTimeCommandBuffer(F f) {
            auto cmdBuffers = device.allocateCommandBuffers({ tempCommands, vk::CommandBufferLevel::ePrimary, 1 });
            auto& cmdBuffer = cmdBuffers[0];

            cmdBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
            f(cmdBuffer);
            cmdBuffer.end();

            auto fence = device.createFence({});

            std::array<vk::CommandBufferSubmitInfo, 1> submitInfos{ vk::CommandBuffer{ cmdBuffer } };

            queues.graphics.submit2({ vk::SubmitInfo2{ {}, {}, submitInfos } }, fence);
            device.waitForFences({ fence }, VK_TRUE, UINT64_MAX);
        }
	};
}
