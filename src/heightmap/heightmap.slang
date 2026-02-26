#pragma once
#include "lookupas.slang"

bool raycastHeightmap(Texture2D<float2> heightmap, Texture2D<float2> hmas, Ray<3> ray, out float3 hitLoc) {
	int2 hmasHit;

	if (!raycastHMAS(hmas, ray, hmasHit)) {
		return false;
	}

	const float2 hmasHitF = float2(hmasHit) - 0.5f;
	
	const int2 hmasHitInUVCoords = int2(hmasHit.x, -hmasHit.y) + 512;

	const float heightTopLeft = heightmap[hmasHit].x;
	const float heightTopRight = heightmap[hmasHit + uint2(1, 0)].x;
	const float heightBottomRight = heightmap[hmasHit + uint2(1, 1)].x;
	const float heightBottomLeft = heightmap[hmasHit + uint2(0, 1)].x;

	if (!raycastTriangle(Triangle(hmasHitF + 
}