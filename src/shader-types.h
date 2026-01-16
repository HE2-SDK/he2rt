#pragma once

namespace he2rt::shaders {
	struct VertexDataReflection {
		uint32_t stride;
		uint32_t positionOffset;
		uint32_t normalOffset;
		uint32_t uvOffset;
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

	struct PushConstant {
		shaders::Mesh* meshes = nullptr;
		shaders::Material* materials = nullptr;
		float4x4 viewInverse;
		float4x4 projInverse;
	};
}
