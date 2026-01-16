#include "shader-types.slang"

using namespace he2rt::shaders;

[[vk::push_constant]] ConstantBuffer<PushConstant> pushConst;
[[vk::binding(2)]] Sampler2D textures[];

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
    
    float distance = length(normalize(payload.position));
    float diffuse = max(dot(payload.lightDirection, payload.normal), 0.0);
    
    diffuse = diffuse * (1.0 / (1.0 + 0.25 * distance * distance));

    //payload.color = float3(params.diffuse, params.specular, params.normal) / float3(30.0f);
    
    payload.color = diffuse * diffuseTexture.SampleLevel(payload.uv, 0).xyz;
}
