#define MISS_DEPTH 1000

struct PushConst {
float unk1;
float4x4 viewInverse;
float4x4 projInverse;
};

[[vk::push_constant]] ConstantBuffer<PushConst> pushConst;
[[vk::binding(0, 0)]] RaytracingAccelerationStructure tlas;
[[vk::binding(1, 0)]] RWTexture2D<float4> outImage;

struct HitPayload {
	float3 color;
	float weight;
	int depth;
};

// int3 getTriangleIndices(uint8_t* dataBufferAddress, const TriangleMesh mesh, int primitiveID) {
//   if(mesh.indices.byteStride == sizeof(int16_t)) {
//     int16_t3* indices = (int16_t3*)(dataBufferAddress + mesh.indices.offset);
//     return indices[primitiveID];
//   } else if(mesh.indices.byteStride == sizeof(int32_t)) {
//     int3* indices = (int3*)(dataBufferAddress + mesh.indices.offset);
//     return indices[primitiveID];
//   }
// 
//   return int3(-1);  // Error case - invalid index format
// }

[shader("raygeneration")]
void rgenMain() {
	float2 launchID = (float2)DispatchRaysIndex().xy;
	float2 launchSize = (float2)DispatchRaysDimensions().xy;

	//GltfSceneInfo* sceneInfo = pushConst.sceneInfoAddress;

	const uint rayFlags = 0;
  
	const float2 clipCoords = launchID / launchSize * 2.0 - 1.0;
	const float4 viewCoords = mul(float4(clipCoords, 1.0, 1.0), pushConst.projInverse);

	RayDesc ray;
	ray.Origin = mul(float4(0.0, 0.0, 0.0, 1.0), pushConst.viewInverse).xyz;
	ray.Direction = mul(float4(normalize(viewCoords.xyz), 0.0), pushConst.viewInverse).xyz;
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
	payload.color = float3(1.0, 1.0, 1.0);
}

[shader("miss")]
void rmissMain(inout HitPayload payload) {
	payload.depth = MISS_DEPTH;
}
