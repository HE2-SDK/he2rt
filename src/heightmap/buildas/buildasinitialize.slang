#include "shader-types.h"

using namespace he2rt::heightmap::buildas;

[[vk::binding(Binding::input)]] Texture2D<float> input;
[[vk::binding(Binding::output)]] RWTexture2D<float2> output[MIP_COUNT];

[shader("compute")]
[numthreads(WORKGROUP_SIZE, WORKGROUP_SIZE, 1)]
void main(uint2 globalThreadID: SV_DispatchThreadID) {
    float2 inputDimensions;
    input.GetDimensions(inputDimensions.x, inputDimensions.y);
    uint2 inputDims = (uint2)inputDimensions;

    uint2 coords = uint2(
        globalThreadID.x == inputDims.x - 1 ? globalThreadID.x - 1 : globalThreadID.x,
        globalThreadID.y == inputDims.y - 1 ? globalThreadID.y - 1 : globalThreadID.y,
    );

	float cellMin = min(min(input[coords], input[coords + uint2(1, 0)]), min(input[coords + uint2(1, 1)], input[coords + uint2(0, 1)]));
	float cellMax = max(max(input[coords], input[coords + uint2(1, 0)]), max(input[coords + uint2(1, 1)], input[coords + uint2(0, 1)]));

    output[0][globalThreadID] = float2(cellMin, cellMax);
}
