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
oc_float<p> HashGridLogBase_impl(const oc_float<p> &x, const oc_float<p> &base) {
    return ocarina::log(x) / ocarina::log(base);
}
VS_MAKE_CALLABLE(HashGridLogBase)

template<EPort p = D>
oc_uint<p> HashGridGetBaseSlot_impl(const oc_uint<p> &slot, const oc_uint<p> &capacity) {
    return (slot / HASH_GRID_HASH_MAP_BUCKET_SIZE) * HASH_GRID_HASH_MAP_BUCKET_SIZE;
}
VS_MAKE_CALLABLE(HashGridGetBaseSlot)

template<EPort p = D>
oc_uint<p> HashGridHashJenkins32_impl(oc_uint<p> a) {
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}
VS_MAKE_CALLABLE(HashGridHashJenkins32)

template<EPort p = H>
oc_uint<p> HashGridHash32_impl(oc_uint64t<p> hashKey) {
    return HashGridHashJenkins32<p>(uint((hashKey >> 0) & 0xFFFFFFFF)) ^
           HashGridHashJenkins32<p>(uint((hashKey >> 32) & 0xFFFFFFFF));
}
VS_MAKE_CALLABLE(HashGridHash32)

auto HashGridGetLevel(float3 samplePosition, HashGridParameters gridParameters) {
    const float distance2 = dot(gridParameters.cameraPosition - samplePosition, gridParameters.cameraPosition - samplePosition);

    return uint(ocarina::clamp(0.5f * HashGridLogBase<H>(distance2, gridParameters.logarithmBase) + gridParameters.levelBias, 1.0f, float(HASH_GRID_LEVEL_BIT_MASK)));
}

float HashGridGetVoxelSize(uint gridLevel, HashGridParameters gridParameters) {
    return ocarina::pow(gridParameters.logarithmBase, float(gridLevel)) / (gridParameters.sceneScale * pow(gridParameters.logarithmBase, gridParameters.levelBias));
}

int4 HashGridCalculatePositionLog(float3 samplePosition, HashGridParameters gridParameters) {
    samplePosition += float3(HASH_GRID_POSITION_BIAS, HASH_GRID_POSITION_BIAS, HASH_GRID_POSITION_BIAS);

    uint gridLevel = HashGridGetLevel(samplePosition, gridParameters);
    float voxelSize = HashGridGetVoxelSize(gridLevel, gridParameters);
    int3 gridPosition = int3(floor(samplePosition / voxelSize));

    return make_int4(gridPosition.xyz(), gridLevel);
}

uint64_t HashGridComputeSpatialHash(float3 samplePosition, float3 sampleNormal, HashGridParameters gridParameters) {
    uint4 gridPosition = uint4(HashGridCalculatePositionLog(samplePosition, gridParameters));

    uint64_t hashKey = ((uint64_t(gridPosition.x) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 0)) |
                       ((uint64_t(gridPosition.y) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 1)) |
                       ((uint64_t(gridPosition.z) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 2)) |
                       ((uint64_t(gridPosition.w) & HASH_GRID_LEVEL_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 3));

    uint normalBits =
        (sampleNormal.x + HASH_GRID_NORMAL_BIAS >= 0 ? 0 : 1) +
        (sampleNormal.y + HASH_GRID_NORMAL_BIAS >= 0 ? 0 : 2) +
        (sampleNormal.z + HASH_GRID_NORMAL_BIAS >= 0 ? 0 : 4);

    hashKey |= (uint64_t(normalBits) << (HASH_GRID_POSITION_BIT_NUM * 3 + HASH_GRID_LEVEL_BIT_NUM));

    return hashKey;
}

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
    float voxelSize = HashGridGetVoxelSize(gridLevel, gridParameters);
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