#pragma once
#include "common.slang"
#include <shading/ggx.slang>
#include <shading/math.slang>
#include <shading/optics.slang>
#include <shading/sampling.slang>

namespace bsdf::pbr {
    struct MaterialParameters {
        float3 normal;
        float3 albedo;
        float specular;
        float roughness;
        float metallic;
        float cavity;
    };

    struct PBRBSDFContext {
        MaterialParameters material;
        Basis local_basis;
        float3 V;
        float NdotV;

        __init(MaterialParameters mat, float3 viewDir) {
            material = mat;
            local_basis = Basis(mat.normal);
            V = viewDir;
            NdotV = saturate(dot(V, material.normal));
        }
    };

    struct PBRBSDFEvaluationContext {
        PBRBSDFContext bsdfContext;
        float3 L;
        float3 H;
        float NdotL;
        float HdotL;
        float3 fresnel_color;

        __init(PBRBSDFContext ctx, float3 lightDir) {
            float3 f0 = lerp(ctx.material.specular, ctx.material.albedo, ctx.material.metallic);

            bsdfContext = ctx;
            L = lightDir;
            H = normalize(L + ctx.V);
            NdotL = saturate(dot(L, ctx.material.normal));
            HdotL = saturate(dot(H, L));
            fresnel_color = schlickFresnel(HdotL, f0, saturate(dot(f0, 16.5)));
        }
    };

    BSDFEvaluationResult evaluateDiffuseBSDF(PBRBSDFEvaluationContext ctx) {
        float fresnel = (1.0 - ctx.fresnel_color.x);
        float pdf = ctx.NdotL * PiRecip;

        return {
            ctx.bsdfContext.material.albedo * fresnel * (1.0 - ctx.bsdfContext.material.metallic) * pdf,
            pdf,
        };
    }

    BSDFEvaluationResult evaluateSpecularBSDF(PBRBSDFEvaluationContext ctx) {
        float NdotH = saturate(dot(ctx.H, ctx.bsdfContext.material.normal));

        if (NdotH == 0.0f || ctx.NdotL == 0.0f || ctx.bsdfContext.NdotV == 0.0f || ctx.HdotL == 0.0f) {
            return { float3(0.0f, 0.0f, 0.0f), 0.0f };
        }

        float3 localH = convertToBasis(ctx.bsdfContext.local_basis, ctx.H);
        float3 localV = convertToBasis(ctx.bsdfContext.local_basis, ctx.bsdfContext.V);
        float3 localL = convertToBasis(ctx.bsdfContext.local_basis, ctx.L);

        float D = hvd_ggx_eval(1.0f / ctx.bsdfContext.material.roughness, localH);
        float G1 = 0.0f, G2 = 0.0f;
        ggx_smith_shadow_mask(G1, G2, localV, localL, ctx.bsdfContext.material.roughness);
        float pdf = G1 * D * 0.25f / (ctx.bsdfContext.NdotV * NdotH);

        return {
            ctx.fresnel_color * G2 * pdf,
            pdf,
        };
    }

    float3 sampleDiffuseBSDF(PBRBSDFContext ctx, float2 xis) {
        float3 local_direction = cosineSampleHemisphere(xis.x, xis.y);

        return convertFromBasis(ctx.local_basis, local_direction);
    }

    float3 sampleSpecularFacetNormal(PBRBSDFContext ctx, float2 xis) {
        float3 localV = convertToBasis(ctx.local_basis, ctx.V);
        float3 facetN = hvd_ggx_sample_vndf(localV, ctx.material.roughness, xis);

        return convertFromBasis(ctx.local_basis, facetN);
    }

    float3 sampleSpecularBSDFReflection(PBRBSDFContext ctx, float2 xis) {
        return reflect(-ctx.V, sampleSpecularFacetNormal(ctx, xis));
    }

    float3 sampleSpecularBSDFRefraction(PBRBSDFContext ctx, float eta, float2 xis) {
        return refract(-ctx.V, sampleSpecularFacetNormal(ctx, xis), eta);
    }

    float specularProbability(PBRBSDFContext ctx) {
        //    float f0 = lerp(parameters.material.specular, 1.0f, parameters.material.metallic);
        return lerp(schlickFresnel(ctx.NdotV, ctx.material.specular, saturate(dot(ctx.material.specular, 16.5))), 1.0f, ctx.material.metallic);
    }

    BSDFEvaluationResult evaluateBSDF(PBRBSDFContext bsdfContext, float3 lightDir) {
        PBRBSDFEvaluationContext ctx = PBRBSDFEvaluationContext(bsdfContext, lightDir);

        BSDFEvaluationResult diffuse = evaluateDiffuseBSDF(ctx);
        BSDFEvaluationResult specular = evaluateSpecularBSDF(ctx);

        float3 radiance = max(0.0, diffuse.color) + max(0.0, specular.color);

        return {
            radiance,
            lerp(diffuse.pdf, specular.pdf, specularProbability(bsdfContext)),
        };
    }

    float3 sampleBSDF(PBRBSDFContext ctx, float3 xis) {
        if (xis.z > specularProbability(ctx)) {
            return sampleDiffuseBSDF(ctx, xis.xy);
        }
        else {
            return sampleSpecularBSDFReflection(ctx, xis.xy);
        }
    }
}

