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

//void describeMesh(auto mesh) {
//    std::string name = mesh.get_field<"name">().as_primitive().as<const char*>();
//
//    std::cout << "  " << name << std::endl;
//    std::cout << "    " << "Index count: " << mesh.get_field<"indexCount">().as_primitive().as<unsigned int>() << std::endl;
//    std::cout << "    " << "Vertex count: " << mesh.get_field<"vertexCount">().as_primitive().as<unsigned int>() << std::endl;
//    std::cout << "    " << "Vertex stride: " << mesh.get_field<"vertexStride">().as_primitive().as<unsigned int>() << std::endl;
//    std::cout << "    " << "Vertex buffer size: " << mesh.get_field<"vertexData">().as_pointer().get().value().as_carray().get_length() << std::endl;
//    std::cout << "    " << "Bone indices: " << mesh.get_field<"boneIndexCount">().as_primitive().as<unsigned int>() << std::endl;
//    std::cout << "    " << "Textures: " << mesh.get_field<"textureCount">().as_primitive().as<unsigned int>() << std::endl;
//
//    auto maybeTextures = mesh.get_field<"textures">().as_pointer().get();
//
//    if (maybeTextures.has_value()) {
//        auto textures = maybeTextures.value().as_carray();
//
//        for (size_t k = 0; k < textures.get_length(); k++) {
//            auto texture = textures[k].as_pointer().get().value().as_structure();
//
//            std::string textureName = texture.get_field<"name">().as_primitive().as<const char*>();
//
//            std::cout << "      " << textureName << " - " << static_cast<unsigned int>(texture.get_field<"index">().as_primitive().as<unsigned char>()) << std::endl;
//        }
//    }
//}
//
//void describeMeshes(auto meshes) {
//    for (size_t i = 0; i < meshes.get_length(); i++)
//        describeMesh(meshes[i].as_pointer().get().value().as_structure());
//}
//
//void describeMeshGroup(auto meshGroup) {
//    std::cout << "opaque meshes" << std::endl;
//    describeMeshes(meshGroup.get_field<"opaqueMeshes">().as_pointer().get().value().as_carray());
//
//    std::cout << "transparent meshes" << std::endl;
//    describeMeshes(meshGroup.get_field<"transparentMeshes">().as_pointer().get().value().as_carray());
//
//    std::cout << "punchthrough meshes" << std::endl;
//    describeMeshes(meshGroup.get_field<"punchthroughMeshes">().as_pointer().get().value().as_carray());
//}
//
//void describeMeshGroups(auto meshGroups) {
//    for (size_t i = 0; i < meshGroups.get_length(); i++) {
//        std::cout << "mesh group " << i << std::endl;
//
//        describeMeshGroup(meshGroups[i].as_pointer().get().value().as_structure());
//    }
//}

int main(void) {
    //std::ifstream ims{ "chr_sonic.model", std::ios::binary };
    //rip::binary::containers::needle_archive::v1::NeedleArchiveReader<unsigned int> reader{ ims };
    //reader.forEachChunk([&](auto& chunk) {
    //    std::string magic = chunk.header.magic;
    //    std::cout << magic << std::endl;

    //    if (chunk.header.magic == "NEDMDLV5") {
    //        std::cout << "=== Start of model " << chunk.header.name << " ===" << std::endl;

    //        rip::binary::containers::mirage::v2::MirageResourceImageReader<unsigned int, false, true> reader{ ims };

    //        auto root_node = reader.get_root_node();

    //        //dumpMirageHeaders(root_node);

    //        if (root_node.header.magic == "Model   ") {
    //            root_node.for_each_child([&](auto& modelChild) {
    //                if (modelChild.header.magic == "Contexts") {
    //                    auto& stream = modelChild.get_stream();

    //                    //// The size of this node in the header is wrong...
    //                    //char* buf = new char[root_node.get_size() - modelChild.get_size()];

    //                    ucsl::reflection::providers::BoundProvider::RootType refl{ ucsl::reflection::providers::simplerfl<GI, unsigned int>::Type<ucsl::resources::model::v5::reflections::ContextsData>{} };
    //                    typename rip::accessors::binary_istream<std::remove_reference_t<decltype(stream)>>::template ValueAccessor<decltype(refl)> acc{ stream, refl };

    //                    auto contexts = acc.as_structure();
    //                    describeMeshGroups(contexts.get_field<"meshGroups">().as_pointer().get().value().as_carray());
    //                }
    //                else if (modelChild.header.magic == "NodesExt") {

    //                }
    //                else if (modelChild.header.magic == "Topology") {

    //                }
    //                else if (modelChild.header.magic == "UserAABB") {

    //                }
    //            });
    //        }
    //    }
    //});

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

        auto physicalDevice = std::move(*maybePhysicalDevice);
        auto device = physicalDevice.createDevice();

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Device>(device.device));

        auto swapchain = device.createSwapchain(desiredExtent, surface);
        Scene scene{ device };

        std::cout << "swap chain contains " << swapchain.images.size() << " images." << std::endl;

        std::ifstream ims{ "chr_sonic.model", std::ios::binary };
        rip::binary::containers::needle_archive::v1::NeedleArchiveReader<unsigned int> reader{ ims };
        reader.forEachChunk([&](auto& chunk) {
            if (chunk.header.magic == "NEDMDLV5") {
                scene.addModel(ims);
            }
        });

        using VulkanCompatibleTransform = Eigen::Transform<float, 3, Eigen::AffineCompact, Eigen::RowMajor>;
        VulkanCompatibleTransform tf{ Eigen::Translation3f{ 0.0f, 0.0f, -2.0f } };

        scene.tlasInstances.push_back(vk::AccelerationStructureInstanceKHR{
            *reinterpret_cast<vk::TransformMatrixKHR*>(tf.data()),
            0,
            0xFF,
            0,
            vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
            scene.blases[0].deviceAddress,
        });

        scene.generateMeshBuffer();
        scene.buildTLAS();

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
        engine.globalPushConstant.meshes = (shaders::Mesh*)scene.meshBuffer.value().deviceAddress;

        glfwMakeContextCurrent(window);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            auto frameFence = vk::Fence{ frameFences[currentFrame] };
            auto acquireSemaphore = vk::Semaphore{ acquireSemaphores[currentFrame] };
            auto commandBuffer = vk::CommandBuffer{ commandBuffers[currentFrame] };

            device.device.waitForFences({ frameFence }, vk::True, UINT64_MAX);
            device.device.resetFences({ frameFence });

            auto imageIndex = swapchain.swapchain.acquireNextImage(UINT64_MAX, acquireSemaphore).value;

            auto submitSemaphore = vk::Semaphore{ swapchain.imageReadySemaphores[imageIndex] };

            commandBuffer.reset();
            commandBuffer.begin({ vk::CommandBufferUsageFlags{} });

            vk::Sampler sampler{ gbufferSampler.sampler };
            vk::ImageView gbufferImageView{ gbuffer.imageView };
            vk::ImageView tonemappedGbufferImageView{ tonemappedGbuffer.imageView };
            engine.raytrace(commandBuffer, gbufferImageView, swapchain.description.extent);

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