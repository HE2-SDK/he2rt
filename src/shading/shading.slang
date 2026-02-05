#pragma once
#include "shader-types.slang"
#include "material.slang"

struct ShadingResult {
    float3 contribution;
    float3 throughput;
    float3 nextDirection;
};

struct ShadowPayload {
    bool isMiss;
};

[shader("miss")]
void rmissShadowMain(inout ShadowPayload payload) {
    payload.isMiss = true;
}

bool testShadow(float3 worldPosition, float3 worldNormal, float3 lightDirection) {
    RayDesc ray;
    ray.Origin = worldPosition + worldNormal * 0.001;
    ray.Direction = lightDirection;
    ray.TMin = 0.001;
    ray.TMax = INFINITE;

    ShadowPayload payload = ShadowPayload(false);

    if (USE_SER) {
        HitObject hit = HitObject::TraceRay(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 1, ray, payload);

        return hit.IsHit();
    }
    else {
        TraceRay(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 1, ray, payload);

        return !payload.isMiss;
    }
}

float3 calculateSunlightContribution(bsdf::pbr::PBRBSDFContext ctx, float3 position) {
    float3 sunlight_dir = normalize(float3(1.0f, 1.0f, 1.0f));
    float3 sunlight_color = float3(1.0f, 1.0f, 1.0f);

    if (dot(ctx.material.normal, sunlight_dir) <= 0.0f || testShadow(position, ctx.material.normal, sunlight_dir))
        return float3(0, 0, 0);

    BSDFEvaluationResult sunlight_result = bsdf::pbr::evaluateBSDF(ctx, sunlight_dir);

    return sunlight_result.color * sunlight_color;
}

float3 calculateMaterialContribution(bsdf::pbr::PBRBSDFContext ctx, float3 emission, float3 position) {
    float3 contribution = emission;

    // Apply sunlight
    contribution += calculateSunlightContribution(ctx, position);

    return contribution;
}
