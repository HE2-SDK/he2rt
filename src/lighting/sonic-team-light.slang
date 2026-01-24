#include "math.slang"
#include "light.slang"

// void SampleCDRF(BSDFEvaluationContext ctx, float3 light_direction, float shadow, inout float3 result)
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
// }

// GGX normal distribution function
float NdfGGX(float3 halfway, float3 normal, float roughness) {
    float cos_halfway_normal = saturate(dot(halfway, normal));

    float alpha = roughness * roughness;

    float denom = (cos_halfway_normal * alpha - cos_halfway_normal) * cos_halfway_normal + 1;
    return alpha / (Pi * denom * denom);
}

float DGGXAniso(float3 halfway, float3 normal, float3 tangent, float3 binormal, float roughness, float2 anisotropy) {
    anisotropy *= roughness * roughness;
    anisotropy = max(anisotropy, 0.000001);
    float a2 = anisotropy.x * anisotropy.y;

    float3 v = float3(
        dot(halfway, tangent) * anisotropy.x,
        dot(halfway, binormal) * anisotropy.y,
        dot(halfway, normal) * a2
    );

    float v2 = dot(v, v);
    float w2 = a2 / v2;

    return (a2 * w2 * w2) / Pi;
}

float VisSchlick(float roughness, float cos_view_normal, float cos_light_normal) {
    float r = roughness + 1;
    float k = (r * r) / 8;
    float schlick_v = cos_view_normal * (1 - k) + k;
    float schlick_l = cos_light_normal * (1 - k) + k;
    return 0.25 / (schlick_v * schlick_l);
}

BSDFEvaluationResult EvaluateSTDiffuseBSDF(BSDFEvaluationContext ctx) {
//	if (ctx.shading_model.type == ShadingModelType_SSS)
//	{
//		SampleCDRF(ctx, ctx.hit.light_direction, ctx.shadow, cos_light_normal);
//	}

    return {
        ctx.cos_light_normal * (1.0 - ctx.fresnel_color.x) * (1.0 - ctx.parameters.material.metallic) * ctx.parameters.material.albedo / Pi,
        1,
    };
}

BSDFEvaluationResult EvaluateSTSpecularBSDF(BSDFEvaluationContext ctx, bool enable_anisotropy) {
    float distribution;
//    if (enable_anisotropy)
//    {
//        distribution = DGGXAniso(
//            halfway_direction,
//            ctx.world_normal,
//            ctx.anisotropic_tangent,
//            ctx.anisotropic_binormal,
//            ctx.roughness,
//            ctx.anisotropy
//        );
//    }
//    else
//    {
        distribution = NdfGGX(ctx.halfway_direction, ctx.parameters.world_normal, ctx.parameters.material.roughness);
//    }

    float visibility = VisSchlick(ctx.parameters.material.roughness, ctx.cos_view_normal, ctx.cos_light_normal);

    return {
        ctx.cos_light_normal * saturate(ctx.fresnel_color * distribution * visibility),
        1,
    };
}
