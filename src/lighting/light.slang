#pragma once
#include "math.slang"
#include "normals.slang"

struct HitParameters {
    float3 world_position;
    float3 view_direction;
    float3 light_direction;
    float3 light_color;
};

struct MaterialParameters {
    float3 albedo;
    float specular;
    float roughness;
    float metallic;
    float cavity;
};

struct LightingParameters {
    HitParameters hit;
    MaterialParameters material;
    float3 world_normal;

    //ShadingModel shading_model;

    // Geometry properties

    // float2 screen_position;
    // uint2 pixel_position;
    // uint2 tile_position;

    // float depth;
    // float view_distance;
    // float3 anisotropic_tangent;
    // float3 anisotropic_binormal;

    // float3 shadow_position;
    // float shadow_depth;
    //
    // Surface properties
    //
    // 
    // float3 emission;
    // float3 sss_param;
    // float2 anisotropy;
    // 
    // bool approximate_env_brdf;
    // 
    // float lightfield_ao;
    // 
    // TypedOcclusion typed_occlusion;
    // LightScatteringColors light_scattering_colors;
};

//void SampleCDRF(BSDFEvaluationContext ctx, float3 light_direction, float shadow, inout float3 result)
//{
//	#ifdef enable_ssss
//		return;
//	#endif
//
//	float cos = dot(light_direction, ctx.world_normal);
//
//	float t = saturate(cos * 0.5 + 0.5);
//	      t = cos - t * (1.0 - shadow);
//	      t = saturate(t * 0.5 + 0.5);
//
//	result = SampleTextureLevel(s_Common_CDRF, float3(t, ctx.sss_param.xy), 0).xyz;
//}

//float ShlickFresnelFactor(float cos_theta, float power = 5.0) {
//    return pow(1.0 - cos_theta, power);
//}
//
//__generic<int N> vector<float, N> FresnelSchlick(vector<float, N> f0, float cos_theta, vector<float, N> f90 = saturate(dot(f0, 16.5)), float power = 5.0) {
//    return lerp(f0, f90, ShlickFresnelFactor(cos_theta, power));
//}

__generic<int N> vector<float, N> FresnelSchlick(vector<float, N> f0, float cos_theta) {
    float p = pow(1.0 - cos_theta, 5.0);
	float f90 = saturate(dot(f0, 16.5));

	return lerp(f0, f90, p );
}

// // GGX normal distribution function
// float NdfGGX(float3 halfway, float3 normal, float roughness) {
//     float cos_halfway_normal = saturate(dot(halfway, normal));
// 
//     float alpha = roughness * roughness;
// 
//     float denom = (cos_halfway_normal * alpha - cos_halfway_normal) * cos_halfway_normal + 1;
//     return alpha / (Pi * denom * denom);
// }
// 
// float DGGXAniso(float3 halfway, float3 normal, float3 tangent, float3 binormal, float roughness, float2 anisotropy) {
//     anisotropy *= roughness * roughness;
//     anisotropy = max(anisotropy, 0.000001);
//     float a2 = anisotropy.x * anisotropy.y;
// 
//     float3 v = float3(
//         dot(halfway, tangent) * anisotropy.x,
//         dot(halfway, binormal) * anisotropy.y,
//         dot(halfway, normal) * a2
//     );
// 
//     float v2 = dot(v, v);
//     float w2 = a2 / v2;
// 
//     return (a2 * w2 * w2) / Pi;
// }
// 
// float VisSchlick(float roughness, float cos_view_normal, float cos_light_normal) {
//     float r = roughness + 1;
//     float k = (r * r) / 8;
//     float schlick_v = cos_view_normal * (1 - k) + k;
//     float schlick_l = cos_light_normal * (1 - k) + k;
//     return 0.25 / (schlick_v * schlick_l);
// }

