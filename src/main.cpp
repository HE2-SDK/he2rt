#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>
#include <set>
#include "instance.h"
#include "scene.h"
#include "raytracing-engine.h"
#include "post-processing/tonemapper/tonemapper.h"
#include "gpu-crash-tracker.h"
#include "common_dpn.slang.spv.h"
#include <filesystem>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

using namespace he2rt;

class GBuffer {
public:
    Image image;
    vk::raii::ImageView imageView;
    vk::ImageMemoryBarrier2 barrier;

    GBuffer(Device& device, Swapchain& swapchain, vk::Format format)
        : image{ device.createImage({
            {},
            vk::ImageType::e2D,
            format,
            { swapchain.description.extent.width, swapchain.description.extent.height, 1 },
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive,
            {},
            vk::ImageLayout::eUndefined,
        }) }
        , imageView{ device.device.createImageView({
            {},
            image.image,
            vk::ImageViewType::e2D,
            format,
            { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
            { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
        }) }
    {}
};

vk::raii::SurfaceKHR createSurface(Instance& instance, GLFWwindow * window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance.instance, window, NULL, &surface) != VK_SUCCESS)
        throw std::runtime_error{ "can't create surface" };

    return { instance.instance, surface };
}

Eigen::Vector3f camPos{ 0.0f, 0.0f, 3.0f };
Eigen::Vector3f camTgt{ 0.0f, 0.0f, 0.0f };
float yaw{};
float pitch{};
Eigen::Quaternionf camRot{ Eigen::Quaternionf::Identity() };
bool done{};
float lastX = 1920 / 2, lastY = 1080 / 2;
float sens = 0.005f;
float moveSensSlow = 0.001f;
float moveSens = 1.0f;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!done) {
        lastX = xpos;
        lastY = ypos;
        done = true;
    }
    float deltaX = xpos - lastX;
    float deltaY = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    deltaX *= sens;
    deltaY *= sens;

    yaw += deltaX;
    pitch -= deltaY;

    //camRot = Eigen::AngleAxisf{ deltaY, Eigen::Vector3f::UnitX() } * Eigen::AngleAxisf{ deltaX, Eigen::Vector3f::UnitY() } * camRot;
    //camPos.x() += deltaX;
    //camPos.y() -= deltaY;
    //camTgt.x() += deltaX;
    //camTgt.y() -= deltaY;
    // viewMatrix = lookatMatrix(camPos, camTgt, Eigen::Vector3f::UnitY());
    camRot = Eigen::AngleAxisf{ yaw, Eigen::Vector3f::UnitY() } * Eigen::AngleAxisf{ pitch, Eigen::Vector3f::UnitX() };

    //viewMatrix.rotate(Eigen::AngleAxisf{ deltaY, Eigen::Vector3f::UnitX() } * Eigen::AngleAxisf{ deltaX, Eigen::Vector3f::UnitY() });
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
}

