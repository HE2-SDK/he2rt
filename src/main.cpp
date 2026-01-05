#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>
#include <set>
#include "instance.h"
#include "scene.h"
#include "raytracing-engine.h"

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
            vk::ImageLayout::eGeneral,
        }) }
        , imageView{ device.device.createImageView({
            {},
            image.image,
            vk::ImageViewType::e2D,
            format,
            { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eOne },
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
    auto* window = glfwCreateWindow(640, 480, "HE2 RT", NULL, NULL);
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

        mat34 tf{ mat34::Identity() };

        scene.tlasInstances.push_back(vk::AccelerationStructureInstanceKHR{
            *reinterpret_cast<vk::TransformMatrixKHR*>(&tf),
            0,
            0xFF,
            0,
            vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable,
            scene.blases[0].deviceAddress,
        });

        scene.buildTLAS();

        auto commandPool = device.device.createCommandPool({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, physicalDevice.queueIndices.graphics });

        constexpr size_t inFlightFrameCount = 2;
        size_t currentFrame{};

        std::vector<vk::raii::Fence> frameFences{};
        std::vector<vk::raii::Semaphore> acquireSemaphores{};
        std::vector<vk::raii::CommandBuffer> commandBuffers{};

        GBuffer gbuffer{ device, swapchain, vk::Format::eR32G32B32A32Sfloat };
        GBuffer tonemappedGbuffer{ device, swapchain, vk::Format::eB8G8R8A8Unorm };

        for (size_t i = 0; i < inFlightFrameCount; i++) {
            frameFences.emplace_back(device.device.createFence({ vk::FenceCreateFlagBits::eSignaled }));
            acquireSemaphores.emplace_back(device.device.createSemaphore({}));
            commandBuffers.emplace_back(std::move(device.device.allocateCommandBuffers({ commandPool, vk::CommandBufferLevel::ePrimary, 1 })[0]));
        }

        RaytracingEngine engine{ device, scene };

        mat44 projMat{ projectionMatrix(60.0f, swapchain.description.extent.width / swapchain.description.extent.height, 10.0f, 10000.0f) };
        engine.globalPushConstant.projInverse = projMat.inverse();

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

            auto gbufferImageView = vk::ImageView{ gbuffer.imageView };
            engine.raytrace(commandBuffer, gbufferImageView, swapchain.description.extent);

            vk::ImageCopy imageCopy{
                { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                { 0, 0, 0 },
                { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                { 0, 0, 0 },
                { swapchain.description.extent.width, swapchain.description.extent.height, 1 },
            };
            commandBuffer.copyImage(framebuffer, vk::ImageLayout::eGeneral, swapchain.images[imageIndex], vk::ImageLayout::ePresentSrcKHR, { imageCopy });

            commandBuffer.end();

            vk::PipelineStageFlags stageFlags{ vk::PipelineStageFlagBits::eRayTracingShaderKHR };
            device.queues.graphics.submit({ { { acquireSemaphore }, { stageFlags }, { commandBuffer }, { submitSemaphore } } }, frameFence);

            auto swp = vk::SwapchainKHR{ swapchain.swapchain };
            device.queues.present.presentKHR({ { submitSemaphore }, { swp }, { imageIndex } });

            currentFrame = (currentFrame + 1) % inFlightFrameCount;
        }
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}