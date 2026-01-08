#pragma once

namespace he2rt::shaders {
	struct VertexDataReflection {
		uint8_t stride;
		uint8_t positionOffset;
		uint8_t normalOffset;
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
