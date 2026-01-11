#pragma once

namespace he2rt::shaders {
	struct VertexDataReflection {
		uint32_t stride;
		uint32_t positionOffset;
		uint32_t normalOffset;
	};

	struct Mesh {
		uint16_t* indexBuffer = nullptr;
		uint8_t* vertexBuffer = nullptr;
		VertexDataReflection vertexDataReflection;
	};

	struct PushConstant {
		shaders::Mesh* meshes = nullptr;
		float4x4 viewInverse;
		float4x4 projInverse;
	};
}
