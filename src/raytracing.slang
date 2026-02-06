#include "random.slang"
#include "shader-types.slang"
#include <shading/shading.slang>
#include <shading/surface.slang>

#define MISS_DEPTH 1000

static int MAX_DEPTH = 4;
static uint SPP = 1;

using namespace he2rt::shaders;

struct PathTracingState {
    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    float3 throughput = float3(1.0f, 1.0f, 1.0f);
};

struct HitResult {
    float4x3 objectToWorld;
    float2 barycentrics;
    uint modelId;
    uint meshId;
    uint triangleId;
    bool isHit;
};

float3 offsetRay(in float3 p, in float3 n)
{
    // Smallest epsilon that can be added without losing precision is 1.19209e-07, but we play safe
    const float epsilon = 1.0f / 65536.0f; // Safe epsilon

    const float magnitude = length(p);
    const float offset = epsilon * magnitude;
    // multiply the direction vector by the smallest offset
    const float3 offsetVector = n * offset;
    // add the offset vector to the starting point
    const float3 offsetPoint = p + offsetVector;

    return offsetPoint;
}

HitResult traceRay(RayDesc ray, int depth) {
    HitResult result;

    if (USE_SER) {
        HitObject hit = HitObject::TraceRay(tlas, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, result);

        result.isHit = hit.IsHit();

        uint materialId = 0;

        if (result.isHit) {
            result.objectToWorld = hit.GetObjectToWorld();
            result.barycentrics = hit.GetAttributes<BuiltInTriangleIntersectionAttributes>().barycentrics;
            result.modelId = hit.GetInstanceID();
            result.meshId = hit.GetGeometryIndex();
            result.triangleId = hit.GetPrimitiveIndex();

            materialId = pushConst.models[result.modelId].meshes[result.meshId].materialId;
        }
        //else {
        //    result.modelId = 0;
        //    result.meshId = 0;
        //}

        //uint materialId =
        //uint hint = (result.modelId << 8) | (result.meshId << 1) | ((depth + 1 == MAX_DEPTH) ? 1 : 0);
        uint hint = (materialId << 1) | ((depth + 1 == MAX_DEPTH) ? 1 : 0);

        ReorderThread(hit, hint, 16);
    }
    else {
        TraceRay(tlas, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, result);
    }

    return result;
}

float3 pathTrace(RayDesc ray, inout uint seed) {
    PathTracingState state = {};

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        const HitResult hit = traceRay(ray, depth);

        if (!hit.isHit) {
            break;
        }

        // Collect some basic information
        const float3 viewDirection = -ray.Direction;
        const float3 barycentrics = float3(1 - hit.barycentrics.x - hit.barycentrics.y, hit.barycentrics.x, hit.barycentrics.y);

        const Model model = pushConst.models[hit.modelId];
        const Mesh mesh = model.meshes[hit.meshId];

        const float3 position = getPosition(mesh, hit.triangleId, hit.objectToWorld, barycentrics);
        const SurfaceParameters surface = getSurfaceParameters(mesh, hit.triangleId, hit.objectToWorld, barycentrics);

        // Look up the material
        const uint materialId = mesh.materialId;
        const Material material = pushConst.materials[materialId];

        // Call the material shader
        MaterialShaderPayload materialShaderPayload = { materialId, surface, viewDirection };

        CallShader<MaterialShaderPayload>(material.shaderId, materialShaderPayload);

        bsdf::pbr::PBRBSDFContext ctx = bsdf::pbr::PBRBSDFContext(materialShaderPayload.result.material, viewDirection);

        // Add total contribution to radiance;
        state.radiance += calculateMaterialContribution(ctx, materialShaderPayload.result.emission, position) * state.throughput;

        const float3 sampledDir = bsdf::pbr::sampleBSDF(ctx, float3(rand(seed), rand(seed), rand(seed)));
        const BSDFEvaluationResult sampled = bsdf::pbr::evaluateBSDF(ctx, sampledDir);

        const float3 throughput = sampled.color / sampled.pdf;
        state.throughput *= any(isnan(throughput)) || sampled.pdf < 0.0001f ? 0.0f : throughput;

        // Russian roulette
        const float rrPcont = min(max(state.throughput.x, max(state.throughput.y, state.throughput.z)) + 0.001F, 0.95F);
        if (rand(seed) >= rrPcont)
            break;
        state.throughput /= rrPcont;

        ray.Origin = offsetRay(position, materialShaderPayload.result.material.normal);
        ray.Direction = sampledDir;
    }

    return state.radiance;
}

float3 pathTracePixel(inout uint seed) {
    const float2 launchID = (float2)DispatchRaysIndex().xy;
    const float2 launchSize = (float2)DispatchRaysDimensions().xy;

    const float2 subpixelJitter = pushConst.frame == 0 ? float2(0.5f, 0.5f) : float2(rand(seed), rand(seed));
    const float2 pixelCenter = launchID + subpixelJitter;
    const float2 clipCoords = pixelCenter / launchSize * 2.0 - 1.0;
    const float4 viewCoords = mul(pushConst.projInverse, float4(clipCoords, 1.0, 1.0));

    RayDesc ray;
    ray.Origin = mul(pushConst.viewInverse, float4(0.0, 0.0, 0.0, 1.0)).xyz;
    ray.Direction = mul(pushConst.viewInverse, float4(normalize(viewCoords.xyz), 0.0)).xyz;
    ray.TMin = 0.001;
    ray.TMax = 100000.0;

    float3 radiance = pathTrace(ray, seed);

    const float lum = dot(radiance, float3(0.212671F, 0.715160F, 0.072169F));
    const float fireflyClampThreshold = 10.0f;
    if (lum > fireflyClampThreshold) {
        radiance *= fireflyClampThreshold / lum;
    }

    return radiance;
}

[shader("raygeneration")]
void rgenMain() {
	const float2 launchID = (float2)DispatchRaysIndex().xy;
    const float2 launchSize = (float2)DispatchRaysDimensions().xy;

    uint seed = xxhash32(uint3(uint2(launchID.xy), pushConst.totalFrame));

    float3 color = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < SPP; i++) {
        color += pathTracePixel(seed);
    }

    color /= SPP;

    if (pushConst.frame > 0) {
        const float a = 1.0f / float(pushConst.frame + 1);
        const float3 old_color = outImage[int2(launchID)].xyz;
        outImage[int2(launchID)] = float4(lerp(old_color, color, a), 1.f);
    }
    else {
        outImage[int2(launchID)] = float4(color, 1.f);
    }
}

[shader("closesthit")]
void rchitMain(inout HitResult result, in BuiltInTriangleIntersectionAttributes attr) {
    result.isHit = true;
    result.objectToWorld = ObjectToWorld4x3();
    result.barycentrics = attr.barycentrics;
    result.modelId = InstanceID();
    result.meshId = GeometryIndex();
    result.triangleId = PrimitiveIndex();
}

[shader("miss")]
void rmissMain(inout HitResult result) {
    result.isHit = false;
}
