#pragma once
#include "basic.slang"

struct SlabMethodResult {
	float min;
	float max;
};

__generic<int N>
SlabMethodResult slabMethod(AABB<float, N> aabb, Ray<N> ray) {
	const vector<float, N> vMin = (aabb.min - ray.origin) / ray.direction;
	const vector<float, N> vMax = (aabb.max - ray.origin) / ray.direction;

	const vector<float, N> tMins = min(vMin, vMax);
	const vector<float, N> tMaxs = max(vMin, vMax);

	return SlabMethodResult(maxV(tMins), minV(tMaxs));
}

__generic<int N>
bool raycastAABB(AABB<float, N> aabb, Ray<N> ray) {
	SlabMethodResult result = slabMethod(aabb, ray);

	return result.max >= max(0.0, result.min); // && tMin < t;
}

enum RaycastAABBResult {
	MISS,
	OUTSIDE,
	INSIDE,
};

__generic<int N>
RaycastAABBResult raycastAABBEx(AABB<float, N> aabb, Ray<N> ray, out vector<float, N> hit) {
	SlabMethodResult result = slabMethod(aabb, ray);

	if (result.max < max(0.0, result.min)) {
		return RaycastAABBResult.MISS; // && tMin < t;
	}
	else if (result.min >= 0) {
		hit = ray.origin + ray.direction * result.min;
		return RaycastAABBResult.OUTSIDE;
	}
	else {
		hit = ray.origin + ray.direction * result.max;
		return RaycastAABBResult.INSIDE;
	}
}

bool raycastTriangle(Triangle triangle, Ray<3> ray, out float3 hit, out float2 barycentrics) {
	// calculate triangle normal
	float3 v0v1 = triangle.v[1] - triangle.v[0];
	float3 v0v2 = triangle.v[2] - triangle.v[0];
	float3 N = cross(v0v1, v0v2);

	float NdotR = dot(N, ray.direction);

	// ensure triangle plane is not parallel to ray
	if (abs(NdotR) < 1e-8f) {
		return false;
	}

	// solve plane intersection
	float NdotO = dot(N, ray.origin);
	float d = -dot(N, triangle.v[0]);
	float t = -(NdotO + d) / NdotR;

	// discard intersections in negative direction
	if (t < 0) {
		return false;
	}

	// locate intersection point
	hit = ray.origin + t * ray.direction;

	// compute barycentrics
	float u = dot(N, cross(v0v1, hit - triangle.v[0]));
	float v = dot(N, cross(triangle.v[2] - triangle.v[1], hit - triangle.v[1]));
	float w = dot(N, cross(triangle.v[0] - triangle.v[2], hit - triangle.v[2]));

	// check if the intersection point lies on the same side of each edge
	if (u < 0 || v < 0 || w < 0)
		return false;

	// normalize barycentrics and return result
	barycentrics = float2(v, w) / dot(N, N);

	return true;
}
