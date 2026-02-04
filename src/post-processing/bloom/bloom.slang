#include "shader-types.h"

using namespace he2rt::shaders::bloom;

[[vk::push_constant]] ConstantBuffer<Config> config;
[[vk::binding(Binding::input)]] Sampler2D input;
[[vk::binding(Binding::output)]] RWTexture2D<float4> output;

[shader("compute")]
[numthreads(BLOOM_WORKGROUP_SIZE, BLOOM_WORKGROUP_SIZE, 1)]
void downsample(uint2 coords: SV_DispatchThreadID) {
    float2 inputDimensions, outputDimensions;
    input.GetDimensions(inputDimensions.x, inputDimensions.y);
    output.GetDimensions(outputDimensions.x, outputDimensions.y);

    float2 inputTexelSize = 1.0f / inputDimensions;
    float2 texCoord = (float2(coords) + float2(0.5f)) / outputDimensions;
    float inputLod = float(config.inputLod);

    float x = inputTexelSize.x;
    float y = inputTexelSize.y;

    float3 a = input.SampleLevel(float2(texCoord.x - 2*x, texCoord.y + 2*y), inputLod).xyz;
    float3 b = input.SampleLevel(float2(texCoord.x,       texCoord.y + 2*y), inputLod).xyz;
    float3 c = input.SampleLevel(float2(texCoord.x + 2*x, texCoord.y + 2*y), inputLod).xyz;

    float3 d = input.SampleLevel(float2(texCoord.x - 2*x, texCoord.y), inputLod).xyz;
    float3 e = input.SampleLevel(float2(texCoord.x,       texCoord.y), inputLod).xyz;
    float3 f = input.SampleLevel(float2(texCoord.x + 2*x, texCoord.y), inputLod).xyz;

    float3 g = input.SampleLevel(float2(texCoord.x - 2*x, texCoord.y - 2*y), inputLod).xyz;
    float3 h = input.SampleLevel(float2(texCoord.x,       texCoord.y - 2*y), inputLod).xyz;
    float3 i = input.SampleLevel(float2(texCoord.x + 2*x, texCoord.y - 2*y), inputLod).xyz;

    float3 j = input.SampleLevel(float2(texCoord.x - x, texCoord.y + y), inputLod).xyz;
    float3 k = input.SampleLevel(float2(texCoord.x + x, texCoord.y + y), inputLod).xyz;
    float3 l = input.SampleLevel(float2(texCoord.x - x, texCoord.y - y), inputLod).xyz;
    float3 m = input.SampleLevel(float2(texCoord.x + x, texCoord.y - y), inputLod).xyz;

    output.Store(coords, float4(e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125, 1));
}

[shader("compute")]
[numthreads(BLOOM_WORKGROUP_SIZE, BLOOM_WORKGROUP_SIZE, 1)]
void upsample(uint2 coords: SV_DispatchThreadID) {
    float2 outputDimensions;
    output.GetDimensions(outputDimensions.x, outputDimensions.y);

    float2 texCoord = (float2(coords) + float2(0.5f)) / outputDimensions;
    float inputLod = float(config.inputLod);

    float x = config.kernelSize;
    float y = config.kernelSize;

    float3 a = input.SampleLevel(float2(texCoord.x - x, texCoord.y + y), inputLod).xyz;
    float3 b = input.SampleLevel(float2(texCoord.x, texCoord.y + y), inputLod).xyz;
    float3 c = input.SampleLevel(float2(texCoord.x + x, texCoord.y + y), inputLod).xyz;

    float3 d = input.SampleLevel(float2(texCoord.x - x, texCoord.y), inputLod).xyz;
    float3 e = input.SampleLevel(float2(texCoord.x, texCoord.y), inputLod).xyz;
    float3 f = input.SampleLevel(float2(texCoord.x + x, texCoord.y), inputLod).xyz;

    float3 g = input.SampleLevel(float2(texCoord.x - x, texCoord.y - y), inputLod).xyz;
    float3 h = input.SampleLevel(float2(texCoord.x, texCoord.y - y), inputLod).xyz;
    float3 i = input.SampleLevel(float2(texCoord.x + x, texCoord.y - y), inputLod).xyz;

    output.Store(coords, float4((e * 4.0 + (b + d + f + h) * 2.0 + (a + c + g + i)) / 16.0, 1));
}
