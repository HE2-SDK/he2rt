#pragma once
#include <shader-types.slang>
#include "math.slang"

struct SurfaceParameters {
    Basis basis;
    half2 uvs[4];
    float4 vertexColor;
};

struct TriangleAttributes<T> {
	T v0;
	T v1;
	T v2;
};

T getVertexAttribute<T>(he2rt::shaders::Mesh mesh, uint16_t vertexIndex, uint offset) {
	return *(T*)(mesh.vertexBuffer + vertexIndex * mesh.vertexDataReflection.stride + offset);
}

TriangleAttributes<T> getTriangleAttributes<T>(he2rt::shaders::Mesh mesh, uint triangleIndex, uint offset) {
	return {
		getVertexAttribute<T>(mesh, mesh.indexBuffer[triangleIndex * 3 + 0], offset),
		getVertexAttribute<T>(mesh, mesh.indexBuffer[triangleIndex * 3 + 1], offset),
		getVertexAttribute<T>(mesh, mesh.indexBuffer[triangleIndex * 3 + 2], offset),
	};
}

T getInterpolatedAttribute<T : IFloat>(he2rt::shaders::Mesh mesh, uint triangleIndex, uint offset, float3 barycentrics) {
	const TriangleAttributes<T> attrs = getTriangleAttributes<T>(mesh, triangleIndex, offset);

	return T(barycentrics.x) * attrs.v0 + T(barycentrics.y) * attrs.v1 + T(barycentrics.z) * attrs.v2;
}

float4 getInterpolatedUByte4nAttribute(he2rt::shaders::Mesh mesh, uint triangleIndex, uint offset, float3 barycentrics) {
    const TriangleAttributes<uint8_t4> attrs = getTriangleAttributes<uint8_t4>(mesh, triangleIndex, offset);

    return float4(barycentrics.x) * float4(attrs.v0.xyzw) / 255.0 + float4(barycentrics.y) * float4(attrs.v1.xyzw) / 255.0 + float4(barycentrics.z) * float4(attrs.v2.xyzw) / 255.0;
}

float3 getInterpolatedDec3nAttribute(he2rt::shaders::Mesh mesh, uint triangleIndex, uint offset, float3 barycentrics) {
	const TriangleAttributes<int8_t4> attrs = getTriangleAttributes<int8_t4>(mesh, triangleIndex, offset);

    return float3(barycentrics.x) * float3(attrs.v0.xyz) / 511.0 + float3(barycentrics.y) * float3(attrs.v1.xyz) / 511.0 + float3(barycentrics.z) * float3(attrs.v2.xyz) / 511.0;
}

half2 getInterpolatedHalf2Attribute(he2rt::shaders::Mesh mesh, uint triangleIndex, uint offset, float3 barycentrics) {
    const TriangleAttributes<half2> attrs = getTriangleAttributes<half2>(mesh, triangleIndex, offset);

    return half2(barycentrics.x) * attrs.v0 + half2(barycentrics.y) * attrs.v1 + half2(barycentrics.z) * attrs.v2;
}



float3 getPosition(he2rt::shaders::Mesh mesh, uint triangleId, float4x3 objToWorld, float3 barycentrics) {
    return mul(float4(getInterpolatedAttribute<float3>(mesh, triangleId, mesh.vertexDataReflection.positionOffset, barycentrics), 1.0f), objToWorld);
}

float3 getNormal(he2rt::shaders::Mesh mesh, uint triangleId, float4x3 objToWorld, float3 barycentrics) {
    return mul(float4(getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.normalOffset, barycentrics), 0.0f), objToWorld);
}

float3 getTangent(he2rt::shaders::Mesh mesh, uint triangleId, float4x3 objToWorld, float3 barycentrics) {
    return mul(float4(getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.tangentOffset, barycentrics), 0.0f), objToWorld);
}

float3 getBinormal(he2rt::shaders::Mesh mesh, uint triangleId, float4x3 objToWorld, float3 barycentrics) {
    return mul(float4(getInterpolatedDec3nAttribute(mesh, triangleId, mesh.vertexDataReflection.binormalOffset, barycentrics), 0.0f), objToWorld);
}

half2 getUV0(he2rt::shaders::Mesh mesh, uint triangleId, float3 barycentrics) {
    return getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uvOffset, barycentrics);
}

half2 getUV1(he2rt::shaders::Mesh mesh, uint triangleId, float3 barycentrics) {
    return getInterpolatedHalf2Attribute(mesh, triangleId, mesh.vertexDataReflection.uv2Offset, barycentrics);
}

half2[4] getUVs(he2rt::shaders::Mesh mesh, uint triangleId, float3 barycentrics) {
    const half2 uv = getUV0(mesh, triangleId, barycentrics);
    const half2 uv2 = getUV1(mesh, triangleId, barycentrics);

    return { uv, uv2, uv, uv };
}

float4 getColor(he2rt::shaders::Mesh mesh, uint triangleId, float3 barycentrics) {
    return getInterpolatedUByte4nAttribute(mesh, triangleId, mesh.vertexDataReflection.colorOffset, barycentrics);
}



SurfaceParameters getSurfaceParameters(he2rt::shaders::Mesh mesh, uint triangleId, float4x3 objToWorld, float3 barycentrics) {
    return {
        Basis(
            getNormal(mesh, triangleId, objToWorld, barycentrics),
            getTangent(mesh, triangleId, objToWorld, barycentrics),
            getBinormal(mesh, triangleId, objToWorld, barycentrics)
        ),
        getUVs(mesh, triangleId, barycentrics),
        getColor(mesh, triangleId, barycentrics),
    };
}
