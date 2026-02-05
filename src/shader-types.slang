#pragma once
#include "shader-types.h"

[[vk::push_constant]] ConstantBuffer<he2rt::shaders::PushConstant> pushConst;
[[vk::binding(0, 0)]] Sampler2D textures[];
[[vk::binding(0, 1)]] RaytracingAccelerationStructure tlas;
[[vk::binding(1, 1)]] RWTexture2D<float4> outImage;

static const float INFINITE = 1e32F;
static const bool USE_SER = true;
