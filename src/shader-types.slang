#include "shader-types.h"

namespace he2rt::shaders {
	struct MaterialShaderPayload {
		uint32_t materialId;
        RayDesc ray;
		float weight;
		float2 uv;
		float3 color;
		float3 position;
		float3 normal;
		float3 viewDirection;
		float3 lightDirection;
	};
}
