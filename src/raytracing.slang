#include "shader-types.slang"
#include "random.slang"

#define MISS_DEPTH 1000

using namespace he2rt::shaders;

[[vk::push_constant]] ConstantBuffer<PushConstant> pushConst;
[[vk::binding(0, 0)]] Sampler2D textures[];
[[vk::binding(0, 1)]] RaytracingAccelerationStructure tlas;
[[vk::binding(1, 1)]] RWTexture2D<float4> outImage;

struct HitPayload {
	float3 color;
	float weight;
	int depth;
};

struct ShadowPayload {
    int depth;
};

struct TriangleAttributes<T> {
	T v0;
	T v1;
	T v2;
};

T getVertexAttribute<T>(Mesh mesh, uint16_t vertexIndex, uint offset) {
	return *(T*)(mesh.vertexBuffer + vertexIndex * mesh.vertexDataReflection.stride + offset);
}

TriangleAttributes<T> getTriangleAttributes<T>(Mesh mesh, uint triangleIndex, uint offset) {
	return {
		getVertexAttribute<T>(mesh, mesh.indexBuffer[triangleIndex * 3 + 0], offset),
		getVertexAttribute<T>(mesh, mesh.indexBuffer[triangleIndex * 3 + 1], offset),
		getVertexAttribute<T>(mesh, mesh.indexBuffer[triangleIndex * 3 + 2], offset),
	};
}

T getInterpolatedAttribute<T : IFloat>(Mesh mesh, uint triangleIndex, uint offset, float3 barycentrics) {
	TriangleAttributes<T> attrs = getTriangleAttributes<T>(mesh, triangleIndex, offset);

	return T(barycentrics.x) * attrs.v0 + T(barycentrics.y) * attrs.v1 + T(barycentrics.z) * attrs.v2;
}

float3 getInterpolatedDec3nAttribute(Mesh mesh, uint triangleIndex, uint offset, float3 barycentrics) {
	TriangleAttributes<int8_t4> attrs = getTriangleAttributes<int8_t4>(mesh, triangleIndex, offset);

    return float3(barycentrics.x) * float3(attrs.v0.xyz) / 511.0 + float3(barycentrics.y) * float3(attrs.v1.xyz) / 511.0 + float3(barycentrics.z) * float3(attrs.v2.xyz) / 511.0;
}

float2 getInterpolatedHalf2Attribute(Mesh mesh, uint triangleIndex, uint offset, float3 barycentrics) {
    TriangleAttributes<half2> attrs = getTriangleAttributes<half2>(mesh, triangleIndex, offset);

    return float2(barycentrics.x) * float2(attrs.v0) + float2(barycentrics.y) * float2(attrs.v1) + float2(barycentrics.z) * float2(attrs.v2);
}

bool testShadowSER(float3 worldPosition, float3 worldNormal, float3 lightDirection) {
    RayDesc ray;
    ray.Origin = worldPosition + worldNormal * 0.001;
    ray.Direction = normalize(lightDirection);
    ray.TMin = 0.001;
    ray.TMax = INFINITE;

    ShadowPayload payload;
    payload.depth = 0;

    HitObject hit = HitObject::TraceRay(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 1, ray, payload);

    return hit.IsHit();
}

float testShadow(float3 worldPosition, float3 worldNormal, float3 lightDirection) {
    RayDesc ray;
    ray.Origin = worldPosition + worldNormal * 0.001;
    ray.Direction = normalize(lightDirection);
	ray.TMin = 0.001;
	ray.TMax = INFINITE;

    ShadowPayload payload;
    payload.depth = 0;

    TraceRay(tlas, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 0, 1, ray, payload);

    return payload.depth != MISS_DEPTH ? 0.0 : 1.0;
}

HitParameters collectHitParameters(Mesh mesh, uint triangleId, float3 worldRayDirection, float3 barycentrics) {
    float3 pos = getInterpolatedAttribute<float3>(mesh, triangleId, mesh.vertexDataReflection.positionOffset, barycentrics);
    float3 viewDirection = -worldRayDirection;
    float3 lightDirection = normalize(float3(1.0f, 1.0f, 1.0f));

    return {
        pos,
        viewDirection,
        lightDirection,
        float3(1.0f, 1.0f, 1.0f),
    };
}

