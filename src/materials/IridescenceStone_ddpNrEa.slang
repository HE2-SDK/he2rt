#include <shader-types.slang>
#include <lighting/material.slang>
#include <lighting/normals.slang>

using namespace he2rt::shaders;

struct Parameters {
    float4 FalloffAndLuminance;
    float4 IridescenceOffset;
    float4 WPBRFactor;
    uint32_t diffuseTextureId;
    uint32_t diffuse1TextureId;
	uint32_t specularTextureId;
    uint32_t normalTextureId;
    uint32_t reflectionTextureId;
    uint32_t emissionTextureId;
    uint32_t transparencyTextureId;
};

float2 calculateCameraBias(int rightFactor, int upFactor, int2 offsets) {
    float3 right = pushConst.viewInverse._m00_m01_m02 * rightFactor;
    float3 up = pushConst.viewInverse._m10_m11_m12 * upFactor;
    float3 forward = -pushConst.viewInverse._m20_m21_m22;

    float bias = dot(right + up + forward, float3(1, 1, 1));

    return float2(bias) + float2(offsets);
}

static const float2 ambient_color = float2(0.525f, 0.149f);

[shader("callable")]
void main(inout MaterialShaderPayload payload) {
    Parameters params = *(Parameters*)pushConst.materials[payload.materialId].paramBuffer;

    float falloff = params.FalloffAndLuminance.x;
    float iridescenceLuminance = params.FalloffAndLuminance.y;
    float emissionLuminance = params.FalloffAndLuminance.z;

    float pbrFactor0Smoothness = params.WPBRFactor.x;
    float pbrFactor0Metallic = params.WPBRFactor.y;
    float pbrFactor1Smoothness = params.WPBRFactor.z;
    float pbrFactor1Metallic = params.WPBRFactor.w;

    Sampler2D diffuseTexture = textures[params.diffuseTextureId];
    Sampler2D diffuse1Texture = textures[params.diffuse1TextureId];
    Sampler2D specularTexture = textures[params.specularTextureId];
    Sampler2D normalTexture = textures[params.normalTextureId];
    Sampler2D reflectionTexture = textures[params.reflectionTextureId];
    Sampler2D emissionTexture = textures[params.emissionTextureId];
    Sampler2D transparencyTexture = textures[params.transparencyTextureId];

    float4 diffuse = diffuseTexture.SampleLevel(payload.surface.uv, 0);
    float4 diffuse1 = diffuse1Texture.SampleLevel(payload.surface.uv2, 0);
    float4 specular = specularTexture.SampleLevel(payload.surface.uv, 0);
    float3 emission = emissionTexture.SampleLevel(payload.surface.uv, 0).xyz;
    float transparency = transparencyTexture.SampleLevel(payload.surface.uv, 0).x;

    float3 normal0 = UnpackNormalMapSafe(payload.surface.basis, normalTexture.SampleLevel(payload.surface.uv, 0).xy);
    float3 normal1 = UnpackNormalMapSafe(payload.surface.basis, normalTexture.SampleLevel(payload.surface.uv2, 0).xy);

    float blendFactor = saturate(payload.surface.vertexColor.x * diffuse.w * diffuse1.w);
    float3 worldNormal = normalize(lerp(normal0, normal1, blendFactor));

//    float3 cameraPosition = pushConst.viewInverse._m30_m31_m32;
//    float3 iridescenceView = normalize(cameraPosition - payload.hit.world_position); // w components from normals

    float2 cameraBias = calculateCameraBias(0, 0, int2(0, 0));

    float iridescenceNdotV0 = dot(normal0, payload.hit.view_direction);
    float iridescenceNdotV1 = dot(payload.surface.basis.normal, payload.hit.view_direction);

    float fresnel0 = pow(1.0 - abs(iridescenceNdotV0), falloff);
    float fresnel1 = pow(1.0 - abs(iridescenceNdotV1), falloff);

    float2 iridescenceUV0 = float2(params.IridescenceOffset.x, iridescenceNdotV0 + params.IridescenceOffset.z * cameraBias.x);
    float2 iridescenceUV1 = float2(params.IridescenceOffset.y, iridescenceNdotV1 + params.IridescenceOffset.w * cameraBias.y);

    float3 iridescence0 = reflectionTexture.SampleLevel(iridescenceUV0, 0).xyz * fresnel0;
    float3 iridescence1 = reflectionTexture.SampleLevel(iridescenceUV1, 0).xyz * fresnel1;

    float iridescenceFresnel = fresnel0 * fresnel1;
    float3 iridescence = iridescence0 * iridescence1;

    float4 diffuseTotal = diffuse + float4(iridescence, 0.0f);
    float3 diffuseBlended = pow(abs(lerp(diffuseTotal.xyz, diffuse1.xyz, blendFactor)), 2.2f);

    MaterialParameters materialParams;
    //LoadAlbedoTexture(materialParams, diffuse.xyz);
    LoadPRM(materialParams, specular);

    float pbrFactor0Roughness = SmoothnessToRoughness(pbrFactor0Smoothness);
    float pbrFactor1Roughness = SmoothnessToRoughness(pbrFactor1Smoothness);

    //float3 textureF0 = lerp(materialParams.specular, diffuseTotal.xyz, materialParams.metallic);
    //float3 pbrFactor0F0 = lerp(0.1, diffuseTotal.xyz, pbrFactor0Metallic);
    //float3 pbrFactor1F0 = lerp(0.1, diffuse1.xyz, pbrFactor1Metallic);

    float iridescenceSpecular = lerp(materialParams.specular, 0.1, iridescenceFresnel);
    //float3 iridescenceF0 = lerp(textureF0, pbrFactor0F0, iridescenceFresnel);
    float iridescenceRoughness = lerp(materialParams.roughness, pbrFactor0Roughness, iridescenceFresnel);
    float iridescenceMetallic = lerp(materialParams.metallic, pbrFactor0Metallic, iridescenceFresnel);

    float totalSpecular = lerp(iridescenceSpecular, 0.1, blendFactor);
    //float3 totalF0 = lerp(iridescenceF0, pbrFactor1F0, blendFactor);
    float totalRoughness = lerp(iridescenceRoughness, pbrFactor1Roughness, blendFactor);
    float totalMetallic = lerp(iridescenceMetallic, pbrFactor1Metallic, blendFactor);

    float transparencyFromTexture = transparency < diffuseTotal.w ? diffuseTotal.w : 0.0f;
    float transparencyFromMatAnim = abs(diffuseTotal.w) < (9.99999997e-007 + ambient_color.y) ? 0 : saturate(transparencyFromTexture + 2 * ambient_color.x - 1);

    LightingParameters lightingParams;
    lightingParams.hit = payload.hit;
    lightingParams.world_normal = normal0;
    LoadAlbedoTexture(lightingParams.material, diffuseBlended);
    lightingParams.material.specular = totalSpecular;
    lightingParams.material.roughness = totalRoughness;
    lightingParams.material.metallic = totalMetallic;
    lightingParams.material.cavity = 0.0f;

    payload.result.contribution = (iridescence * (1 - blendFactor) * iridescenceLuminance + emission * (1 - blendFactor) * emissionLuminance) * transparencyFromMatAnim;

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