float hvd_ggx_eval(const float2 invRoughness,
                   const float3 h) { // == float3(dot(tangent, h), dot(bitangent, h), dot(normal, h))
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

//////////////////////////////////////////////////
// Full color functions

struct BSDFEvaluationContext {
    LightingParameters parameters;

    float3 halfway_direction;
    float cos_view_normal;
    float cos_light_normal;
    float cos_halfway_light;
    float3 fresnel_color;

    __init(LightingParameters params) {
        float3 f0 = lerp(params.material.specular, params.material.albedo, params.material.metallic);

        parameters = params;

        halfway_direction = normalize(params.hit.light_direction + params.hit.view_direction);
        cos_view_normal = saturate(dot(params.hit.view_direction, params.world_normal));
        cos_light_normal = saturate(dot(params.hit.light_direction, params.world_normal));
        cos_halfway_light = saturate(dot(halfway_direction, params.hit.light_direction));
        fresnel_color = FresnelSchlick(f0, cos_halfway_light);
    }
};

struct BSDFEvaluationResult {
    float3 color;
    float pdf;
};

// float3 EvaluateDiffuseBSDF(BSDFEvaluationContext ctx) {
// //	if (ctx.shading_model.type == ShadingModelType_SSS)
// //	{
// //		SampleCDRF(ctx, ctx.hit.light_direction, ctx.shadow, cos_light_normal);
// //	}
// 
//     return ctx.parameters.hit.light_color * ctx.cos_light_normal * (1.0 - ctx.fresnel_color.x) * (1.0 - ctx.parameters.material.metallic) * ctx.parameters.hit.shadow;
// }
// 
// float3 EvaluateSpecularBSDF(BSDFEvaluationContext ctx, bool enable_anisotropy) {
//     float distribution;
// //    if (enable_anisotropy)
// //    {
// //        distribution = DGGXAniso(
// //            halfway_direction,
// //            ctx.world_normal,
// //            ctx.anisotropic_tangent,
// //            ctx.anisotropic_binormal,
// //            ctx.roughness,
// //            ctx.anisotropy
// //        );
// //    }
// //    else
// //    {
//         distribution = NdfGGX(ctx.halfway_direction, ctx.parameters.world_normal, ctx.parameters.material.roughness);
// //    }
// 
//     float visibility = VisSchlick(ctx.parameters.material.roughness, ctx.cos_view_normal, ctx.cos_light_normal);
// 
//     return ctx.parameters.hit.light_color * ctx.cos_light_normal * saturate(ctx.fresnel_color * distribution * visibility) * ctx.parameters.hit.shadow;
// }

BSDFEvaluationResult EvaluateDiffuseBSDF(BSDFEvaluationContext ctx) {
    //	if (ctx.shading_model.type == ShadingModelType_SSS)
    //	{
    //		SampleCDRF(ctx, ctx.hit.light_direction, ctx.shadow, cos_light_normal);
    //	}
    float fresnel = (1.0 - ctx.fresnel_color.x);
    float pdf = ctx.cos_light_normal * PiRecip;

    return {
        ctx.parameters.material.albedo * fresnel * (1.0 - ctx.parameters.material.metallic) * pdf,
        pdf,
    };
}

BSDFEvaluationResult EvaluateSpecularBSDF(BSDFEvaluationContext ctx, bool enable_anisotropy) {
    float cos_halfway_normal = saturate(dot(ctx.halfway_direction, ctx.parameters.world_normal));

    if (cos_halfway_normal == 0.0f || ctx.cos_light_normal == 0.0f || ctx.cos_view_normal == 0.0f || ctx.cos_halfway_light == 0.0f) {
        return { float3(0.0f, 0.0f, 0.0f), 0.0f };
    }

    Basis local_basis = OrthonormalBasis(ctx.parameters.world_normal);

    float3 localH = ConvertToBasis(local_basis, ctx.halfway_direction);
    float3 localV = ConvertToBasis(local_basis, ctx.parameters.hit.view_direction);
    float3 localL = ConvertToBasis(local_basis, ctx.parameters.hit.light_direction);

    float D = hvd_ggx_eval(1.0f / ctx.parameters.material.roughness, localH);
    float G1 = 0.0f, G2 = 0.0f;
    ggx_smith_shadow_mask(G1, G2, localV, localL, ctx.parameters.material.roughness);
    float pdf = G1 * D * 0.25f / (ctx.cos_view_normal * cos_halfway_normal);

    return {
        ctx.fresnel_color * G2 * pdf,
        pdf,
    };
}

inline float3 cosineSampleHemisphere(float r1, float r2) {
    float r = sqrt(r1);
    float phi = 2 * Pi * r2;
    float3 dir;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.F, 1.F - dir.x * dir.x - dir.y * dir.y));
    return dir;
}

struct BSDFSamplingContext {
    LightingParameters parameters;
    Basis local_basis;
    float2 randomness;

    __init(LightingParameters params, float2 rands) {
        parameters = params;
        randomness = rands;
        local_basis = OrthonormalBasis(params.world_normal);
    }
}

float3 SampleDiffuseBSDF(BSDFSamplingContext ctx) {
    float3 local_direction = cosineSampleHemisphere(ctx.randomness.x, ctx.randomness.y);

    return ConvertFromBasis(ctx.local_basis, local_direction);
}

float3 SampleSpecularBSDF(BSDFSamplingContext ctx) {
    float3 localV = ConvertToBasis(ctx.local_basis, ctx.parameters.hit.view_direction);
    float3 facetN = hvd_ggx_sample_vndf(localV, ctx.parameters.material.roughness, ctx.randomness);

    return reflect(-ctx.parameters.hit.view_direction, ConvertFromBasis(ctx.local_basis, facetN));
}