SurfaceParameters collectSurfaceParameters(Mesh mesh, uint triangleId, float4x3 objToWorld, float3 barycentrics) {
    float3 nrm = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.normalOffset, barycentrics);
    float3 tng = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.tangentOffset, barycentrics);
    float3 bin = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.binormalOffset, barycentrics);
    float2 uv = getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uvOffset, barycentrics);
    float2 uv2 = getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uv2Offset, barycentrics);

    return {
        normalize(mul(float4(nrm, 0.0), objToWorld).xyz),
        normalize(mul(float4(tng, 0.0), objToWorld).xyz),
        normalize(mul(float4(bin, 0.0), objToWorld).xyz),
        uv,
        uv2,
    };
}

float3 offsetRay(in float3 p, in float3 n)
{
    // Smallest epsilon that can be added without losing precision is 1.19209e-07, but we play safe
    const float epsilon = 1.0f / 65536.0f; // Safe epsilon

    float magnitude = length(p);
    float offset = epsilon * magnitude;
    // multiply the direction vector by the smallest offset
    float3 offsetVector = n * offset;
    // add the offset vector to the starting point
    float3 offsetPoint = p + offsetVector;

    return offsetPoint;
}

float3 pathTrace(RayDesc ray, inout uint seed) {
    PathTracingState state = {};

    for (int depth = 0; depth < 8; depth++) {
        HitPayload payload;
        payload.color = float3(0, 0, 0);
        payload.weight = 1;
        payload.depth = 0;

        HitObject hit = HitObject::TraceRay(tlas, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

        if (!hit.IsHit()) {
            return state.radiance;
        }
        else {
            BuiltInTriangleIntersectionAttributes attr = hit.GetAttributes<BuiltInTriangleIntersectionAttributes>();
            uint modelId = hit.GetInstanceID();
            uint geometryId = hit.GetGeometryIndex();
            uint triangleId = hit.GetPrimitiveIndex();
            float4x3 objToWorld = hit.GetObjectToWorld();

            Model model = pushConst.models[modelId];
            Mesh mesh = model.meshes[geometryId];

            // Collect some basic information
            float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
            HitParameters hitParameters = collectHitParameters(mesh, triangleId, ray.Direction, barycentrics);
            SurfaceParameters surfaceParameters = collectSurfaceParameters(mesh, triangleId, objToWorld, barycentrics);

            // Look up the material
            uint materialId = mesh.materialId;
            Material material = pushConst.materials[materialId];

            // Call the material shader
            MaterialShaderPayload materialShaderPayload = { materialId, hitParameters, surfaceParameters, float3(rand(seed), rand(seed), rand(seed)) };

            CallShader<MaterialShaderPayload>(material.shaderId, materialShaderPayload);

            if (!testShadowSER(hitParameters.world_position, surfaceParameters.basis.normal, hitParameters.light_direction)) {
                state.radiance += materialShaderPayload.contribution * state.throughput;
            }

            state.throughput *= materialShaderPayload.throughput;

            // Russian roulette
            float rrPcont = min(max(state.throughput.x, max(state.throughput.y, state.throughput.z)) + 0.001F, 0.95F);
            if (rand(seed) >= rrPcont)
                break;
            state.throughput /= rrPcont;

            ray.Origin = offsetRay(hitParameters.world_position, surfaceParameters.basis.normal);
            ray.Direction = materialShaderPayload.nextDirection;
        }
    }

    return state.radiance;
}

float3 pathTracePixel(inout uint seed) {
    float2 launchID = (float2)DispatchRaysIndex().xy;
    float2 launchSize = (float2)DispatchRaysDimensions().xy;

    const float2 subpixelJitter = pushConst.frame == 0 ? float2(0.5f, 0.5f) : float2(rand(seed), rand(seed));
    const float2 pixelCenter = launchID + subpixelJitter;
    const float2 clipCoords = pixelCenter / launchSize * 2.0 - 1.0;
    const float4 viewCoords = mul(pushConst.projInverse, float4(clipCoords, 1.0, 1.0));

    RayDesc ray;
    ray.Origin = mul(pushConst.viewInverse, float4(0.0, 0.0, 0.0, 1.0)).xyz;
    ray.Direction = mul(pushConst.viewInverse, float4(normalize(viewCoords.xyz), 0.0)).xyz;
    ray.TMin = 0.001;
    ray.TMax = 100000.0;

    return pathTrace(ray, seed);
}

[shader("raygeneration")]
void rgenMain() {
	float2 launchID = (float2)DispatchRaysIndex().xy;
    float2 launchSize = (float2)DispatchRaysDimensions().xy;

    uint seed = xxhash32(uint3(uint2(launchID.xy), pushConst.frame));

    float3 color = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < 4; i++) {
        color += pathTracePixel(seed);
    }

    color /= 4;

    if (pushConst.frame > 0) {
        float a = 1.0f / float(pushConst.frame + 1);
        float3 old_color = outImage[int2(launchID)].xyz;
        outImage[int2(launchID)] = float4(lerp(old_color, color, a), 1.f);
    }
    else {
        outImage[int2(launchID)] = float4(color, 1.f);
    }
}

