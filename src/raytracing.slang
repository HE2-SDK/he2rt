#include "shader-types.slang"

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

[shader("raygeneration")]
void rgenMain() {
	float2 launchID = (float2)DispatchRaysIndex().xy;
	float2 launchSize = (float2)DispatchRaysDimensions().xy;

	//GltfSceneInfo* sceneInfo = pushConst.sceneInfoAddress;

	const uint rayFlags = 0;
  
	const float2 clipCoords = launchID / launchSize * 2.0 - 1.0;
	const float4 viewCoords = mul(pushConst.projInverse, float4(clipCoords, 1.0, 1.0));

	RayDesc ray;
	ray.Origin = mul(pushConst.viewInverse, float4(0.0, 0.0, 0.0, 1.0)).xyz;
	ray.Direction = mul(pushConst.viewInverse, float4(normalize(viewCoords.xyz), 0.0)).xyz;
	ray.TMin = 0.001;
	ray.TMax = 100000.0;

	HitPayload payload;
	payload.color = float3(0, 0, 0);
	payload.weight = 1;
	payload.depth = 0;

	TraceRay(tlas, rayFlags, 0xff, 0, 0, 0, ray, payload);

	float3 color = payload.color;

	outImage[int2(launchID)] = float4(color, 1.0);
}

[shader("closesthit")]
void rchitMain(inout HitPayload payload, in BuiltInTriangleIntersectionAttributes attr) {
    uint modelId = InstanceID();
    uint geometryId = GeometryIndex();
	uint triangleId = PrimitiveIndex();

    Model model = pushConst.models[modelId];
    Mesh mesh = model.meshes[geometryId];
	
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float3 pos = getInterpolatedAttribute<float3>(mesh, triangleId, mesh.vertexDataReflection.positionOffset, barycentrics);
    float3 nrm = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.normalOffset, barycentrics);
    float3 tng = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.tangentOffset, barycentrics);
    float3 bin = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.binormalOffset, barycentrics);
    float2 uv = getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uvOffset, barycentrics);
    float2 uv2 = getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uv2Offset, barycentrics);
	
	float3 worldPosition = float3(mul(float4(pos, 1.0), ObjectToWorld4x3()));
    float3 worldNormal = normalize(mul(float4(nrm, 0.0), ObjectToWorld4x3()).xyz);
    float3 worldTangent = normalize(mul(float4(tng, 0.0), ObjectToWorld4x3()).xyz);
    float3 worldBinormal = normalize(mul(float4(bin, 0.0), ObjectToWorld4x3()).xyz);
	float distance = length(normalize(worldPosition));

	float3 viewDirection = -WorldRayDirection();
	float3 lightDirection = normalize(float3(1.0f, 1.0f, 1.0f));

    uint materialId = mesh.materialId;

    Material material = pushConst.materials[materialId];

    MaterialShaderPayload materialShaderPayload;
    materialShaderPayload.materialId = materialId;
    materialShaderPayload.weight = 1.0f;
    materialShaderPayload.hit.world_position = worldPosition;
    materialShaderPayload.hit.light_direction = lightDirection;
    materialShaderPayload.hit.light_color = float3(1.0, 1.0, 1.0);
    materialShaderPayload.hit.view_direction = viewDirection;
    materialShaderPayload.hit.shadow = testShadow(worldPosition, worldNormal, lightDirection);
    materialShaderPayload.surface.basis.normal = normalize(worldNormal);
    materialShaderPayload.surface.basis.tangent = normalize(worldTangent);
    materialShaderPayload.surface.basis.binormal = normalize(worldBinormal);
    materialShaderPayload.surface.uv = uv;
    materialShaderPayload.surface.uv2 = uv2;

    CallShader<MaterialShaderPayload>(material.shaderId, materialShaderPayload);

    payload.color = materialShaderPayload.color;
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
