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
#include "Common_d.slang.spv.h"
#include "Common_dn.slang.spv.h"
#include "Common_dp.slang.spv.h"
#include "Common_dpn.slang.spv.h"
#include "Emission_dpnE.slang.spv.h"
#include "EmissionTone_dpnE.slang.spv.h"
#include "EmissionTone_dapnE.slang.spv.h"
#include "Indirect_daon.slang.spv.h"
#include "Iridescence_ddpNrEa.slang.spv.h"
#include "IridescenceStone_ddpNrEa.slang.spv.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

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
float moveSensSlow = 0.025f;
float moveSens = 5.0f;
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

        scene.addMaterialShader("Common_d", Common_d_shader, Common_d_shader_sizeInBytes, { { 20, {
            { "PBRFactor", 0 },
            { "diffuseTextureId", 16 },
        } } });
        scene.addMaterialShader("Common_dn", Common_dn_shader, Common_dn_shader_sizeInBytes, { { 24, {
            { "PBRFactor", 0 },
            { "diffuseTextureId", 16 },
            { "normalTextureId", 20 },
        } } });
        scene.addMaterialShader("Common_dp", Common_dp_shader, Common_dp_shader_sizeInBytes, { { 8, {
            { "diffuseTextureId", 0 },
            { "specularTextureId", 4 },
        } } });
        scene.addMaterialShader("Common_dpn", Common_dpn_shader, Common_dpn_shader_sizeInBytes, { { 12, {
            { "diffuseTextureId", 0 },
            { "specularTextureId", 4 },
            { "normalTextureId", 8 },
        } } });
        scene.addMaterialShader("Emission_dpnE", Emission_dpnE_shader, Emission_dpnE_shader_sizeInBytes, { { 32, {
            { "Luminance", 0 },
            { "diffuseTextureId", 16 },
            { "specularTextureId", 20 },
            { "normalTextureId", 24 },
            { "emissionTextureId", 28 },
        } } });
        scene.addMaterialShader("EmissionTone_dpnE", EmissionTone_dpnE_shader, EmissionTone_dpnE_shader_sizeInBytes, { { 36, {
            { "Luminance", 0 },
            { "diffuseTextureId", 16 },
            { "specularTextureId", 20 },
            { "normalTextureId", 24 },
            { "emissionTextureId", 28 },
            { "s_LuminanceTextureId", 32 },
        } } });
        scene.addMaterialShader("EmissionTone_dapnE", EmissionTone_dapnE_shader, EmissionTone_dapnE_shader_sizeInBytes, { { 36, {
            { "Luminance", 0 },
            { "diffuseTextureId", 16 },
            { "specularTextureId", 20 },
            { "normalTextureId", 24 },
            { "emissionTextureId", 28 },
            { "s_LuminanceTextureId", 32 },
        } } });
        scene.addMaterialShader("Indirect_daon", Indirect_daon_shader, Indirect_daon_shader_sizeInBytes, { { 48, {
            { "PBRFactor", 0 },
            { "Offset", 16 },
            { "diffuseTextureId", 32 },
            { "normalTextureId", 36 },
            { "transparencyTextureId", 40 },
            { "reflectionTextureId", 44 },
        } } });
        scene.addMaterialShader("Iridescence_ddpNrEa", Iridescence_ddpNrEa_shader, Iridescence_ddpNrEa_shader_sizeInBytes, { { 96, {
            { "FalloffAndLuminance", 0 },
            { "IridescenceOffset", 16 },
            { "IridescenceMask", 32 },
            { "WPBRFactor", 48 },
            { "diffuseTextureId", 64 },
            { "diffuse1TextureId", 68 },
            { "specularTextureId", 72 },
            { "normalTextureId", 76 },
            { "reflectionTextureId", 80 },
            { "emissionTextureId", 84 },
            { "transparencyTextureId", 88 },
            { "s_LuminanceTextureId", 92 },
        } } });
        scene.addMaterialShader("IridescenceStone_ddpNrEa", IridescenceStone_ddpNrEa_shader, IridescenceStone_ddpNrEa_shader_sizeInBytes, { { 76, {
            { "FalloffAndLuminance", 0 },
            { "IridescenceOffset", 16 },
            { "WPBRFactor", 32 },
            { "diffuseTextureId", 48 },
            { "diffuse1TextureId", 52 },
            { "specularTextureId", 56 },
            { "normalTextureId", 60 },
            { "reflectionTextureId", 64 },
            { "emissionTextureId", 68 },
            { "transparencyTextureId", 72 },
        } } });

        for (auto& f : std::filesystem::directory_iterator("./scene")) {
            auto fpath = f.path();

            if (fpath.extension() == ".ntsp") {
                scene.addTextureStreamingPackage(fpath.stem().generic_string(), fpath.generic_string());
            }
        }

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

        //scene.addModelInstance("boss_dragon01", Eigen::Affine3f{ Eigen::Translation3f{ 0.0f, 0.0f, 0.0f } });
        //scene.addModelInstance("chr_sonic", Eigen::Affine3f{ Eigen::Translation3f{ 0.0f, 0.0f, 0.0f } });
        //scene.addModelInstance("cmnisl_obj_airwall02_A", Eigen::Affine3f{ Eigen::Translation3f{ 1.5f, 1.0f, -1.0f } * Eigen::AngleAxisf{ -EIGEN_PI / 7, Eigen::Vector3f::UnitY() } });

        scene.finalize();

        auto commandPool = device.device.createCommandPool({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, physicalDevice.queueIndices.graphics });

        constexpr size_t inFlightFrameCount = 2;
        size_t currentFrame{};

        std::vector<vk::raii::Fence> frameFences{};
        std::vector<vk::raii::Semaphore> acquireSemaphores{};
        std::vector<vk::raii::CommandBuffer> commandBuffers{};

        GBuffer gbuffer{ device, swapchain, vk::Format::eR32G32B32A32Sfloat };
        GBuffer tonemappedGbuffer{ device, swapchain, vk::Format::eA2B10G10R10UnormPack32 };
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

        std::array<vk::Format, 1> colorAttachmentFormats{ { swapchain.description.surfaceFormat.format } };

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = vk::Instance{ instance.instance };
        init_info.PhysicalDevice = vk::PhysicalDevice{ physicalDevice.physicalDevice };
        init_info.Device = vk::Device{ device.device };
        init_info.QueueFamily = device.physicalDevice.queueIndices.graphics;
        init_info.Queue = vk::Queue{ device.queues.graphics };
        init_info.DescriptorPoolSize = 32;
        init_info.MinImageCount = 2;
        init_info.ImageCount = 2;
        init_info.UseDynamicRendering = true;
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{ {}, colorAttachmentFormats };
        //init_info.PipelineInfoMain.RenderPass = wd->RenderPass;
        //init_info.PipelineInfoMain.Subpass = 0;
        //init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        //init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info);

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

        std::chrono::high_resolution_clock clock{};
        std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> frameStarts(inFlightFrameCount);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            auto frameFence = vk::Fence{ frameFences[currentFrame] };
            auto acquireSemaphore = vk::Semaphore{ acquireSemaphores[currentFrame] };
            auto commandBuffer = vk::CommandBuffer{ commandBuffers[currentFrame] };

            device.device.waitForFences({ frameFence }, vk::True, UINT64_MAX);
            device.device.resetFences({ frameFence });

            auto imageIndex = swapchain.swapchain.acquireNextImage(UINT64_MAX, acquireSemaphore).value;

            std::chrono::time_point<std::chrono::high_resolution_clock> frameStart{ clock.now() };
            auto frameTime = frameStart - frameStarts[currentFrame];
            auto frameTimeF = static_cast<float>(frameTime.count()) / static_cast<float>(inFlightFrameCount);
            frameStarts[currentFrame] = frameStart;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * frameTimeF / (1.0e9f / 60.0f) * -Eigen::Vector3f::UnitZ();
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * frameTimeF / (1.0e9f / 60.0f) * Eigen::Vector3f::UnitZ();
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * frameTimeF / (1.0e9f / 60.0f) * Eigen::Vector3f::UnitX();
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camPos += camRot.toRotationMatrix() * (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? moveSens : moveSensSlow) * frameTimeF / (1.0e9f / 60.0f) * -Eigen::Vector3f::UnitX();

            auto submitSemaphore = vk::Semaphore{ swapchain.imageReadySemaphores[imageIndex] };

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
            vk::Rect2D renderArea{ { 0, 0 }, swapchain.description.extent };
            std::array<vk::RenderingAttachmentInfo, 1> colorAttachments{ {
                { swapchain.imageViews[imageIndex], vk::ImageLayout::eGeneral}
            } };
            commandBuffer.beginRendering({ {}, renderArea, 1, 0, colorAttachments });

            ImGui::Text("Frame Time: %f", frameTimeF / 1.0e6f);
            ImGui::Text("FPS: %f", 1.0e9f / frameTimeF);
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            commandBuffer.endRendering();
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

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}