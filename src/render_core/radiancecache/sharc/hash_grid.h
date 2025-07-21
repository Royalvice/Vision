//
// Created by Zero on 2025/7/2.
//

#pragma once

#include "core/stl.h"
#include "dsl/dsl.h"
#include "constant.h"
#include "base/vs_header.h"

namespace vision {
using namespace ocarina;
struct HashGridParameters {
    float3 cameraPosition{};
    float logarithmBase{};
    float sceneScale{};
    float levelBias{};
};
}// namespace vision

OC_STRUCT(vision, HashGridParameters, cameraPosition,
          logarithmBase, sceneScale, levelBias){};

namespace vision {
using namespace ocarina;
template<EPort p = D>
[[nodiscard]] oc_float<p> HashGridLogBase_impl(const oc_float<p> &x, const oc_float<p> &base) {
    return ocarina::log(x) / ocarina::log(base);
}
VS_MAKE_CALLABLE(HashGridLogBase)

template<EPort p = D>
[[nodiscard]] oc_uint<p> HashGridGetBaseSlot_impl(const oc_uint<p> &slot, const oc_uint<p> &capacity) {
    return (slot / HASH_GRID_HASH_MAP_BUCKET_SIZE) * HASH_GRID_HASH_MAP_BUCKET_SIZE;
}
VS_MAKE_CALLABLE(HashGridGetBaseSlot)

template<EPort p = D>
[[nodiscard]] oc_uint<p> HashGridHashJenkins32_impl(oc_uint<p> a) {
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}
VS_MAKE_CALLABLE(HashGridHashJenkins32)

template<EPort p = D>
[[nodiscard]] oc_uint<p> HashGridHash32_impl(const oc_uint64t<p> &hashKey) {
    return HashGridHashJenkins32<p>(uint((hashKey >> 0) & 0xFFFFFFFF)) ^
           HashGridHashJenkins32<p>(uint((hashKey >> 32) & 0xFFFFFFFF));
}
VS_MAKE_CALLABLE(HashGridHash32)

template<EPort p = D>
[[nodiscard]] oc_uint<p> HashGridGetLevel_impl(const oc_float3<p> &samplePosition, const var_t<HashGridParameters, p> &gridParameters) {
    oc_float<p> distance2 = dot(gridParameters.cameraPosition - samplePosition, gridParameters.cameraPosition - samplePosition);
    oc_float<p> val = 0.5f * HashGridLogBase<p>(distance2, gridParameters.logarithmBase) + gridParameters.levelBias;
    return cast<uint>(ocarina::clamp(val, 1.0f, cast<float>(HASH_GRID_LEVEL_BIT_MASK)));
}
VS_MAKE_CALLABLE(HashGridGetLevel)

template<EPort p = D>
[[nodiscard]] oc_float<p> HashGridGetVoxelSize_impl(const oc_uint<p> &gridLevel,
                                                    const var_t<HashGridParameters, p> &gridParameters) {
    oc_float<p> numerator = ocarina::pow(gridParameters.logarithmBase, cast<float>(gridLevel));
    oc_float<p> denominator = (gridParameters.sceneScale * pow(gridParameters.logarithmBase, gridParameters.levelBias));
    return numerator / denominator;
}
VS_MAKE_CALLABLE(HashGridGetVoxelSize)

template<EPort p = D>
[[nodiscard]] oc_int4<p> HashGridCalculatePositionLog_impl(oc_float3<p> samplePosition,
                                                           const var_t<HashGridParameters, p> &gridParameters) {
    samplePosition += make_float3(HASH_GRID_POSITION_BIAS, HASH_GRID_POSITION_BIAS, HASH_GRID_POSITION_BIAS);

    oc_uint<p> gridLevel = HashGridGetLevel<p>(samplePosition, gridParameters);
    oc_float<p> voxelSize = HashGridGetVoxelSize<p>(gridLevel, gridParameters);
    oc_int3<p> gridPosition = make_int3(floor(samplePosition / voxelSize));

    return make_int4(gridPosition.xyz(), gridLevel);
}
VS_MAKE_CALLABLE(HashGridCalculatePositionLog)

template<EPort p = D>
oc_uint64t<p> HashGridComputeSpatialHash_impl(const oc_float3<p> &samplePosition, const oc_float3<p> &sampleNormal,
                                              const var_t<HashGridParameters, p> &gridParameters) {
    oc_uint4<p> gridPosition = cast<uint4>(HashGridCalculatePositionLog<p>(samplePosition, gridParameters));

    oc_uint64t<p> hashKey = ((cast<uint64_t>(gridPosition.x) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 0)) |
                            ((cast<uint64_t>(gridPosition.y) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 1)) |
                            ((cast<uint64_t>(gridPosition.z) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 2)) |
                            ((cast<uint64_t>(gridPosition.w) & HASH_GRID_LEVEL_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 3));

    oc_uint<p> normalBits =
        (sampleNormal.x + HASH_GRID_NORMAL_BIAS >= 0 ? 0 : 1) +
        (sampleNormal.y + HASH_GRID_NORMAL_BIAS >= 0 ? 0 : 2) +
        (sampleNormal.z + HASH_GRID_NORMAL_BIAS >= 0 ? 0 : 4);

    hashKey |= (cast<uint64_t>(normalBits) << (HASH_GRID_POSITION_BIT_NUM * 3 + HASH_GRID_LEVEL_BIT_NUM));

    return hashKey;
}
VS_MAKE_CALLABLE(HashGridComputeSpatialHash)

float3 HashGridGetPositionFromKey(const uint64_t hashKey, HashGridParameters gridParameters) {
    const int signBit = 1 << (HASH_GRID_POSITION_BIT_NUM - 1);
    const int signMask = ~((1 << HASH_GRID_POSITION_BIT_NUM) - 1);

    int3 gridPosition;
    gridPosition.x = int((hashKey >> (HASH_GRID_POSITION_BIT_NUM * 0)) & HASH_GRID_POSITION_BIT_MASK);
    gridPosition.y = int((hashKey >> (HASH_GRID_POSITION_BIT_NUM * 1)) & HASH_GRID_POSITION_BIT_MASK);
    gridPosition.z = int((hashKey >> (HASH_GRID_POSITION_BIT_NUM * 2)) & HASH_GRID_POSITION_BIT_MASK);

    // Fix negative coordinates
    gridPosition.x = (gridPosition.x & signBit) != 0 ? gridPosition.x | signMask : gridPosition.x;
    gridPosition.y = (gridPosition.y & signBit) != 0 ? gridPosition.y | signMask : gridPosition.y;
    gridPosition.z = (gridPosition.z & signBit) != 0 ? gridPosition.z | signMask : gridPosition.z;

    uint gridLevel = uint((hashKey >> HASH_GRID_POSITION_BIT_NUM * 3) & HASH_GRID_LEVEL_BIT_MASK);
    float voxelSize = HashGridGetVoxelSize<H>(gridLevel, gridParameters);
    float3 samplePosition = (gridPosition + 0.5f) * voxelSize;

    return samplePosition;
}

}// namespace vision

namespace vision {
struct HashMapData {
    uint capacity{};
    BufferDesc<uint64_t> hashEntriesBuffer;
    BufferDesc<uint> lockBuffer;
};
}// namespace vision

OC_PARAM_STRUCT(vision, HashMapData, capacity,
                hashEntriesBuffer, lockBuffer){};

namespace vision {

void HashMapAtomicCompareExchange(HashMapData hashMapData, uint dstOffset, uint64_t compareValue, uint64_t value, uint64_t originalValue) {
}
}// namespace vision