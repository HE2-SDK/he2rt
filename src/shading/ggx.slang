#pragma once
#include "math.slang"

float hvd_ggx_eval(const float2 invRoughness,
                   const float3 h) {
    const float x = h.x * invRoughness.x;
    const float y = h.y * invRoughness.y;
    const float aniso = x * x + y * y;
    const float f = aniso + h.z * h.z;

    return PiRecip * invRoughness.x * invRoughness.y * h.z / (f * f);
}

float3 hvd_ggx_sample_vndf(float3 k, float2 roughness, float2 xi) {
    const float3 v = normalize(float3(k.x * roughness.x, k.y * roughness.y, k.z));

    const float3 t1 = (v.z < 0.99999f) ? normalize(cross(v, float3(0.0f, 0.0f, 1.0f))) : float3(1.0f, 0.0f, 0.0f);
    const float3 t2 = cross(t1, v);

    const float a = 1.0f / (1.0f + v.z);
    const float r = sqrt(xi.x);

    const float phi = (xi.y < a) ? xi.y / a * Pi : Pi + (xi.y - a) / (1.0f - a) * Pi;
    float sp = sin(phi);
    float cp = cos(phi);
    const float p1 = r * cp;
    const float p2 = r * sp * ((xi.y < a) ? 1.0f : v.z);

    float3 h = p1 * t1 + p2 * t2 + sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

    h.x *= roughness.x;
    h.y *= roughness.y;
    h.z = max(0.0f, h.z);
    return normalize(h);
}

float smith_shadow_or_mask(const float3 k, const float2 roughness) {
    float kz2 = k.z * k.z;
    if (kz2 == 0.0f) {
        return 0.0f; // Totally shadowed
    }
    const float ax = k.x * roughness.x;
    const float ay = k.y * roughness.y;
    const float inv_a2 = (ax * ax + ay * ay) / kz2;

    return 2.0f / (1.0f + sqrt(1.0f + inv_a2));
}

float ggx_smith_shadow_mask(out float G1, out float G2, const float3 k1, const float3 k2, const float2 roughness) {
    G1 = smith_shadow_or_mask(k1, roughness);
    G2 = smith_shadow_or_mask(k2, roughness);

    return G1 * G2;
}
