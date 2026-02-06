#pragma once
#include "bsdf/pbr.slang"
#include "surface.slang"

struct MaterialShaderResult {
    bsdf::pbr::MaterialParameters material;
    float3 emission = float3(0.0f, 0.0f, 0.0f);
};

struct MaterialShaderPayload {
    uint32_t materialId;
    SurfaceParameters surface;
    float3 V;
    MaterialShaderResult result;
};
