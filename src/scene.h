#pragma once
#include <vector>
#include <optional>
#include <vk_mem_alloc.hpp>
#include <vk_mem_alloc_raii.hpp>
#include "math.h"
#include "acceleration-structures.h"
#include "util.h"
#include "staging-area.h"
#include "device.h"
#include "buffer.h"

#include <rip/binary/containers/needle-archive/v1.h>
#include <rip/binary/containers/mirage/v2.h>
#include <rip/binary/serialization/ReflectionDeserializer.h>
#include <rip/binary/stream.h>
#include <ucsl-reflection/game-interfaces/standalone/game-interface.h>
#include <ucsl-reflection/providers/simplerfl.h>
#include <ucsl-reflection/reflections/resources/model/v5.h>
#include <ucsl-reflection/bound-reflection.h>
#include <rip/binary/accessors/binary-stream.h>
#include "mem_stream.h"
#include "shader-types.h"

#include <entt/entt.hpp>

namespace he2rt {
	struct Texture {

	};

	struct Material {

	};

	struct Model {
		Buffer indexBuffer;
		Buffer vertexBuffer;

		Model(Buffer&& indexBuffer, Buffer&& vertexBuffer);
		Model(const Model& other) = delete;
		Model(Model&& other);

		Model& operator=(const Model& other) = delete;
		Model& operator=(Model&& other);
	};

	struct ModelInstance {
		uint32_t modelId{};
		float3x4 transform{};
	};

	struct Light {
		float3 color{};
	};

	struct LightInstance {
		uint32_t lightId{};
		float3x4 transform{};
	};

	class Device;
	struct Scene {
		Device& device;

		entt::registry registry{};

		std::vector<Model> models{};
		std::vector<ModelInstance> modelInstances{};
		std::vector<Light> lights{};
		std::vector<LightInstance> lightInstances{};

		std::vector<AccelerationStructure> blases{};
		std::vector<vk::AccelerationStructureInstanceKHR> tlasInstances{};
		std::optional<AccelerationStructure> tlas{};

		std::vector<shaders::Mesh> shaderMeshes{};
		std::optional<Buffer> meshBuffer{};

		Scene(Device& device);

		void addModel(std::istream& stream);

