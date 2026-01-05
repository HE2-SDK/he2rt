#include "scene.h"
#include <iostream>

using GI = ucsl::reflection::game_interfaces::standalone::StandaloneGameInterface;

namespace he2rt {
    Model::Model(Buffer&& indexBuffer, Buffer&& vertexBuffer) : indexBuffer{ std::move(indexBuffer) }, vertexBuffer{ std::move(vertexBuffer) } {
    }

    Model::Model(Model&& other) : indexBuffer{ std::move(other.indexBuffer) }, vertexBuffer{ std::move(other.vertexBuffer) } {
    }

    Model& Model::operator=(Model&& other) {
        std::cout << "moving assigning model" << std::endl;
        indexBuffer = std::move(other.indexBuffer);
        vertexBuffer = std::move(other.vertexBuffer);
        return *this;
    }

    Scene::Scene(Device& device) : device{ device } {
    }

    void Scene::addModel(std::istream& stream) {
        rip::binary::containers::mirage::v2::MirageResourceImageReader<unsigned int, false, true> reader{ stream };

        auto root_node = reader.get_root_node();

        if (root_node.header.magic == "Model   ") {
            root_node.for_each_child([&](auto& modelChild) {
                if (modelChild.header.magic == "Contexts") {
                    auto& stream = modelChild.get_stream();

                    ucsl::reflection::providers::BoundProvider::RootType refl{ ucsl::reflection::providers::simplerfl<GI, unsigned int>::Type<ucsl::resources::model::v5::reflections::ContextsData>{} };
                    typename rip::accessors::binary_istream<std::remove_reference_t<decltype(stream)>>::template ValueAccessor<decltype(refl)> acc{ stream, refl };

                    auto contexts = acc.as_structure();
                    addMeshGroup(contexts.get_field<"meshGroups">().as_pointer().get().value().as_carray()[0].as_pointer().get().value().as_structure());
                }
                else if (modelChild.header.magic == "NodesExt") {

                }
                else if (modelChild.header.magic == "Topology") {

                }
                else if (modelChild.header.magic == "UserAABB") {

                }
            });
        }
	}

    void Scene::buildTLAS() {
        auto buffer = device.createBuffer(
            tlasInstances.size() * sizeof(vk::AccelerationStructureInstanceKHR),
            16,
            vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eShaderDeviceAddress | vk::BufferUsageFlagBits2::eTransferDst
        );

        {
            StagingArea staging{ device };
            staging.addBuffer(buffer, std::span{ tlasInstances });
            device.runOneTimeCommandBuffer([&](auto& cmd) { staging.upload(cmd); });
        }

        vk::AccelerationStructureGeometryInstancesDataKHR geometryInstances{ {}, { buffer.deviceAddress } };
        vk::AccelerationStructureGeometryKHR geometry{ vk::GeometryTypeKHR::eInstances, geometryInstances };
        vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{ (uint32_t)tlasInstances.size() };

        tlas = device.buildTLAS(geometry, rangeInfo);
    }
}