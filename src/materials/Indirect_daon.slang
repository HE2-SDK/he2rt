#include <shader-types.slang>
#include <shading/color.slang>
#include <shading/material.slang>
#include <shading/math.slang>
#include <shading/needle.slang>

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
    NormalMapSamplerWithBasis normalTexture = { textures[params.normalTextureId], payload.surface.basis };
    Sampler2D transparencyTexture = textures[params.transparencyTextureId];
    Sampler2D reflectionTexture = textures[params.reflectionTextureId];

    float2 offset = params.Offset.xy * (2 * reflectionTexture.SampleLevel(payload.surface.uvs[0], 0).xy - float2(1));

    // applied to all uvs except uvs[1]
    float2 uvs[4] = {
        payload.surface.uvs[0] + offset,
        payload.surface.uvs[1],
        payload.surface.uvs[2] + offset,
        payload.surface.uvs[3] + offset,
    };

    float4 diffuse = diffuseTexture.SampleLevel(uvs[0], 0);
    float transparency = payload.surface.vertexColor.w * diffuse.w * transparencyTexture.SampleLevel(uvs[0], 0).x;

    payload.result.material.normal = normalTexture.SampleLevel(uvs[0], 0);
    payload.result.material.albedo = srgbToLinearSimple(diffuse.xyz);
    loadNeedlePRM(payload.result.material, unpackPBRFactor(params.PBRFactor));
}
