#pragma once
#include "../math/intersection.slang"

// Raymarch through a (potentially anisotropic) grid.
// The grid is mapped onto (X,Y) coordinates [min * cellSize, max * cellSize].
// The ray origin is expected to be inside the grid.
struct GridRaymarchContext {
	AABB<int, 2> aabb;
	int2 step;
	int2 loc;
	float2 tDelta;
	float2 tMax;

	__init(AABB<int, 2> aabb, float2 cellSize, Ray<2> ray) {
		float2 originScaled = ray.origin / cellSize;
		uint2 startCell = uint2(floor(originScaled));
		float2 positionInCell = frac(originScaled) * cellSize;

		this.aabb = aabb;
		this.step = (int2)sign(ray.direction);
		this.loc = startCell;
		this.tDelta = cellSize / ray.direction;
		this.tMax = cellSize * ray.direction - positionInCell;
	}

	[mutating]
	bool next(out int2 nextLoc) {
		if (tMax.x < tMax.y) {
			tMax.x += tDelta.x;
			loc.x += step.x;
		}
		else {
			tMax.y += tDelta.y;
			loc.y += step.y;
		}

		if (any(loc < aabb.min) || any(loc >= aabb.max)) {
			return false;
		}

		nextLoc = (int2)loc;

		return true;
	}
}

/*
 * We raycast in the terrain unit grid, the vertices of which are the texels of the HMAS.
 */
struct HMASMipRaycastContext {
	Texture2D<float2> hmas;
	GridRaymarchContext gridRaymarchContext;
	Ray<3> ray;
	float cellSize;
	uint lodLevel;
	uint halfGridSize;

	__init(Texture2D<float2> hmas, uint lodLevel, uint halfGridSize, AABB<int, 2> aabb, float cellSize, Ray<3> ray) {
		// The raymarch context grid is aligned to integer values, so we compensate for the terrain grid being aligned on half integers.
		AABB<float, 2> gridAABB = AABB<float, 2>(aabb.min.xy * cellSize, aabb.max.xy * cellSize);
		Ray<2> gridRay = Ray<2>(ray.origin.xy + 0.5 * cellSize, normalize(ray.direction.xy));

		float2 hit;
		RaycastAABBResult aabbTestResult = raycastAABBEx(gridAABB, gridRay, hit);
		
		this.hmas = hmas;
		this.gridRaymarchContext = GridRaymarchContext(aabb, cellSize, Ray<2>(aabbTestResult == RaycastAABBResult.INSIDE ? gridRay.origin : hit, gridRay.direction));
		this.ray = ray;
		this.cellSize = cellSize;
		this.lodLevel = lodLevel;
		this.halfGridSize = halfGridSize;
	}
	
	[mutating]
	bool next(out int2 hitLoc) {
		while (gridRaymarchContext.next(hitLoc)) {
			const int2 hitLocInUVCoords = int2(hitLoc.x, -hitLoc.y) + halfGridSize;

			float2 cellMinMax = hmas.mips[lodLevel][hitLoc];

			// Again, grid is offset so compensate.
			float3 cellCoordsMin = float3((float2(hitLoc) - 0.5) * cellSize, cellMinMax.x);
			float3 cellCoordsMax = float3(cellCoordsMin.xy + cellSize, cellMinMax.y);

			if (raycastAABB(AABB<float, 3>(cellCoordsMin, cellCoordsMax), ray)) {
				return true;
			}
		}

		return false;
	}
};

bool raycastHMASInternal(Texture2D<float2> hmas, uint depth, AABB<int, 2> aabb, Ray<3> ray, out int2 hitLoc) {
	HMASMipRaycastContext ctx = HMASMipRaycastContext(hmas, 9 - depth, (1 << depth) - 1, aabb, 1 << (9 - depth), ray);

	while (ctx.next(hitLoc)) {
		if (depth == 9 || raycastHMASInternal(hmas, depth + 1, AABB<int, 2>(2 * aabb.min - 1, 2 * aabb.max + 1), ray, hitLoc)) {
			return true;
		}
	}

	return false;
}

bool raycastHMAS(Texture2D<float2> hmas, Ray<3> ray, out int2 hitLoc) {
	return raycastHMASInternal(hmas, 0, AABB<int, 2>(0, 1), ray, hitLoc);
}
