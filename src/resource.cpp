//#include <rip/binary/containers/mirage/v2.h>
//#include <rip/binary/serialization/ReflectionDeserializer.h>
//#include <rip/binary/stream.h>
//#include <ucsl-reflection/game-interfaces/standalone/game-interface.h>
//#include <ucsl/resources/model/v5.h>
//#include "mem_stream.h"
//#include "raytracing-engine.h"
//
//he2rt::RaytracingEngine* rte;
//
//using GI = ucsl::reflection::game_interfaces::standalone::StandaloneGameInterface;
//
//void loadMesh(ucsl::resources::model::v5::Mesh* mesh) {
//}
//
//void loadMeshGroup(ucsl::resources::model::v5::MeshGroup* meshGroup) {
//	for (auto* mesh : std::span(meshGroup->opaqueMeshes, meshGroup->opaqueMeshCount)) {
//		loadMesh(mesh);
//	}
//
//	for (auto* mesh : std::span(meshGroup->punchMeshes, meshGroup->punchMeshCount)) {
//		loadMesh(mesh);
//	}
//
//	for (auto* mesh : std::span(meshGroup->transparentMeshes, meshGroup->transparentMeshCount)) {
//		loadMesh(mesh);
//	}
//
//	for (unsigned int i = 0; i < meshGroup->customMeshTypeCount; i++) {
//		for (unsigned int j = 0; j < meshGroup->customMeshTypeMeshCounts[i]; j++)
//			loadMesh(meshGroup->customMeshTypeMeshes[i][j]);
//	}
//}
//
//void loadModel(ucsl::resources::model::v5::ContextsData* model) {
//	for (auto* meshGroup : std::span(model->meshGroups, model->meshGroupCount)) {
//		loadMeshGroup(meshGroup);
//	}
//}
//
//void loadModel(void* data, size_t size) {
//	imemstream ims{ (char*)data, size };
//	rip::binary::containers::mirage::v2::MirageResourceImageReader<unsigned int> reader{ ims };
//
//	auto root = reader.get_root_node();
//
//	root.forEachChild([](auto& rootChild) {
//		if (rootChild.header.magic == "Model   ") {
//			rootChild.forEachChild([](auto& modelChild) {
//				if (modelChild.header.magic == "Contexts") {
//					rip::binary::ReflectionDeserializer<GI, decltype(modelChild)> deserializer{ modelChild };
//
//					//auto* model = deserializer.deserialize<ucsl::resources::model::v5>(ucsl::reflection::providers::simplerfl<GI>::template reflect<ucsl::resources::model::v5>());
//					ucsl::resources::model::v5::ContextsData* model;
//
//				}
//				else if (modelChild.header.magic == "NodesExt") {
//
//				}
//				else if (modelChild.header.magic == "Topology") {
//
//				}
//				else if (modelChild.header.magic == "UserAABB") {
//
//				}
//			});
//		}
//	});
//}
