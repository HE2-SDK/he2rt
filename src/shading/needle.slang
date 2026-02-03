#pragma once
#include "bsdf/pbr.slang"

struct NeedlePRMParameters {
    float specular;
    float roughness;
    float metallic;
    float cavity;
};

struct NeedleWPRMParameters {
    float roughness;
    float metallic;
};

float smoothnessToRoughness(float smoothness) {
    return max(0.01, 1.0 - smoothness);
}

float3 unpackNormalMapTexture(float2 map) {
    float2 scaled = map * 2.0 - 1.0;

    return float3(scaled, sqrt(1.0 - saturate(dot(scaled, scaled))));
}

NeedlePRMParameters unpackUnscaledSpecularTexture(float4 texture) {
    return { texture.x, smoothnessToRoughness(texture.y), texture.z, texture.w };
}

NeedlePRMParameters unpackSpecularTexture(float4 texture) {
    return { texture.x * 0.25, smoothnessToRoughness(texture.y), texture.z, texture.w };
}

NeedlePRMParameters unpackPBRFactor(float4 pbrFactor) {
    return { pbrFactor.x, smoothnessToRoughness(pbrFactor.y), pbrFactor.z, 1.0f };
}

NeedleWPRMParameters[2] unpackWPBRFactor(float4 wpbrFactor) {
    return {
        { smoothnessToRoughness(wpbrFactor.x), wpbrFactor.y },
        { smoothnessToRoughness(wpbrFactor.z), wpbrFactor.w },
    };
}

void loadNeedlePRM(inout bsdf::pbr::MaterialParameters material, NeedlePRMParameters prm) {
    material.specular = prm.specular;
    material.roughness = prm.roughness;
    material.metallic = prm.metallic;
    material.cavity = prm.cavity;
}

void loadNeedleWPRM(inout bsdf::pbr::MaterialParameters material, NeedleWPRMParameters prm) {
    material.roughness = prm.roughness;
    material.metallic = prm.metallic;
}

struct NormalMapSampler {
    Sampler2D sampler;

    float3 SampleLevel(float2 uv, float lod) {
        return unpackNormalMapTexture(sampler.SampleLevel(uv, lod).xy);
    }
};

struct NormalMapSamplerWithBasis {
    NormalMapSampler sampler;
    Basis basis;

    __init(Sampler2D s, Basis b) {
        sampler = { s };
        basis = b;
    }

    float3 SampleLevel(float2 uv, float lod) {
        float3 result = normalize(convertFromBasis(basis, sampler.SampleLevel(uv, lod)));

        bool3 nan_check = result != result;
        
        if (nan_check.x | nan_check.y | nan_check.z) {
            return basis.normal;
        }
        else {
            return result;
        }
    }
};

struct SpecularSampler {
    Sampler2D sampler;

    NeedlePRMParameters SampleLevel(float2 uv, float lod) {
        return unpackSpecularTexture(sampler.SampleLevel(uv, lod));
    }
};
