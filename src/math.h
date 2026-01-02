#pragma once
#include <Eigen/Eigen>

struct alignas(8) vec2 : Eigen::Vector2f {
    using Eigen::Vector2f::Vector2f;

	inline vec2(const Eigen::Vector2f& other) : Eigen::Vector2f{ other } {}
	inline vec2(Eigen::Vector2f&& other) : Eigen::Vector2f{ std::move(other) } {}
};

struct vec3 : Eigen::Vector4f {
    using Eigen::Vector4f::Vector4f;

    inline vec3(float x, float y, float z) : Eigen::Vector4f{ x, y, z, 0.0f } {}
    inline vec3(const Eigen::Vector3f& other) : Eigen::Vector4f{ other.x(), other.y(), other.z(), 0.0f } {}
    inline vec3(Eigen::Vector3f&& other) : Eigen::Vector4f{ other.x(), other.y(), other.z(), 0.0f } {}

    inline operator Eigen::Vector3f& () { return *reinterpret_cast<Eigen::Vector3f*>(this); }
    inline operator const Eigen::Vector3f& () const { return *reinterpret_cast<const Eigen::Vector3f*>(this); }
};

typedef Eigen::Vector4f vec4;

typedef Eigen::Quaternionf quat;
typedef Eigen::Affine3f mat34;
typedef Eigen::Projective3f mat44;

static_assert(alignof(vec2) == 8, "vec2 must be 8 aligned");
static_assert(alignof(vec3) == 16, "vec3 must be 16 aligned");
static_assert(alignof(vec4) == 16, "vec4 must be 16 aligned");
static_assert(alignof(quat) == 16, "quat must be 16 aligned");
static_assert(alignof(mat34) == 16, "mat34 must be 16 aligned");
static_assert(alignof(mat44) == 16, "mat44 must be 16 aligned");