[shader("closesthit")]
void rchitMain(inout HitPayload payload, in BuiltInTriangleIntersectionAttributes attr) {
//    uint modelId = InstanceID();
//    uint geometryId = GeometryIndex();
//	uint triangleId = PrimitiveIndex();
//
//    Model model = pushConst.models[modelId];
//    Mesh mesh = model.meshes[geometryId];
//	
//	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
//    float3 pos = getInterpolatedAttribute<float3>(mesh, triangleId, mesh.vertexDataReflection.positionOffset, barycentrics);
//    float3 nrm = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.normalOffset, barycentrics);
//    float3 tng = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.tangentOffset, barycentrics);
//    float3 bin = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.binormalOffset, barycentrics);
//    float2 uv = getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uvOffset, barycentrics);
//    float2 uv2 = getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uv2Offset, barycentrics);
//	
//	float3 worldPosition = float3(mul(float4(pos, 1.0), ObjectToWorld4x3()));
//    float3 worldNormal = normalize(mul(float4(nrm, 0.0), ObjectToWorld4x3()).xyz);
//    float3 worldTangent = normalize(mul(float4(tng, 0.0), ObjectToWorld4x3()).xyz);
//    float3 worldBinormal = normalize(mul(float4(bin, 0.0), ObjectToWorld4x3()).xyz);
//	float distance = length(normalize(worldPosition));
//
//	float3 viewDirection = -WorldRayDirection();
//	float3 lightDirection = normalize(float3(1.0f, 1.0f, 1.0f));
//
//    uint materialId = mesh.materialId;
//
//    Material material = pushConst.materials[materialId];
//
//    MaterialShaderPayload materialShaderPayload;
//    materialShaderPayload.materialId = materialId;
//    materialShaderPayload.hit.world_position = worldPosition;
//    materialShaderPayload.hit.light_direction = lightDirection;
//    materialShaderPayload.hit.light_color = float3(1.0, 1.0, 1.0);
//    materialShaderPayload.hit.view_direction = viewDirection;
//    materialShaderPayload.surface.basis.normal = normalize(worldNormal);
//    materialShaderPayload.surface.basis.tangent = normalize(worldTangent);
//    materialShaderPayload.surface.basis.binormal = normalize(worldBinormal);
//    materialShaderPayload.surface.uv = uv;
//    materialShaderPayload.surface.uv2 = uv2;
//
//    CallShader<MaterialShaderPayload>(material.shaderId, materialShaderPayload);
//
//    payload.color = materialShaderPayload.color;
}

[shader("miss")]
void rmissMain(inout HitPayload payload) {
	payload.color = float3(0.0, 0.0, 0.0);
	payload.depth = MISS_DEPTH;
}

[shader("miss")]
void rmissShadowMain(inout ShadowPayload payload) {
    payload.depth = MISS_DEPTH;
}
