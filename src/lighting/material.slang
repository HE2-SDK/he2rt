#pragma once
#include "sonic-team-light.slang"

float SmoothnessToRoughness(float smoothness) {
    return max(0.01, 1.0 - smoothness);
}

void LoadPRM(inout MaterialParameters parameters, float4 prm) {
    parameters.specular = prm.x;
    parameters.roughness = SmoothnessToRoughness(prm.y);
    parameters.metallic = prm.z;
    parameters.cavity = prm.w;
}

void LoadAlbedoTexture(inout MaterialParameters parameters, float3 abd) {
    parameters.albedo = abd;
}

void LoadPRMTexture(inout MaterialParameters parameters, float4 prm) {
    prm.x *= 0.25;
    LoadPRM(parameters, prm);
}

float3 EvaluateMaterial(LightingParameters parameters) {
    BSDFEvaluationContext ctx = BSDFEvaluationContext(parameters);

    BSDFEvaluationResult sunlight_diffuse = EvaluateDiffuseBSDF(ctx);
    BSDFEvaluationResult sunlight_specular = EvaluateSpecularBSDF(ctx, false);//parameters.shading_model.type == ShadingModelType_AnisotropicReflection);

    float3 radiance = max(0.0, sunlight_diffuse.color) + max(0.0, sunlight_specular.color);

    return radiance * parameters.hit.light_color * parameters.hit.shadow;
}

//float3 SampleMaterial(LightingParameters parameters, float3 randomness) {
//    BSDFSamplingContext ctx = BSDFSamplingContext(parameters, randomness.xy);
//
//    if (randomness.z <= 
//
//}