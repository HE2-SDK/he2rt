#include <shader-types.slang>
#include <lighting/material.slang>
#include <lighting/normals.slang>

using namespace he2rt::shaders;

struct Parameters {
    float4 Luminance;
    uint32_t diffuseTextureId;
	uint32_t specularTextureId;
    uint32_t normalTextureId;
    uint32_t emissionTextureId;
};

static const float3 ambient_color = float3(1.0f, 1.0f, 1.0f);

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;

    float luminance = params.Luminance.x;

    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    Sampler2D specularTexture = textures[params.specularTextureId];
    Sampler2D normalTexture = textures[params.normalTextureId];
    Sampler2D emissionTexture = textures[params.emissionTextureId];

    float4 diffuse = pow(abs(diffuseTexture.SampleLevel(payload.surface.uv, 0)), 2.2f);
    float4 specular = specularTexture.SampleLevel(payload.surface.uv, 0);
    float3 normal = UnpackNormalMapSafe(payload.surface.basis, normalTexture.SampleLevel(payload.surface.uv, 0).xy);
    float3 emission = emissionTexture.SampleLevel(payload.surface.uv2, 0).xyz;

    LightingParameters lightingParams;
    lightingParams.hit = payload.hit;
    lightingParams.world_normal = normal;
    LoadAlbedoTexture(lightingParams.material, payload.surface.vertexColor.xyz * diffuse.xyz);
    LoadPRMTexture(lightingParams.material, specular);

    payload.result.contribution = emission * ambient_color * luminance;

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
