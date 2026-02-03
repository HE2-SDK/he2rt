#include <shader-types.slang>
#include <shading/color.slang>
#include <shading/material.slang>
#include <shading/math.slang>
#include <shading/needle.slang>

using namespace he2rt::shaders;

struct Parameters {
    float4 PBRFactor;
	uint32_t diffuseTextureId;
    uint32_t normalTextureId;
};

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;
    
    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    NormalMapSamplerWithBasis normalTexture = { textures[params.normalTextureId], payload.surface.basis };

    payload.result.material.normal = normalTexture.SampleLevel(payload.surface.uvs[0], 0);
    payload.result.material.albedo = srgbToLinearSimple(diffuseTexture.SampleLevel(payload.surface.uvs[0], 0).xyz);
    loadNeedlePRM(payload.result.material, unpackPBRFactor(params.PBRFactor));
}
