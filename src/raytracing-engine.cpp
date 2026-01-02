#include "raytracing-engine.h"
#include "staging-area.h"

namespace he2rt {
    RaytracingEngine::RaytracingEngine(Device& device)
        : device{ device } {
    }

    //AccelerationStructure RaytracingEngine::buildBLAS(const ucsl::resources::model::v5::MeshGroup& model) {
    //    //for (auto* mesh : std::span(model.opaqueMeshes, model.opaqueMeshCount)) {
    //    //    loadMesh(mesh);
    //    //}

    //    //for (auto* mesh : std::span(model.punchMeshes, model.punchMeshCount)) {
    //    //    loadMesh(mesh);
    //    //}

    //    //for (auto* mesh : std::span(model.transparentMeshes, model.transparentMeshCount)) {
    //    //    loadMesh(mesh);
    //    //}

    //    vk::AccelerationStructureGeometryTrianglesDataKHR geometryInstances{ {}, { buffer.deviceAddress } };
    //    vk::AccelerationStructureGeometryKHR geometry{ vk::GeometryTypeKHR::eTriangles, triangles };
    //    vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{ scene.tlasInstances.size() };

    //    return device.buildBLAS(geometry, rangeInfo);
    //}
}
