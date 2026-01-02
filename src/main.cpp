#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>
#include <set>
#include "instance.h"

using namespace he2rt;

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

        vk::Extent2D desiredExtent{ (uint32_t)width, (uint32_t)height };

        auto surface = createSurface(instance, window);
        auto maybePhysicalDevice = instance.findPhysicalDevice(desiredExtent, surface);

        if (!maybePhysicalDevice.has_value())
            throw std::runtime_error{ "can't find suitable device" };

        auto physicalDevice = std::move(*maybePhysicalDevice);
        auto device = physicalDevice.createDevice();
        auto swapchain = device.createSwapchain(desiredExtent, surface);

        glfwMakeContextCurrent(window);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}