#include <shader-types.slang>
#include <lighting/material.slang>
#include <lighting/normals.slang>

using namespace he2rt::shaders;

struct Parameters {
    float4 PBRFactor;
    float4 Offset;
    uint32_t diffuseTextureId;
    uint32_t normalTextureId;
    uint32_t transparencyTextureId;
    uint32_t reflectionTextureId;
};

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;

    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    Sampler2D normalTexture = textures[params.normalTextureId];
    Sampler2D transparencyTexture = textures[params.transparencyTextureId];
    Sampler2D reflectionTexture = textures[params.reflectionTextureId];

    float2 offset = params.Offset.xy * (2 * reflectionTexture.SampleLevel(payload.surface.uv, 0).xy - float2(1));

    // applied to all uvs except uvs[1]
    float2 uv = payload.surface.uv + offset;
    float2 uv2 = payload.surface.uv2 + offset;

    float4 diffuse = diffuseTexture.SampleLevel(uv, 0);
    float transparency = payload.surface.vertexColor.w * diffuse.w * transparencyTexture.SampleLevel(uv, 0).x;

    LightingParameters lightingParams;
    lightingParams.hit = payload.hit;
    lightingParams.world_normal = UnpackNormalMapSafe(payload.surface.basis, normalTexture.SampleLevel(uv, 0).xy);
    LoadAlbedoTexture(lightingParams.material, pow(abs(diffuse.xyz), 2.2));
    LoadPBRFactor(lightingParams.material, params.PBRFactor);

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
