#include <shader-types.slang>
#include <shading/color.slang>
#include <shading/material.slang>
#include <shading/needle.slang>

using namespace he2rt::shaders;
using namespace bsdf::pbr;

struct Parameters {
    float4 PBRFactor;
	uint32_t diffuseTextureId;
};

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;
    
    Sampler2D diffuseTexture = textures[params.diffuseTextureId];

    payload.result.material.normal = payload.surface.basis.normal;
    payload.result.material.albedo = srgbToLinearSimple(diffuseTexture.SampleLevel(payload.surface.uvs[0], 0).xyz);
    loadNeedlePRM(payload.result.material, unpackPBRFactor(params.PBRFactor));
}
