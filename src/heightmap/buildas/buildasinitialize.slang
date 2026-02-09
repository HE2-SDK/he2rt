#include "shader-types.h"

using namespace he2rt::heightmap::buildas;

[[vk::binding(Binding::input)]] Texture2D<float> input;
[[vk::binding(Binding::output)]] RWTexture2D<float2> output[MIP_COUNT];

[shader("compute")]
[numthreads(WORKGROUP_SIZE, WORKGROUP_SIZE, 1)]
void main(uint2 globalThreadID: SV_DispatchThreadID) {
    output[0][globalThreadID] = float2(input[globalThreadID], input[globalThreadID]);
}
