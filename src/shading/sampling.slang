#pragma once
#include "math.slang"

inline float3 cosineSampleHemisphere(float r1, float r2) {
    float r = sqrt(r1);
    float phi = 2 * Pi * r2;
    float3 dir;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.F, 1.F - dir.x * dir.x - dir.y * dir.y));
    return dir;
}
