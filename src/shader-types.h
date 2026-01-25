#pragma once

namespace he2rt::shaders {
	struct VertexDataReflection {
		uint32_t stride;
		uint32_t positionOffset;
		uint32_t normalOffset;
		uint32_t tangentOffset;
		uint32_t binormalOffset;
		uint32_t uvOffset;
		uint32_t uv2Offset;
	};

	struct Sampler {
		uint32_t textureId;
	};

	struct Material {
		uint8_t* paramBuffer = nullptr;
		uint32_t shaderId;
	};

	struct Mesh {
		uint16_t* indexBuffer = nullptr;
		uint8_t* vertexBuffer = nullptr;
		uint32_t materialId;
		VertexDataReflection vertexDataReflection;
	};

	struct Model {
		shaders::Mesh* meshes = nullptr;
	};

	struct PushConstant {
		shaders::Model* models = nullptr;
		shaders::Material* materials = nullptr;
		float4x4 viewInverse;
		float4x4 projInverse;
		uint32_t frame;
	};
}