		void addMesh(auto mesh) {
			std::string name = mesh.get_field<"name">().as_primitive().as<const char*>();
			
			unsigned int indexCount = mesh.get_field<"indexCount">().as_primitive().as<unsigned int>();
			unsigned int vertexCount = mesh.get_field<"vertexCount">().as_primitive().as<unsigned int>();
			unsigned int triangleCount = indexCount / 3u;

			auto indices = mesh.get_field<"indices">().as_pointer().get().value().as_carray();
			size_t indexBufSize = indexCount * sizeof(unsigned short);
			std::vector<unsigned short> indexBuf(indexCount);

			for (size_t i = 0; i < indexCount; i++)
				indexBuf[i] = indices[i].as_primitive().as<unsigned short>();

			auto vertexData = mesh.get_field<"vertexData">().as_pointer().get().value().as_carray();
			unsigned int vertexStride = mesh.get_field<"vertexStride">().as_primitive().as<unsigned int>();

			size_t vertexBufSize = vertexCount * vertexStride;

			std::vector<unsigned char> vertexBuf(vertexBufSize);

			for (size_t i = 0; i < vertexBufSize; i++)
				vertexBuf[i] = vertexData[i].as_primitive().as<unsigned char>();

			for (size_t i = 0; i < vertexCount; i++) {
				auto* pos = reinterpret_cast<float*>(&vertexBuf[i * vertexStride]);

				pos[0] = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned int>(pos[0])));
				pos[1] = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned int>(pos[1])));
				pos[2] = std::bit_cast<float>(_byteswap_ulong(std::bit_cast<unsigned int>(pos[2])));

				auto* nrm = reinterpret_cast<uint8_t*>(&vertexBuf[i * vertexStride + 0xC]);
				auto nrmSwapped = _byteswap_ulong(*reinterpret_cast<uint32_t*>(nrm));

				nrm[0] = static_cast<uint8_t>(nrmSwapped >> 2);
				nrm[1] = static_cast<uint8_t>(nrmSwapped >> 12);
				nrm[2] = static_cast<uint8_t>(nrmSwapped >> 22);
				nrm[3] = static_cast<uint8_t>(nrmSwapped >> 24) | 0x3F;
			}

			std::cout << " ==== Loading model " << name << " ====" << std::endl;
			std::cout << "  Index count: " << indexCount << std::endl;
			std::cout << "  Vertex count: " << vertexCount << std::endl;
			std::cout << "  Vertex stride: " << vertexStride << std::endl;
			std::cout << "  Vertex buffer size: " << vertexBufSize << std::endl;
			//std::cout << "  Bone indices: " << mesh.get_field<"boneIndexCount">().as_primitive().as<unsigned int>() << std::endl;
			//std::cout << "  Textures: " << mesh.get_field<"textureCount">().as_primitive().as<unsigned int>() << std::endl;

			auto indexBuffer = device.createBuffer(
				util::align(indexBufSize, 16),
				16,
				vk::BufferUsageFlagBits2::eIndexBuffer | vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eTransferDst
			);

			auto vertexBuffer = device.createBuffer(
				util::align(vertexBufSize, 16),
				16,
				vk::BufferUsageFlagBits2::eVertexBuffer | vk::BufferUsageFlagBits2::eStorageBuffer | vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eTransferDst
			);

			{
				StagingArea staging{ device };

				staging.addBuffer(indexBuffer, indexBuf.data(), indexBufSize);
				staging.addBuffer(vertexBuffer, vertexBuf.data(), vertexBufSize);

				device.runOneTimeCommandBuffer([&](auto& cmd) {
					staging.upload(cmd);
				});
			}

			vk::AccelerationStructureGeometryTrianglesDataKHR triangles{
				vk::Format::eR32G32B32Sfloat,
				vertexBuffer.deviceAddress,
				vertexStride,
				vertexCount - 1,
				vk::IndexType::eUint16,
				indexBuffer.deviceAddress,
			};

			vk::AccelerationStructureGeometryKHR geometry{
				vk::GeometryTypeKHR::eTriangles,
				triangles,
				vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation | vk::GeometryFlagBitsKHR::eOpaque,
			};

			//const auto entity = registry.create();
			models.emplace_back(std::move(indexBuffer), std::move(vertexBuffer));
			shaderMeshes.emplace_back(shaders::Mesh{
				(uint16_t*)indexBuffer.deviceAddress,
				(uint8_t*)vertexBuffer.deviceAddress,
				shaders::VertexDataReflection{ static_cast<uint8_t>(vertexStride), 0, 12 },
			});

			blases.emplace_back(device.buildBLAS(geometry, { triangleCount }));
			//auto maybeTextures = mesh.get_field<"textures">().as_pointer().get();

			//if (maybeTextures.has_value()) {
			//	auto textures = maybeTextures.value().as_carray();

			//	for (size_t k = 0; k < textures.get_length(); k++) {
			//		auto texture = textures[k].as_pointer().get().value().as_structure();

			//		std::string textureName = texture.get_field<"name">().as_primitive().as<const char*>();

			//		std::cout << "      " << textureName << " - " << static_cast<unsigned int>(texture.get_field<"index">().as_primitive().as<unsigned char>()) << std::endl;
			//	}
			//}
			std::cout << " ==== Finished loading model " << name << " ====" << std::endl;
		}

		void addMeshes(auto meshes) {
			for (size_t i = 0; i < meshes.get_length(); i++)
				addMesh(meshes[i].as_pointer().get().value().as_structure());
		}

		void addMeshGroup(auto meshGroup) {
			addMeshes(meshGroup.get_field<"opaqueMeshes">().as_pointer().get().value().as_carray());
			addMeshes(meshGroup.get_field<"transparentMeshes">().as_pointer().get().value().as_carray());
			addMeshes(meshGroup.get_field<"punchthroughMeshes">().as_pointer().get().value().as_carray());
		}

		void generateMeshBuffer();
		void buildTLAS();
	};
}
