#pragma once

static const float Pi = radians(180.0);
static const float PiRecip = 1 / Pi;
static const float Tau = radians(360.0);

struct Basis {
    float3 normal;
    float3 tangent;
    float3 binormal;
};

Basis OrthonormalBasis(float3 n) {
    if (n.z < -0.999999) {
        return { n, float3(0, -1, 0), float3(-1, 0, 0) };
    }
    else {
        float a = 1. / (1. + n.z);
        float b = -n.x * n.y * a;

        return {
            n,
            normalize(float3(1. - n.x * n.x * a, b, -n.x)),
            normalize(float3(b, 1. - n.y * n.y * a, -n.y)),
        };
    }
}

float3 ConvertFromBasis(Basis basis, float3 v) {
    return v.x * basis.tangent + v.y * basis.binormal + v.z * basis.normal;
}

float3 ConvertToBasis(Basis basis, float3 v) {
    return float3(dot(v, basis.tangent), dot(v, basis.binormal), dot(v, basis.normal));
}