int main(void) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(1920, 1080, "HE2 RT", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    {
        vk::raii::Context context{};
        Instance instance{ context };

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Instance>(instance.instance));

        vk::Extent2D desiredExtent{ (uint32_t)width, (uint32_t)height };

        auto surface = createSurface(instance, window);
        auto maybePhysicalDevice = instance.findPhysicalDevice(desiredExtent, surface);

        if (!maybePhysicalDevice.has_value())
            throw std::runtime_error{ "can't find suitable device" };

        GpuCrashTracker gpuCrashTracker{};

        auto physicalDevice = std::move(*maybePhysicalDevice);
        auto device = physicalDevice.createDevice();

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Device>(device.device));

        auto swapchain = device.createSwapchain(desiredExtent, surface);
        Scene scene{ device };

        std::cout << "swap chain contains " << swapchain.images.size() << " images." << std::endl;

        scene.addMaterialShader("Common_dpn", common_dpn_shader, common_dpn_shader_sizeInBytes, { { 12, {
            { "diffuseTextureId", 0 },
            { "specularTextureId", 4 },
            { "normalTextureId", 8 },
        } } });

        //scene.addTexture("chr_sonic_cloth_abd", "chr_sonic_cloth_abd.dds");
        //scene.addTexture("chr_sonic_cloth_prm", "chr_sonic_cloth_prm.dds");
        //scene.addTexture("chr_sonic_cloth_nrm", "chr_sonic_cloth_nrm.dds");
        //scene.addTexture("chr_sonic_fur_abd", "chr_sonic_fur_abd.dds");
        //scene.addTexture("chr_sonic_fur_prm", "chr_sonic_fur_prm.dds");
        //scene.addTexture("chr_sonic_fur_nrm", "chr_sonic_fur_nrm.dds");
        //scene.addTexture("chr_sonic_fur_fal", "chr_sonic_fur_fal.dds");
        //scene.addTexture("chr_sonic_fur_flw", "chr_sonic_fur_flw.dds");
        //scene.addTexture("chr_sonic_fur_fur", "chr_sonic_fur_fur.dds");
        //scene.addTexture("chr_sonic_skin_abd", "chr_sonic_skin_abd.dds");
        //scene.addTexture("chr_sonic_skin_prm", "chr_sonic_skin_prm.dds");
        //scene.addTexture("chr_sonic_skin_nrm", "chr_sonic_skin_nrm.dds");
        //scene.addTexture("chr_sonic_skin_fal", "chr_sonic_skin_fal.dds");
        //scene.addTexture("chr_sonic_eye_abd", "chr_sonic_eye_abd.dds");
        //scene.addTexture("chr_sonic_eye_prm", "chr_sonic_eye_prm.dds");
        //scene.addTexture("chr_sonic_eye_nra", "chr_sonic_eye_nra.dds");
        //scene.addTexture("chr_sonic_eye_spt", "chr_sonic_eye_spt.dds");
        //scene.addMaterial("chr_sonic_fur", "chr_sonic_fur.material");
        //scene.addMaterial("chr_sonic_cloth", "chr_sonic_cloth.material");
        //scene.addMaterial("chr_sonic_skin", "chr_sonic_skin.material");
        //scene.addMaterial("chr_sonic_mouth", "chr_sonic_mouth.material");
        //scene.addMaterial("chr_sonic_eyeL", "chr_sonic_eyeL.material");
        //scene.addMaterial("chr_sonic_eyeR", "chr_sonic_eyeR.material");
        //scene.addModel("chr_sonic.model");
        for (auto& f : std::filesystem::directory_iterator("./scene")) {
            auto fpath = f.path();

            if (fpath.extension() == ".dds") {
                scene.addTexture(fpath.stem().generic_string(), fpath.generic_string());
            }
        }

        for (auto& f : std::filesystem::directory_iterator("./scene")) {
            auto fpath = f.path();

            if (fpath.extension() == ".material") {
                scene.addMaterial(fpath.stem().generic_string(), fpath.generic_string());
            }
        }

        for (auto& f : std::filesystem::directory_iterator("./scene")) {
            auto fpath = f.path();

            if (fpath.extension() == ".model") {
                scene.addModel(fpath.stem().generic_string(), fpath.generic_string());
            }
        }

        for (auto& f : std::filesystem::directory_iterator("./scene")) {
            auto fpath = f.path();

            if (fpath.extension() == ".terrain-model") {
                scene.addTerrainModel(fpath.stem().generic_string(), fpath.generic_string());
            }
        }

        for (auto& f : std::filesystem::directory_iterator("./scene")) {
            auto fpath = f.path();

            if (fpath.extension() == ".pcmodel") {
                scene.addModelInstances(fpath.generic_string());
            }
        }

        scene.addModelInstance("chr_sonic", Eigen::Affine3f{ Eigen::Translation3f{ 0.0f, 0.0f, 0.0f } });

        scene.finalize();

        auto commandPool = device.device.createCommandPool({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, physicalDevice.queueIndices.graphics });

        constexpr size_t inFlightFrameCount = 2;
        size_t currentFrame{};

        std::vector<vk::raii::Fence> frameFences{};
        std::vector<vk::raii::Semaphore> acquireSemaphores{};
        std::vector<vk::raii::CommandBuffer> commandBuffers{};

        GBuffer gbuffer{ device, swapchain, vk::Format::eR32G32B32A32Sfloat };
        GBuffer tonemappedGbuffer{ device, swapchain, vk::Format::eB8G8R8A8Unorm };
        Sampler gbufferSampler{ device.createSampler({}) };
        vk::ImageSubresourceRange colorRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        device.runOneTimeCommandBuffer([&](vk::CommandBuffer& cmd) {
            std::array<GBuffer*, 2> gbuffers{ &gbuffer, &tonemappedGbuffer };

            size_t i{};
            for (auto* gbuf : gbuffers) {
                std::array<vk::ImageMemoryBarrier2, 1> imageBarrierBeforeClear{ {
                    {
                        vk::PipelineStageFlagBits2::eNone,
                        vk::AccessFlagBits2::eNone,
                        vk::PipelineStageFlagBits2::eTransfer,
                        vk::AccessFlagBits2::eTransferWrite,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::QueueFamilyIgnored,
                        vk::QueueFamilyIgnored,
                        gbuf->image.image,
                        colorRange,
                    },
                } };
                cmd.pipelineBarrier2({ {}, {}, {}, imageBarrierBeforeClear });
                cmd.clearColorImage(gbuf->image.image, vk::ImageLayout::eTransferDstOptimal, i++ == 0 ? vk::ClearColorValue{ 0.0f, 1.0f, 0.0f, 1.0f } : vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f }, colorRange);

                std::array<vk::ImageMemoryBarrier2, 1> imageBarrierAfterClear{ {
                    {
                        vk::PipelineStageFlagBits2::eTransfer,
                        vk::AccessFlagBits2::eTransferWrite,
                        vk::PipelineStageFlagBits2::eAllGraphics | vk::PipelineStageFlagBits2::eAllCommands,
                        vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eGeneral,
                        vk::QueueFamilyIgnored,
                        vk::QueueFamilyIgnored,
                        gbuf->image.image,
                        colorRange,
                    },
                } };
                cmd.pipelineBarrier2({ {}, {}, {}, imageBarrierAfterClear });
            }
        });

        for (size_t i = 0; i < inFlightFrameCount; i++) {
            frameFences.emplace_back(device.device.createFence({ vk::FenceCreateFlagBits::eSignaled }));
            acquireSemaphores.emplace_back(device.device.createSemaphore({}));
            commandBuffers.emplace_back(std::move(device.device.allocateCommandBuffers({ commandPool, vk::CommandBufferLevel::ePrimary, 1 })[0]));
        }

        RaytracingEngine engine{ device, scene };
        shaders::tonemapper::Tonemapper tonemapper{ device };

        float4x4 projMat{ projectionMatrix(60.0f, static_cast<float>(swapchain.description.extent.width) / static_cast<float>(swapchain.description.extent.height), 0.1f, 60000.0f) };
        engine.globalPushConstant.projInverse = projMat.inverse();
        engine.globalPushConstant.materials = (shaders::Material*)scene.materialBuffer.value().deviceAddress;
        engine.globalPushConstant.models = (shaders::Model*)scene.modelBuffer.value().deviceAddress;

        glfwMakeContextCurrent(window);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetKeyCallback(window, keyCallback);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * -Eigen::Vector3f::UnitZ();
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * Eigen::Vector3f::UnitZ();
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * Eigen::Vector3f::UnitX();
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * -Eigen::Vector3f::UnitX();

            auto frameFence = vk::Fence{ frameFences[currentFrame] };
            auto acquireSemaphore = vk::Semaphore{ acquireSemaphores[currentFrame] };
            auto commandBuffer = vk::CommandBuffer{ commandBuffers[currentFrame] };

            device.device.waitForFences({ frameFence }, vk::True, UINT64_MAX);
            device.device.resetFences({ frameFence });

            auto imageIndex = swapchain.swapchain.acquireNextImage(UINT64_MAX, acquireSemaphore).value;

            auto submitSemaphore = vk::Semaphore{ swapchain.imageReadySemaphores[imageIndex] };

            engine.globalPushConstant.frame = imageIndex;

            commandBuffer.reset();
            commandBuffer.begin({ vk::CommandBufferUsageFlags{} });

            vk::Sampler sampler{ gbufferSampler.sampler };
            vk::ImageView gbufferImageView{ gbuffer.imageView };
            vk::ImageView tonemappedGbufferImageView{ tonemappedGbuffer.imageView };
            engine.raytrace(commandBuffer, gbufferImageView, swapchain.description.extent, camRot.inverse() * Eigen::Translation3f{ camPos }.inverse());

            tonemapper.run(commandBuffer, swapchain.description.extent, { sampler, gbufferImageView, vk::ImageLayout::eGeneral }, { sampler, tonemappedGbufferImageView, vk::ImageLayout::eGeneral });
            std::array<vk::MemoryBarrier2, 1> memoryBarriers{ {
                vk::MemoryBarrier2{ vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead },
            } };
            std::array<vk::ImageMemoryBarrier2, 1> imageBarrierBeforeSwapchainCopy{ {
                {
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::ImageLayout::eGeneral,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    swapchain.images[imageIndex],
                    colorRange,
                },
            } };
            commandBuffer.pipelineBarrier2({ {}, memoryBarriers, {}, imageBarrierBeforeSwapchainCopy });
            vk::ImageCopy2 copyRegion{
                { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                { 0, 0, 0 },
                { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                { 0, 0, 0 },
                { swapchain.description.extent.width, swapchain.description.extent.height, 1 },
            };
            commandBuffer.copyImage2({ tonemappedGbuffer.image.image, vk::ImageLayout::eGeneral, swapchain.images[imageIndex], vk::ImageLayout::eGeneral, { copyRegion } });
            std::array<vk::ImageMemoryBarrier2, 1> imageBarrierAfterSwapchainCopy{ {
                {
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite,
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eNone,
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::QueueFamilyIgnored,
                    vk::QueueFamilyIgnored,
                    swapchain.images[imageIndex],
                    colorRange,
                },
            } };
            commandBuffer.pipelineBarrier2({ {}, {}, {}, imageBarrierAfterSwapchainCopy });

            commandBuffer.end();

            vk::SemaphoreSubmitInfo acquireSemaphoreSubmitInfo{ acquireSemaphore, 0, vk::PipelineStageFlagBits2::eColorAttachmentOutput };
            vk::CommandBufferSubmitInfo commandBufferSubmitInfo{ commandBuffer };
            vk::SemaphoreSubmitInfo signalSemaphoreSubmitInfo{ submitSemaphore, 0, vk::PipelineStageFlagBits2::eBottomOfPipe };

            device.queues.graphics.submit2({ { {}, { acquireSemaphoreSubmitInfo }, { commandBufferSubmitInfo }, { signalSemaphoreSubmitInfo } } }, frameFence);

            auto swp = vk::SwapchainKHR{ swapchain.swapchain };
            device.queues.present.presentKHR({ { submitSemaphore }, { swp }, { imageIndex } });

            currentFrame = (currentFrame + 1) % inFlightFrameCount;
        }

        device.device.waitIdle();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}