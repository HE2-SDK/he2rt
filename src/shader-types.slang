#include "lighting/light.slang"
#include "shader-types.h"

static const float INFINITE = 1e32F;

struct SurfaceParameters {
    Basis basis;
    float2 uv;
    float2 uv2;
};

struct MaterialShaderPayload {
    uint32_t materialId;
    RayDesc ray;
    float weight;
    float3 color;
    HitParameters hit;
    SurfaceParameters surface;
};
