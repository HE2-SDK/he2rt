#include "shader-types.h"

using namespace he2rt::heightmap::buildas;

[[vk::binding(Binding::input)]] Texture2D<float2> input;
[[vk::binding(Binding::output)]] RWTexture2D<float2> output[MIP_COUNT];

groupshared float minima[WORKGROUP_SIZE][WORKGROUP_SIZE];
groupshared float maxima[WORKGROUP_SIZE][WORKGROUP_SIZE];

inline float2 GetValue(uint2 xy) {
    return float2(minima[xy.x][xy.y], maxima[xy.x][xy.y]);
}
inline void SetValue(uint2 xy, float2 v) {
    minima[xy.x][xy.y] = v.x;
    maxima[xy.x][xy.y] = v.y;
}

[shader("compute")]
[numthreads(WORKGROUP_SIZE, WORKGROUP_SIZE, 1)]
void main(uint2 globalThreadID: SV_DispatchThreadID, uint2 groupThreadID: SV_GroupThreadID) {
    float2 topLeft = input[globalThreadID];
    SetValue(groupThreadID, topLeft);
    GroupMemoryBarrierWithGroupSync();

    for (uint mipLevel = 0; mipLevel < MIP_COUNT; mipLevel++) {
        uint order = 1 << mipLevel;
        uint remainderMask = (2 * order) - 1;

        if ((groupThreadID.x & remainderMask) == 0 && (groupThreadID.y & remainderMask) == 0) {
            const float2 topRight = GetValue(groupThreadID + uint2(order, 0));
            const float2 bottomRight = GetValue(groupThreadID + uint2(order, order));
            const float2 bottomLeft = GetValue(groupThreadID + uint2(0, order));

            topLeft = float2(
                min(min(topLeft.x, topRight.x), min(bottomRight.x, bottomLeft.x)),
                max(max(topLeft.y, topRight.y), max(bottomRight.y, bottomLeft.y)),
            );

            output[mipLevel][globalThreadID / (2 * order)] = topLeft;
            SetValue(groupThreadID, topLeft);

            GroupMemoryBarrierWithGroupSync();
        }
    }
}
