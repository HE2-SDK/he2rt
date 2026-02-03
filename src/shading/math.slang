#pragma once

static const float Pi = radians(180.0);
static const float PiRecip = 1 / Pi;
static const float Tau = radians(360.0);

struct Basis {
    float3 normal;
    float3 tangent;
    float3 binormal;

    __init(float3 nrm, float3 tng, float3 bin) {
        normal = nrm;
        tangent = tng;
        binormal = bin;
    }

    __init(float3 nrm, float3 tng) {
        normal = nrm;
        tangent = tng;
        binormal = normalize(cross(nrm, tng));
    }

    __init(float3 nrm) {
        if (nrm.z < -0.999999) {
            normal = nrm;
            tangent = float3(0, -1, 0);
            binormal = float3(-1, 0, 0);
        }
        else {
            float a = 1. / (1. + nrm.z);
            float b = -nrm.x * nrm.y * a;

            normal = nrm;
            tangent = normalize(float3(1. - nrm.x * nrm.x * a, b, -nrm.x));
            binormal = normalize(float3(b, 1. - nrm.y * nrm.y * a, -nrm.y));
        }
    }
};

float3 convertFromBasis(Basis basis, float3 v) {
    return v.x * basis.tangent + v.y * basis.binormal + v.z * basis.normal;
}

float3 convertToBasis(Basis basis, float3 v) {
    return float3(dot(v, basis.tangent), dot(v, basis.binormal), dot(v, basis.normal));
}
