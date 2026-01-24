#pragma once
#include <Eigen/Eigen>

struct alignas(8) float2 : Eigen::Vector2f {
	using Eigen::Vector2f::Vector2f;

	inline operator Eigen::Vector2f& () { return *reinterpret_cast<Eigen::Vector2f*>(this); }
	inline operator const Eigen::Vector2f& () const { return *reinterpret_cast<const Eigen::Vector2f*>(this); }
};

struct alignas(16) float3 : Eigen::Vector3f {
	using Eigen::Vector3f::Vector3f;

	//inline float3(const Eigen::Vector3f& other) : Eigen::Vector4f{ other.x(), other.y(), other.z(), 0.0f } {}
	//inline float3(Eigen::Vector3f&& other) : Eigen::Vector4f{ other.x(), other.y(), other.z(), 0.0f } {}

	inline operator Eigen::Vector3f& () { return *reinterpret_cast<Eigen::Vector3f*>(this); }
	inline operator const Eigen::Vector3f& () const { return *reinterpret_cast<const Eigen::Vector3f*>(this); }
};

using float4 = Eigen::Vector4f;
using quat = Eigen::Quaternionf;
using float2x2 = Eigen::Matrix<float, 2, 2>;
using float3x3 = Eigen::Matrix<float, 3, 3>;
using float4x4 = Eigen::Matrix<float, 4, 4>;
using float2x3 = Eigen::Matrix<float, 2, 3>;
using float3x2 = Eigen::Matrix<float, 3, 2>;
using float4x3 = Eigen::Matrix<float, 4, 3>;
using float3x4 = Eigen::Matrix<float, 3, 4>;

using int2 = Eigen::Vector<int32_t, 2>;
using int3 = Eigen::Vector<int32_t, 3>;
using int4 = Eigen::Vector<int32_t, 4>;

using uint = uint32_t;
using uint2 = Eigen::Vector<uint32_t, 2>;
using uint3 = Eigen::Vector<uint32_t, 3>;
using uint4 = Eigen::Vector<uint32_t, 4>;

using bool2 = Eigen::Vector<bool, 2>;
using bool3 = Eigen::Vector<bool, 3>;
using bool4 = Eigen::Vector<bool, 4>;

static_assert(alignof(float2) == 8, "vec2 must be 8 aligned");
static_assert(alignof(float3) == 16, "vec3 must be 16 aligned");
static_assert(alignof(float4) == 16, "vec4 must be 16 aligned");
static_assert(alignof(quat) == 16, "quat must be 16 aligned");
static_assert(alignof(float3x4) == 16, "float3x4 must be 16 aligned");
static_assert(alignof(float4x4) == 16, "float4x4 must be 16 aligned");
static_assert(alignof(float3x3) == 4, "float3x3 must be 4 aligned");
static_assert(alignof(float2x2) == 16, "float2x2 must be 16 aligned");
static_assert(alignof(float2x3) == 4, "float2x3 must be 4 aligned");
static_assert(alignof(float3x2) == 4, "float3x2 must be 4 aligned");
static_assert(alignof(int2) == 4, "int2 must be 4 aligned");
static_assert(alignof(int3) == 4, "int3 must be 4 aligned");
static_assert(alignof(int4) == 16, "int4 must be 16 aligned");
static_assert(alignof(uint) == 4, "uint must be 4 aligned");
static_assert(alignof(uint2) == 4, "uint2 must be 4 aligned");
static_assert(alignof(uint3) == 4, "uint3 must be 4 aligned");
static_assert(alignof(uint4) == 16, "uint4 must be 16 aligned");
static_assert(alignof(bool2) == 1, "bool2 must be 1 aligned");
static_assert(alignof(bool3) == 1, "bool3 must be 1 aligned");
static_assert(alignof(bool4) == 1, "bool4 must be 1 aligned");

inline float4x4 projectionMatrix(float fov, float ratio, float near, float far) {
	float focalLength = 1 / tanf(fov * 0.5f * EIGEN_PI / 180.0f);

	float4x4 res{};
	res.matrix() <<
		-focalLength / ratio, 0.0f, 0.0f, 0.0f,
		0.0f, -focalLength, 0.0f, 0.0f,
		0.0f, 0.0f, -far / (far - near), -(far * near) / (far - near),
		0.0f, 0.0f, -1.0f, 0.0f;

	return res;
}

inline float4x4 lookatMatrix(const Eigen::Vector3f& pos, const Eigen::Vector3f& target, const Eigen::Vector3f& up) {
	Eigen::Vector3f z{ (pos - target).normalized() };
	Eigen::Vector3f x{ up.cross(z).normalized() };
	Eigen::Vector3f y{ z.cross(x) };

	float4x4 res{};
	res.matrix() <<
		x.x(), x.y(), x.z(), -x.dot(pos),
		y.x(), y.y(), y.z(), -y.dot(pos),
		z.x(), z.y(), z.z(), -z.dot(pos),
		0.0f, 0.0f, 0.0f, 1.0f;

	return res;
}