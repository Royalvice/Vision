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
    return HashGridHashJenkins32<p>(cast<uint>((hashKey >> 0) & 0xFFFFFFFF)) ^
           HashGridHashJenkins32<p>(cast<uint>((hashKey >> 32) & 0xFFFFFFFF));
}
VS_MAKE_CALLABLE(HashGridHash32)

template<EPort p = D>
[[nodiscard]] oc_uint<p> HashGridGetLevel_impl(const oc_float3<p> &samplePosition,
                                               const var_t<HashGridParameters, p> &gridParameters) {
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
[[nodiscard]] oc_uint64t<p> HashGridComputeSpatialHash_impl(const oc_float3<p> &samplePosition, const oc_float3<p> &sampleNormal,
                                                            const var_t<HashGridParameters, p> &gridParameters) {
    oc_uint4<p> gridPosition = cast<uint4>(HashGridCalculatePositionLog<p>(samplePosition, gridParameters));

    oc_uint64t<p> hashKey = ((cast<uint64_t>(gridPosition.x) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 0)) |
                            ((cast<uint64_t>(gridPosition.y) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 1)) |
                            ((cast<uint64_t>(gridPosition.z) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 2)) |
                            ((cast<uint64_t>(gridPosition.w) & HASH_GRID_LEVEL_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 3));

    oc_uint<p> normalBits =
        ocarina::select(sampleNormal.x + HASH_GRID_NORMAL_BIAS >= 0, 0, 1) +
        ocarina::select(sampleNormal.y + HASH_GRID_NORMAL_BIAS >= 0, 0, 2) +
        ocarina::select(sampleNormal.z + HASH_GRID_NORMAL_BIAS >= 0, 0, 4);

    hashKey |= (cast<uint64_t>(normalBits) << (HASH_GRID_POSITION_BIT_NUM * 3 + HASH_GRID_LEVEL_BIT_NUM));

    return hashKey;
}
VS_MAKE_CALLABLE(HashGridComputeSpatialHash)

template<EPort p = D>
[[nodiscard]] oc_float3<p> HashGridGetPositionFromKey_impl(const oc_uint64t<p> &hashKey,
                                                           const var_t<HashGridParameters, p> &gridParameters) {
    int signBit = 1 << (HASH_GRID_POSITION_BIT_NUM - 1);
    int signMask = ~((1 << HASH_GRID_POSITION_BIT_NUM) - 1);

    oc_int3<p> gridPosition;
    gridPosition.x = cast<int>((hashKey >> (HASH_GRID_POSITION_BIT_NUM * 0)) & HASH_GRID_POSITION_BIT_MASK);
    gridPosition.y = cast<int>((hashKey >> (HASH_GRID_POSITION_BIT_NUM * 1)) & HASH_GRID_POSITION_BIT_MASK);
    gridPosition.z = cast<int>((hashKey >> (HASH_GRID_POSITION_BIT_NUM * 2)) & HASH_GRID_POSITION_BIT_MASK);

    // Fix negative coordinates
    gridPosition.x = ocarina::select((gridPosition.x & signBit) != 0, gridPosition.x | signMask, gridPosition.x);
    gridPosition.y = ocarina::select((gridPosition.y & signBit) != 0, gridPosition.y | signMask, gridPosition.y);
    gridPosition.z = ocarina::select((gridPosition.z & signBit) != 0, gridPosition.z | signMask, gridPosition.z);

    oc_uint<p> gridLevel = cast<uint>((hashKey >> HASH_GRID_POSITION_BIT_NUM * 3) & HASH_GRID_LEVEL_BIT_MASK);
    oc_float<p> voxelSize = HashGridGetVoxelSize<p>(gridLevel, gridParameters);
    oc_float3<p> samplePosition = (gridPosition + 0.5f) * voxelSize;

    return samplePosition;
}
VS_MAKE_CALLABLE(HashGridGetPositionFromKey)

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

namespace vision ::inline sharc {
using namespace ocarina;
void HashMapAtomicCompareExchange(HashMapDataVar &hashMapData, const Uint &dstOffset, const Uint64t &compareValue,
                                  const Uint64t &value, Uint64t &originalValue) {
    outline("HashMapAtomicCompareExchange", [&] {
        originalValue = ocarina::atomic_CAS(hashMapData.hashEntriesBuffer.at(dstOffset), compareValue, value);
    });
}

[[nodiscard]] Bool HashMapInsert(HashMapDataVar &hashMapData, const Uint64t &hashKey, Uint &cacheIndex) {
    static Callable impl = [](HashMapDataVar &hashMapData, const Uint64t &hashKey, Uint &cacheIndex) -> Bool {
        Bool ret = false;
        Uint hash = HashGridHash32<D>(hashKey);
        Uint slot = hash % hashMapData.capacity;
        Uint64t prevHashGridKey = HASH_GRID_INVALID_HASH_KEY;
        Uint baseSlot = HashGridGetBaseSlot<D>(slot, hashMapData.capacity);

        $for(bucketOffset, HASH_GRID_HASH_MAP_BUCKET_SIZE) {
            HashMapAtomicCompareExchange(hashMapData, baseSlot + bucketOffset,
                                         HASH_GRID_INVALID_HASH_KEY, hashKey, prevHashGridKey);
            $if(prevHashGridKey == HASH_GRID_INVALID_HASH_KEY || prevHashGridKey == hashKey) {
                cacheIndex = baseSlot + bucketOffset;
                ret = true;
                $return(ret);
            };
        };
        cacheIndex = 0u;
        return ret;
    };
    impl.function()->set_description("HashMapInsert");
    return impl(hashMapData, hashKey, cacheIndex);
}

[[nodiscard]] Bool HashMapFind(HashMapDataVar &hashMapData, const Uint64t &hashKey, Uint &cacheIndex) {
    static Callable impl = [](HashMapDataVar &hashMapData, const Uint64t &hashKey, Uint &cacheIndex) {
        Bool ret = false;

        Uint hash = HashGridHash32<D>(hashKey);
        Uint slot = hash % hashMapData.capacity;

        Uint baseSlot = HashGridGetBaseSlot<D>(slot, hashMapData.capacity);

        $for(bucketOffset, HASH_GRID_HASH_MAP_BUCKET_SIZE) {
            Uint64t storedHashKey = hashMapData.hashEntriesBuffer[baseSlot + bucketOffset];
            $if(storedHashKey == hashKey) {
                cacheIndex = baseSlot + bucketOffset;
                ret = true;
                $break;
            }
            $elif(storedHashKey == HASH_GRID_INVALID_HASH_KEY) {
                ret = false;
                $break;
            };
        };
        return ret;
    };
    impl.function()->set_description("HashMapFind");
    return impl(hashMapData, hashKey, cacheIndex);
}

[[nodiscard]] Uint HashMapInsertEntry(HashMapDataVar &hashMapData, const Float3 &samplePosition,
                                      const Float3 &sampleNormal, const HashGridParametersVar &gridParameters) {
    static Callable impl = [](HashMapDataVar &hashMapData, const Float3 &samplePosition,
                              const Float3 &sampleNormal, const HashGridParametersVar &gridParameters) {
        Uint cacheIndex = HASH_GRID_INVALID_CACHE_INDEX;
        const Uint64t hashKey = HashGridComputeSpatialHash<D>(samplePosition, sampleNormal, gridParameters);
        Bool successful = HashMapInsert(hashMapData, hashKey, cacheIndex);
        return cacheIndex;
    };
    impl.function()->set_description("HashMapInsertEntry");
    return impl(hashMapData, samplePosition, sampleNormal, gridParameters);
}

HashGridIndex HashMapFindEntry(HashMapDataVar &hashMapData, const Float3 &samplePosition,
                               const Float3 &sampleNormal, const HashGridParametersVar &gridParameters) {
    static Callable impl = [](HashMapDataVar &hashMapData, const Float3 &samplePosition,
                              const Float3 &sampleNormal, const HashGridParametersVar &gridParameters) {
        HashGridIndex cacheIndex = HASH_GRID_INVALID_CACHE_INDEX;
        const HashGridKey hashKey = HashGridComputeSpatialHash(samplePosition, sampleNormal, gridParameters);
        Bool successful = HashMapFind(hashMapData, hashKey, cacheIndex);

        return cacheIndex;
    };
    impl.function()->set_description("HashMapFindEntry");
    return impl(hashMapData, samplePosition, sampleNormal, gridParameters);
}

template<EPort p = D>
[[nodiscard]] oc_float3<p> HashGridGetColorFromHash32_impl(const oc_uint<p> &hash) {
    oc_float3<p> color;
    color.x = ((hash >> 0) & 0x3ff) / 1023.0f;
    color.y = ((hash >> 11) & 0x7ff) / 2047.0f;
    color.z = ((hash >> 22) & 0x7ff) / 2047.0f;
    return color;
}
VS_MAKE_CALLABLE(HashGridGetColorFromHash32)

Float3 HashGridDebugColoredHash(const Float3 &samplePosition,
                                const HashGridParametersVar &gridParameters) {
    static Callable impl = [](const Float3 &samplePosition,
                              const HashGridParametersVar &gridParameters) {
        Uint64t hashKey = HashGridComputeSpatialHash(samplePosition, float3(0, 0, 0), gridParameters);
        Uint gridLevel = HashGridGetLevel<D>(samplePosition, gridParameters);
        Float3 hashColor = HashGridGetColorFromHash32(HashGridHashJenkins32(gridLevel));
        Float3 color = HashGridGetColorFromHash32(HashGridHash32(hashKey)) * hashColor;
        return color;
    };
    impl.function()->set_description("HashGridDebugColoredHash");
    return impl(samplePosition, gridParameters);
}

Float3 HashGridDebugOccupancy(const Uint2 &pixelPosition, const Uint2 &screenSize,
                              HashMapDataVar &hashMapData) {
    static Callable impl = [](const Uint2 &pixelPosition, const Uint2 &screenSize,
                              HashMapDataVar &hashMapData) {
        const uint elementSize = 7;
        const uint borderSize = 1;
        const uint blockSize = elementSize + borderSize;
        Uint rowNum = screenSize.y / blockSize;
        Uint rowIndex = pixelPosition.y / blockSize;
        Uint columnIndex = pixelPosition.x / blockSize;
        Uint elementIndex = (columnIndex / HASH_GRID_HASH_MAP_BUCKET_SIZE) * (rowNum * HASH_GRID_HASH_MAP_BUCKET_SIZE) +
                            rowIndex * HASH_GRID_HASH_MAP_BUCKET_SIZE +
                            (columnIndex % HASH_GRID_HASH_MAP_BUCKET_SIZE);
        Float3 ret = make_float3(0.f);

        $if(elementIndex < hashMapData.capacity && ((pixelPosition.x % blockSize) < elementSize && (pixelPosition.y % blockSize) < elementSize)) {
            Uint64t storedHashGridKey = hashMapData.hashEntriesBuffer[elementIndex];
            $if(storedHashGridKey != HASH_GRID_INVALID_HASH_KEY) {
                ret = make_float3(0.0f, 1.0f, 0.0f);
            };
        };

        return ret;
    };
    impl.function()->set_description("HashGridDebugOccupancy");
    return impl(pixelPosition, screenSize, hashMapData);
}

}// namespace vision::inline sharc