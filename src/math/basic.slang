#pragma once

__generic<T, int N>
struct AABB {
	vector<T, N> min;
	vector<T, N> max;
};

__generic<int N>
struct Ray {
	vector<float, N> origin;
	vector<float, N> direction;
//	vector<float, N> invDirection;
//
//	__init(vector<float, N> origin, vector<float, N> direction) {
//		this.origin = origin;
//		this.direction = direction;
//		this.invDirection = 1 / direction;
//	}
};

struct Triangle {
	float3 v[3];
};

__generic<int N>
inline float minV(vector<float, N> v) {
	float res = v[0];

	[unroll]
	for (int i = 1; i < N; i++) {
		res = min(res, v[i]);
	}

	return res;
}

__generic<int N>
inline float maxV(vector<float, N> v) {
	float res = v[0];

	[unroll]
	for (int i = 1; i < N; i++) {
		res = max(res, v[i]);
	}

	return res;
}
