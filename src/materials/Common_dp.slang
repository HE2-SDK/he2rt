#include <shader-types.slang>
#include <lighting/material.slang>
#include <lighting/normals.slang>

using namespace he2rt::shaders;

struct Parameters {
	uint32_t diffuseTextureId;
	uint32_t specularTextureId;
};

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;
    
    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    Sampler2D specularTexture = textures[params.specularTextureId];

    LightingParameters lightingParams;
    lightingParams.hit = payload.hit;
    lightingParams.world_normal = payload.surface.basis.normal;
    LoadAlbedoTexture(lightingParams.material, diffuseTexture.SampleLevel(payload.surface.uv, 0).xyz);
    LoadPRMTexture(lightingParams.material, specularTexture.SampleLevel(payload.surface.uv, 0).xyzw);

    payload.result.contribution = float3(0.0f, 0.0f, 0.0f);

    if (dot(lightingParams.hit.light_direction, lightingParams.world_normal) > 0.0f) {
        BSDFEvaluationResult sunlight = EvaluateMaterial(lightingParams);
        payload.result.contribution += sunlight.color;
    }

    payload.result.nextDirection = SampleMaterial(lightingParams, payload.randomness);
    lightingParams.hit.light_direction = payload.result.nextDirection;
    
    BSDFEvaluationResult sampled = EvaluateMaterial(lightingParams);

    float3 throughput = sampled.color / sampled.pdf;
    payload.result.throughput = any(isnan(throughput)) || sampled.pdf < 0.0001f ? 0.0f : throughput;
}
