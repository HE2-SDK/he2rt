#include "shader-types.slang"
#include "lighting/material.slang"
#include "lighting/normals.slang"

using namespace he2rt::shaders;

[[vk::push_constant]] ConstantBuffer<PushConstant> pushConst;
[[vk::binding(0, 0)]] Sampler2D textures[];

struct Parameters {
	uint32_t diffuseTextureId;
	uint32_t specularTextureId;
    uint32_t normalTextureId;
};

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;
    
    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    Sampler2D specularTexture = textures[params.specularTextureId];
    Sampler2D normalTexture = textures[params.normalTextureId];

    LightingParameters lightingParams;
    lightingParams.hit = payload.hit;
    lightingParams.world_normal = UnpackNormalMapSafe(payload.surface.basis, normalTexture.SampleLevel(payload.surface.uv, 0).xy);
    LoadAlbedoTexture(lightingParams.material, diffuseTexture.SampleLevel(payload.surface.uv, 0).xyz);
    LoadPRMTexture(lightingParams.material, specularTexture.SampleLevel(payload.surface.uv, 0).xyzw);

    //payload.color = float3(params.diffuse, params.specular, params.normal) / float3(30.0f);

    payload.contribution = float3(0.0f, 0.0f, 0.0f);

    if (dot(lightingParams.hit.light_direction, lightingParams.world_normal) > 0.0f) {
        BSDFEvaluationResult sunlight = EvaluateMaterial(lightingParams);
        payload.contribution += sunlight.color;
    }

    payload.nextDirection = SampleMaterial(lightingParams, payload.randomness);
    lightingParams.hit.light_direction = payload.nextDirection;

    BSDFEvaluationResult sampled = EvaluateMaterial(lightingParams);
    payload.throughput = sampled.color / sampled.pdf;
}
