#include <shader-types.slang>
#include <shading/color.slang>
#include <shading/material.slang>
#include <shading/math.slang>
#include <shading/needle.slang>

using namespace he2rt::shaders;

struct Parameters {
	uint32_t diffuseTextureId;
	uint32_t specularTextureId;
    uint32_t normalTextureId;
};

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;
    
    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    SpecularSampler specularTexture = { textures[params.specularTextureId] };
    NormalMapSamplerWithBasis normalTexture = { textures[params.normalTextureId], payload.surface.basis };

    payload.result.material.normal = normalTexture.SampleLevel(payload.surface.uv, 0);
    payload.result.material.albedo = srgbToLinearSimple(diffuseTexture.SampleLevel(payload.surface.uv, 0).xyz);
    loadNeedlePRM(payload.result.material, specularTexture.SampleLevel(payload.surface.uv, 0));
}
