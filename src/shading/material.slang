#pragma once
#include "bsdf/pbr.slang"

struct SurfaceParameters {
    Basis basis;
    float2 uvs[4];
    float4 vertexColor;
};

struct MaterialShaderResult {
    bsdf::pbr::MaterialParameters material;
    float3 emission = float3(0.0f, 0.0f, 0.0f);
};

struct MaterialShaderPayload {
    uint32_t materialId;
    //    HitParameters hit;
    SurfaceParameters surface;
    float3 V;
    MaterialShaderResult result;
};
