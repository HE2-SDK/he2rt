#include "lighting/light.slang"
#include "shader-types.h"

[[vk::push_constant]] ConstantBuffer<he2rt::shaders::PushConstant> pushConst;
[[vk::binding(0, 0)]] Sampler2D textures[];
[[vk::binding(0, 1)]] RaytracingAccelerationStructure tlas;
[[vk::binding(1, 1)]] RWTexture2D<float4> outImage;

static const float INFINITE = 1e32F;

struct PathTracingState {
    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    float3 throughput = float3(1.0f, 1.0f, 1.0f);
};

struct SurfaceParameters {
    Basis basis;
    float2 uv;
    float2 uv2;
};

struct MaterialShaderPayload {
    uint32_t materialId;
    HitParameters hit;
    SurfaceParameters surface;
    float3 randomness;
    float3 contribution;
    float3 throughput;
    float3 nextDirection;
};
