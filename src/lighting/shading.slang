#pragma once
#include "shader-types.slang"
#include "material.slang"

MaterialShaderPayload::Result PerformShading(LightingParameters parameters) {
    MaterialShaderPayload::Result result;

    result.contribution = emission * ambient_color * luminance;

    if (dot(parameters.hit.light_direction, parameters.world_normal) > 0.0f) {
        BSDFEvaluationResult sunlight = EvaluateMaterial(parameters);
        result.contribution += sunlight.color;
    }

    result.nextDirection = SampleMaterial(parameters, payload.randomness);
    parameters.hit.light_direction = result.nextDirection;

    BSDFEvaluationResult sampled = EvaluateMaterial(parameters);

    float3 throughput = sampled.color / sampled.pdf;
    result.throughput = any(isnan(throughput)) || sampled.pdf < 0.0001f ? 0.0f : throughput;

    return result;
}
