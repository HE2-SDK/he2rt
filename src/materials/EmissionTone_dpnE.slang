#include <shader-types.slang>
#include <shading/color.slang>
#include <shading/material.slang>
#include <shading/math.slang>
#include <shading/needle.slang>

using namespace he2rt::shaders;

struct Parameters {
    float4 Luminance;
    uint32_t diffuseTextureId;
	uint32_t specularTextureId;
    uint32_t normalTextureId;
    uint32_t emissionTextureId;
    uint32_t s_LuminanceTextureId;
};

static const float3 ambient_color = float3(1.0f, 1.0f, 1.0f);

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;

    float luminance = params.Luminance.x;

    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    SpecularSampler specularTexture = { textures[params.specularTextureId] };
    NormalMapSamplerWithBasis normalTexture = { textures[params.normalTextureId], payload.surface.basis };
    Sampler2D emissionTexture = textures[params.emissionTextureId];
    Sampler2D luminanceTexture = textures[params.s_LuminanceTextureId];

    float3 diffuse = srgbToLinearSimple(diffuseTexture.SampleLevel(payload.surface.uv, 0).xyz);
    NeedlePRMParameters specular = specularTexture.SampleLevel(payload.surface.uv, 0);
    float3 normal = normalTexture.SampleLevel(payload.surface.uv, 0);
    float3 emission = emissionTexture.SampleLevel(payload.surface.uv, 0).xyz;
    float luminanceTone = luminanceTexture.SampleLevel(float2(0.75, 0.5), 0).x;

    payload.result.material.normal = normal;
    payload.result.material.albedo = payload.surface.vertexColor.xyz * diffuse.xyz;
    loadNeedlePRM(payload.result.material, specular);

    payload.result.emission = emission * ambient_color * luminance * luminanceTone;
}
