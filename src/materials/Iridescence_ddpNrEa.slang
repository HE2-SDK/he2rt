#include <shader-types.slang>
#include <shading/color.slang>
#include <shading/material.slang>
#include <shading/math.slang>
#include <shading/needle.slang>

using namespace he2rt::shaders;

struct Parameters {
    float4 FalloffAndLuminance;
    float4 IridescenceOffset;
    float4 IridescenceMask;
    float4 WPBRFactor;
    uint32_t diffuseTextureId;
    uint32_t diffuse1TextureId;
	uint32_t specularTextureId;
    uint32_t normalTextureId;
    uint32_t reflectionTextureId;
    uint32_t emissionTextureId;
    uint32_t transparencyTextureId;
    uint32_t s_LuminanceTextureId;
};

float2 calculateCameraBias(int rightFactor, int upFactor, int2 offsets) {
    float3 right = pushConst.viewInverse._m00_m01_m02 * rightFactor;
    float3 up = pushConst.viewInverse._m10_m11_m12 * upFactor;
    float3 forward = -pushConst.viewInverse._m20_m21_m22;

    float bias = dot(right + up + forward, float3(1, 1, 1));

    return float2(bias) + float2(offsets);
}

static const float3 ambient_color = float3(1.0f, 1.0f, 1.0f);

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;

    float falloff = params.FalloffAndLuminance.x;
    float iridescenceLuminance = params.FalloffAndLuminance.y;
    float emissionLuminance = params.FalloffAndLuminance.z;

    NeedleWPRMParameters wpbrFactors[2] = unpackWPBRFactor(params.WPBRFactor);

    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    Sampler2D diffuse1Texture = textures[params.diffuse1TextureId];
    SpecularSampler specularTexture = { textures[params.specularTextureId] };
    NormalMapSamplerWithBasis normalTexture = { textures[params.normalTextureId], payload.surface.basis };
    Sampler2D reflectionTexture = textures[params.reflectionTextureId];
    Sampler2D emissionTexture = textures[params.emissionTextureId];
    Sampler2D transparencyTexture = textures[params.transparencyTextureId];
    Sampler2D luminanceTexture = textures[params.s_LuminanceTextureId];

    float4 diffuse = diffuseTexture.SampleLevel(payload.surface.uvs[0], 0);
    float4 diffuse1 = diffuse1Texture.SampleLevel(payload.surface.uvs[1], 0);
    NeedlePRMParameters specular = specularTexture.SampleLevel(payload.surface.uvs[0], 0);
    float3 emission = emissionTexture.SampleLevel(payload.surface.uvs[0], 0).xyz;
    float transparency = transparencyTexture.SampleLevel(payload.surface.uvs[0], 0).x;
    float luminance = luminanceTexture.SampleLevel(float2(0.75, 0.5), 0).x;

    float3 normal0 = normalTexture.SampleLevel(payload.surface.uvs[0], 0);
    float3 normal1 = normalTexture.SampleLevel(payload.surface.uvs[1], 0);

    float blendFactor = saturate(payload.surface.vertexColor.x * diffuse.w * diffuse1.w);
    float3 normalBlended = normalize(lerp(normal0, normal1, blendFactor));

//    float3 cameraPosition = pushConst.viewInverse._m30_m31_m32;
//    float3 iridescenceView = normalize(cameraPosition - payload.hit.world_position); // w components from normals

    float2 cameraBias = calculateCameraBias(0, 0, int2(0, 0));

    float iridescenceNdotV0 = dot(normal0, payload.V);
    float iridescenceNdotV1 = dot(payload.surface.basis.normal, payload.V);

    float fresnel0 = pow(1.0 - abs(iridescenceNdotV0), falloff);
    float fresnel1 = pow(1.0 - abs(iridescenceNdotV1), falloff);

    float2 iridescenceUV0 = float2(params.IridescenceOffset.x, iridescenceNdotV0 + params.IridescenceOffset.z * cameraBias.x);
    float2 iridescenceUV1 = float2(params.IridescenceOffset.y, iridescenceNdotV1 + params.IridescenceOffset.w * cameraBias.y);

    float3 iridescence0 = reflectionTexture.SampleLevel(iridescenceUV0, 0).xyz * fresnel0;
    float3 iridescence1 = reflectionTexture.SampleLevel(iridescenceUV1, 0).xyz * fresnel1;

    float iridescenceFresnel = fresnel0 * fresnel1;
    float3 iridescence = iridescence0 * iridescence1 * params.IridescenceMask.xyz;

    float3 diffuseTotal = diffuse.xyz + iridescence;
    float3 diffuseBlended = srgbToLinearSimple(lerp(diffuseTotal.xyz, diffuse1.xyz, blendFactor));

    //float3 textureF0 = lerp(specular.specular, diffuseTotal.xyz, specular.metallic);
    //float3 pbrFactor0F0 = lerp(0.1, diffuseTotal.xyz, wpbrFactors[0].metallic);
    //float3 pbrFactor1F0 = lerp(0.1, diffuse1.xyz, wpbrFactors[1].metallic);

    float iridescenceSpecular = lerp(specular.specular, 0.1, iridescenceFresnel);
    // float3 iridescenceF0 = lerp(textureF0, pbrFactor0F0, iridescenceFresnel);
    float iridescenceRoughness = lerp(specular.roughness, wpbrFactors[0].roughness, iridescenceFresnel);
    float iridescenceMetallic = lerp(specular.metallic, wpbrFactors[0].metallic, iridescenceFresnel);

    float totalSpecular = lerp(iridescenceSpecular, 0.1, blendFactor);
    //float3 totalF0 = lerp(iridescenceF0, pbrFactor1F0, blendFactor);
    float totalRoughness = lerp(iridescenceRoughness, wpbrFactors[1].roughness, blendFactor);
    float totalMetallic = lerp(iridescenceMetallic, wpbrFactors[1].metallic, blendFactor);

    payload.result.material.normal = normalBlended;
    payload.result.material.albedo = diffuseBlended;
    payload.result.material.specular = totalSpecular;
    payload.result.material.roughness = totalRoughness;
    payload.result.material.metallic = totalMetallic;
    payload.result.material.cavity = 0.0f;

    payload.result.emission
        = iridescence * (1 - blendFactor) * iridescenceLuminance
        + emission * (1 - blendFactor) * emissionLuminance * ambient_color * transparency * luminance;
}
