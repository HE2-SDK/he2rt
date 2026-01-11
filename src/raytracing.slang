#include "shader-types.h"

#define MISS_DEPTH 1000

using namespace he2rt::shaders;

[[vk::push_constant]] ConstantBuffer<PushConstant> pushConst;
[[vk::binding(0)]] RaytracingAccelerationStructure tlas;
[[vk::binding(1)]] RWTexture2D<float4> outImage;

struct HitPayload {
	float3 color;
	float weight;
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
	uint meshId = InstanceID();
	uint triangleId = PrimitiveIndex();

	Mesh mesh = pushConst.meshes[meshId];
	
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	float3 pos = getInterpolatedAttribute<float3>(mesh, triangleId, mesh.vertexDataReflection.positionOffset, barycentrics);
	float3 nrm = getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.normalOffset, barycentrics);
	
	float3 worldPos = float3(mul(float4(pos, 1.0), ObjectToWorld4x3()));
	//float3 worldNormal = normalize(mul(WorldToObject4x3(), nrm).xyz);
	float3 worldNormal = normalize(mul(float4(nrm, 0.0), ObjectToWorld4x3()).xyz);
	float distance = length(normalize(worldPos));

	float3 N = normalize(worldNormal); // Surface normal
	float3 V = -WorldRayDirection(); // View direction (towards camera)
	float3 L = normalize(float3(1.0f, 1.0f, 0.0f)); // Light direction

	// float3 color = pbrMetallicRoughness(albedo, metallic, roughness, N, V, L);
	float diffuse = max(dot(L, N), 0.0);
	diffuse = diffuse * (1.0 / (1.0 + 0.25 * distance * distance));

	payload.color = float3(diffuse);
}

[shader("miss")]
void rmissMain(inout HitPayload payload) {
	payload.color = float3(0.0, 0.0, 0.0);
	payload.depth = MISS_DEPTH;
}
