//
// Created by Zero on 2025/7/25.
//

#include "hash_grid.h"

using namespace ocarina;

namespace vision {
struct SharcParameters {
    HashGridParameters gridParameters;
    HashMapData hashMapData;
    uint enableAntiFireflyFilter{};
    BufferDesc<uint4> voxelDataBuffer;
    BufferDesc<uint4> voxelDataBufferPrev;
};
}// namespace vision
OC_PARAM_STRUCT(vision, SharcParameters, gridParameters, hashMapData, enableAntiFireflyFilter,
                voxelDataBuffer, voxelDataBufferPrev){};

namespace vision {
struct SharcState {
    std::array<uint, SHARC_PROPOGATION_DEPTH> cacheIndices{};
    std::array<float3, SHARC_PROPOGATION_DEPTH> sampleWeights;
    uint pathLength{};
};
}// namespace vision
OC_STRUCT(vision, SharcState, cacheIndices,
          sampleWeights, pathLength){};

namespace vision {
struct SharcHitData {
    float3 positionWorld;
    float3 normalWorld;
    float3 emissive;
};
}// namespace vision
OC_STRUCT(vision, SharcHitData, positionWorld,
          normalWorld, emissive){};

namespace vision {
struct SharcVoxelData {
    uint3 accumulatedRadiance;
    uint accumulatedSampleNum{};
    uint accumulatedFrameNum{};
    uint staleFrameNum{};
};
}// namespace vision
OC_STRUCT(vision, SharcVoxelData, accumulatedRadiance,
          accumulatedSampleNum, accumulatedFrameNum,
          staleFrameNum){};

namespace vision {
struct SharcResolveParameters {
    float3 cameraPositionPrev;
    uint accumulationFrameNum{};
    uint staleFrameNumMax{};
    uint enableAntiFireflyFilter{};
};
}// namespace vision
OC_STRUCT(vision, SharcResolveParameters, cameraPositionPrev,
          accumulationFrameNum, staleFrameNumMax,
          enableAntiFireflyFilter){};

namespace vision {
template<EPort p = D>
oc_uint<p> SharcGetSampleNum_impl(const oc_uint<p> &packedData) {
    return (packedData >> SHARC_SAMPLE_NUM_BIT_OFFSET) & SHARC_SAMPLE_NUM_BIT_MASK;
}
VS_MAKE_CALLABLE(SharcGetSampleNum)

template<EPort p = D>
oc_uint<p> SharcGetStaleFrameNum_impl(const oc_uint<p> &packedData) {
    return (packedData >> SHARC_STALE_FRAME_NUM_BIT_OFFSET) & SHARC_STALE_FRAME_NUM_BIT_MASK;
}
VS_MAKE_CALLABLE(SharcGetStaleFrameNum)

template<EPort p = D>
oc_uint<p> SharcGetAccumulatedFrameNum_impl(const oc_uint<p> &packedData) {
    return (packedData >> SHARC_ACCUMULATED_FRAME_NUM_BIT_OFFSET) & SHARC_ACCUMULATED_FRAME_NUM_BIT_MASK;
}
VS_MAKE_CALLABLE(SharcGetAccumulatedFrameNum)

template<EPort p = D>
oc_float3<p> SharcResolveAccumulatedRadiance_impl(const oc_uint3<p> &accumulatedRadiance,
                                                  const oc_uint<p> &accumulatedSampleNum) {
    return accumulatedRadiance / (accumulatedSampleNum * SHARC_RADIANCE_SCALE);
}
VS_MAKE_CALLABLE(SharcResolveAccumulatedRadiance)

template<EPort p = D>
var_t<SharcVoxelData, p> SharcUnpackVoxelData_impl(const oc_uint4<p> &voxelDataPacked) {
    var_t<SharcVoxelData, p> voxelData;
    voxelData.accumulatedRadiance = voxelDataPacked.xyz();
    voxelData.accumulatedSampleNum = SharcGetSampleNum<p>(voxelDataPacked.w);
    voxelData.staleFrameNum = SharcGetStaleFrameNum<p>(voxelDataPacked.w);
    voxelData.accumulatedFrameNum = SharcGetAccumulatedFrameNum<p>(voxelDataPacked.w);
    return voxelData;
}
VS_MAKE_CALLABLE(SharcUnpackVoxelData)

SharcVoxelDataVar SharcGetVoxelData(BufferVar<uint4> &voxelDataBuffer, const Uint &cacheIndex) {
    static Callable impl = [](BufferVar<uint4> &voxelDataBuffer, const Uint &cacheIndex) {
        SharcVoxelDataVar voxelData;
        voxelData.accumulatedRadiance = make_uint3(0u);
        voxelData.accumulatedSampleNum = 0;
        voxelData.accumulatedFrameNum = 0;
        voxelData.staleFrameNum = 0;
        SharcVoxelDataVar ret;
        $if(cacheIndex == HASH_GRID_INVALID_CACHE_INDEX) {
            ret = voxelData;
        }
        $else {
            Uint4 voxelDataPacked = voxelDataBuffer.read(cacheIndex);
            ret = SharcUnpackVoxelData(voxelDataPacked);
        };
        return ret;
    };
    impl.set_description("SharcGetVoxelData");
    return impl(voxelDataBuffer, cacheIndex);
}

void SharcAddVoxelData(SharcParametersVar &sharcParameters, const HashGridIndex &cacheIndex,
                       const Float3 &sampleValue, const Float3 &sampleWeight, const Uint &sampleData) {
    static Callable impl = [](SharcParametersVar &sharcParameters, const HashGridIndex &cacheIndex,
                              const Float3 &sampleValue, const Float3 &sampleWeight, const Uint &sampleData) {
        $if(cacheIndex == HASH_GRID_INVALID_CACHE_INDEX) {
            $return();
        };
        $if(sharcParameters.enableAntiFireflyFilter) {
            Float scalarWeight = ocarina::luminance(sampleWeight);
            scalarWeight = max(scalarWeight, 1.0f);

            const float sampleWeightThreshold = 2.0f;

            $if(scalarWeight > sampleWeightThreshold) {
                Uint4 voxelDataPackedPrev = sharcParameters.voxelDataBufferPrev.read(cacheIndex);
                Uint sampleNumPrev = SharcGetSampleNum(voxelDataPackedPrev.w);

                const uint sampleConfidenceThreshold = 2;
                $if(sampleNumPrev > SHARC_SAMPLE_NUM_MULTIPLIER * sampleConfidenceThreshold) {
                    Float lum = luminance(SharcResolveAccumulatedRadiance(voxelDataPackedPrev.xyz(), sampleNumPrev));
                    Float luminancePrev = max(lum, 1.0f);
                    Float luminanceCur = max(luminance(sampleValue * sampleWeight), 1.0f);
                    Float confidenceScale = lerp(1.0f / sampleNumPrev, 5.0f, 10.0f);
                    sampleWeight *= saturate(confidenceScale * luminancePrev / luminanceCur);
                }
                $else {
                    scalarWeight = pow(scalarWeight, 0.5f);
                    sampleWeight /= scalarWeight;
                };
            };
        };

        Uint3 scaledRadiance = make_uint3(sampleValue * sampleWeight * SHARC_RADIANCE_SCALE);

        for (uint i = 0; i < 3; ++i) {
            $if(scaledRadiance[i] != 0) {
                atomic_add(sharcParameters.voxelDataBuffer[cacheIndex][i], scaledRadiance[i]);
            };
        }
        $if(sampleData != 0) {
            atomic_add(sharcParameters.voxelDataBuffer[cacheIndex].w, sampleData);
        };
    };
    impl.set_description("SharcAddVoxelData");
    impl(sharcParameters, cacheIndex,
         sampleValue, sampleWeight, sampleData);
}

void SharcInit(SharcStateVar &sharcState) {
    sharcState.pathLength = 0;
}

void SharcUpdateMiss(SharcParametersVar &sharcParameters, const SharcStateVar &sharcState, Float3 radiance) {
    static Callable impl = [](SharcParametersVar &sharcParameters, const SharcStateVar &sharcState, Float3 radiance) {
        $for(i, sharcState.pathLength) {
            SharcAddVoxelData(sharcParameters, sharcState.cacheIndices[i], radiance, sharcState.sampleWeights[i], 0);
            radiance *= sharcState.sampleWeights[i];
        };
    };
    impl.set_description("SharcUpdateMiss");
    impl(sharcParameters, sharcState, radiance);
}

Bool SharcUpdateHit(SharcParametersVar &sharcParameters, SharcStateVar &sharcState,
                    const SharcHitDataVar &sharcHitData, Float3 directLighting, Float random) {

    static Callable impl = [](SharcParametersVar &sharcParameters, SharcStateVar &sharcState,
                              const SharcHitDataVar &sharcHitData, Float3 directLighting, Float random) {
        Bool continueTracing = true;

        HashGridIndex cacheIndex = HashMapInsertEntry(sharcParameters.hashMapData, sharcHitData.positionWorld, sharcHitData.normalWorld, sharcParameters.gridParameters);

        Float3 sharcRadiance = directLighting;

        Uint resamplingDepth = cast<uint>(ocarina::round(ocarina::lerp(random, float(SHARC_RESAMPLING_DEPTH_MIN), float(SHARC_PROPOGATION_DEPTH - 1))));

        $if(resamplingDepth <= sharcState.pathLength) {
            SharcVoxelDataVar voxelData = SharcGetVoxelData(sharcParameters.voxelDataBufferPrev, cacheIndex);

            $if(voxelData.accumulatedSampleNum > SHARC_SAMPLE_NUM_THRESHOLD) {
                sharcRadiance = SharcResolveAccumulatedRadiance(voxelData.accumulatedRadiance, voxelData.accumulatedSampleNum);

                continueTracing = false;
            };
        };

        $if(continueTracing) {
            SharcAddVoxelData(sharcParameters, cacheIndex, float3(0.0f), float3(0.0f), 1);
        };

        Uint i = 0;
        $for(idx, sharcState.pathLength) {
            i = idx;
            SharcAddVoxelData(sharcParameters, sharcState.cacheIndices[i], sharcRadiance, sharcState.sampleWeights[i], 0);
            sharcRadiance *= sharcState.sampleWeights[i];
        };

        $for(idx, i, 0, -1) {
            sharcState.cacheIndices[idx] = sharcState.cacheIndices[idx - 1];
            sharcState.sampleWeights[idx] = sharcState.sampleWeights[idx - 1];
        };

        sharcState.cacheIndices[0] = cacheIndex;
        sharcState.pathLength += 1;
        sharcState.pathLength = ocarina::min(sharcState.pathLength, SHARC_PROPOGATION_DEPTH - 1u);

        return continueTracing;
    };
    impl.set_description("SharcUpdateHit");
    return impl(sharcParameters, sharcState, sharcHitData, directLighting, random);
}

void SharcSetThroughput(SharcStateVar &sharcState, Float3 throughput) {
    sharcState.sampleWeights[0] = throughput;
}

Bool SharcGetCachedRadiance(SharcParametersVar &sharcParameters, const SharcHitDataVar &sharcHitData,
                            Float3 &radiance, const Bool &debug) {
    static Callable impl = [](SharcParametersVar &sharcParameters, const SharcHitDataVar &sharcHitData,
                              Float3 &radiance, const Bool &debug) {
        Bool ret = false;
        $if(debug) {
            radiance = make_float3(0);
        };
        const Uint sampleThreshold = ocarina::select(debug, 0u, SHARC_SAMPLE_NUM_THRESHOLD);
        HashGridIndex cacheIndex = HashMapFindEntry(sharcParameters.hashMapData, sharcHitData.positionWorld,
                                                    sharcHitData.normalWorld, sharcParameters.gridParameters);
        $if(cacheIndex == HASH_GRID_INVALID_CACHE_INDEX) {
            $return(ret);
        };
        SharcVoxelDataVar voxelData = SharcGetVoxelData(sharcParameters.voxelDataBuffer, cacheIndex);
        $if(voxelData.accumulatedSampleNum > sampleThreshold) {
            radiance = SharcResolveAccumulatedRadiance(voxelData.accumulatedRadiance,
                                                       voxelData.accumulatedSampleNum);
            ret = true;
        };
        return ret;
    };
    impl.set_description("SharcGetCachedRadiance");
    return impl(sharcParameters, sharcHitData, radiance, debug);
}

void SharcCopyHashEntry(const Uint &entryIndex, HashMapDataVar &hashMapData,
                        BufferVar<uint> &copyOffsetBuffer) {
    static Callable impl = [](const Uint &entryIndex, HashMapDataVar &hashMapData,
                              BufferVar<uint> &copyOffsetBuffer) -> void {
        $if(entryIndex >= hashMapData.capacity) {
            $return();
        };

        Uint copyOffset = copyOffsetBuffer.read(entryIndex);
        $if(copyOffset == 0) {
            $return();
        };

        $if(copyOffset == HASH_GRID_INVALID_CACHE_INDEX) {
            hashMapData.hashEntriesBuffer.write(entryIndex, HASH_GRID_INVALID_HASH_KEY);
        }
        $elif(copyOffset != 0) {
            HashGridKey hashKey = hashMapData.hashEntriesBuffer.read(entryIndex);
            hashMapData.hashEntriesBuffer.write(entryIndex, HASH_GRID_INVALID_HASH_KEY);
            hashMapData.hashEntriesBuffer.write(copyOffset, hashKey);
        };
        copyOffsetBuffer.write(entryIndex, 0u);
    };
    impl.set_description("SharcCopyHashEntry");
    impl(entryIndex, hashMapData, copyOffsetBuffer);
}

template<EPort p = D>
oc_int<p> SharcGetGridDistance2_impl(const oc_int3<p> &position) {
    return ocarina::dot(position, position);
}
VS_MAKE_CALLABLE(SharcGetGridDistance2)

template<EPort p = D>
oc_ulong<p> SharcGetAdjacentLevelHashKey_impl(const oc_ulong<p> &hashKey, const HashGridParametersVar &gridParameters,
                                              const oc_float3<p> &cameraPositionPrev) {
    const int signBit = 1 << (HASH_GRID_POSITION_BIT_NUM - 1);
    const int signMask = ~((1 << HASH_GRID_POSITION_BIT_NUM) - 1);

    oc_int3<p> gridPosition;

    for (int i = 0; i < 3; ++i) {
        gridPosition[i] = cast<int>((hashKey >> HASH_GRID_POSITION_BIT_NUM * i) & HASH_GRID_POSITION_BIT_MASK);
        gridPosition[i] = ocarina::select(((gridPosition[i] & signBit) != 0),
                                          gridPosition[i] | signMask,
                                          gridPosition[i]);
    }

    oc_int<p> level = cast<int>((hashKey >> (HASH_GRID_POSITION_BIT_NUM * 3)) & HASH_GRID_LEVEL_BIT_MASK);

    oc_float<p> voxelSize = HashGridGetVoxelSize<p>(level, gridParameters);
    oc_int3<p> cameraGridPosition = make_int3(floor((gridParameters.cameraPosition + HASH_GRID_POSITION_OFFSET) / voxelSize));
    oc_int3<p> cameraVector = cameraGridPosition - gridPosition;
    oc_int<p> cameraDistance = SharcGetGridDistance2<p>(cameraVector);

    oc_int3<p> cameraGridPositionPrev = make_int3(floor((cameraPositionPrev + HASH_GRID_POSITION_OFFSET) / voxelSize));
    oc_int3<p> cameraVectorPrev = cameraGridPositionPrev - gridPosition;
    oc_int<p> cameraDistancePrev = SharcGetGridDistance2(cameraVectorPrev);

    $if(cameraDistance < cameraDistancePrev) {
        gridPosition = make_int3(floor(gridPosition / gridParameters.logarithmBase));
        level = ocarina::min(level + 1, int(HASH_GRID_LEVEL_BIT_MASK));
    }
    $else {
        gridPosition = make_int3(floor(gridPosition * gridParameters.logarithmBase));
        level = ocarina::max(level - 1, 1);
    };

    oc_ulong<p> modifiedHashGridKey = ((cast<ulong>(gridPosition.x) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 0)) |
                                      ((cast<ulong>(gridPosition.y) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 1)) |
                                      ((cast<ulong>(gridPosition.z) & HASH_GRID_POSITION_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 2)) |
                                      ((cast<ulong>(level) & HASH_GRID_LEVEL_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 3));

    modifiedHashGridKey |= hashKey & (cast<ulong>(HASH_GRID_NORMAL_BIT_MASK) << (HASH_GRID_POSITION_BIT_NUM * 3 + HASH_GRID_LEVEL_BIT_NUM));

    return modifiedHashGridKey;
}
VS_MAKE_CALLABLE(SharcGetAdjacentLevelHashKey)

void SharcResolveEntry(const Uint &entryIndex, SharcParametersVar &sharcParameters,
                       const SharcResolveParametersVar &resolveParameters,
                       BufferVar<uint> &copyOffsetBuffer) {
    static Callable impl = [](const Uint &entryIndex, SharcParametersVar &sharcParameters,
                              const SharcResolveParametersVar &resolveParameters,
                              BufferVar<uint> &copyOffsetBuffer) {
        $if(entryIndex >= sharcParameters.hashMapData.capacity) {
            $return();
        };
        HashGridKey hashKey = sharcParameters.hashMapData.hashEntriesBuffer.read(entryIndex);

        $if(hashKey == HASH_GRID_INVALID_HASH_KEY) {
            $return();
        };

        Uint4 voxelDataPackedPrev = sharcParameters.voxelDataBufferPrev.read(entryIndex);
        Uint4 voxelDataPacked = sharcParameters.voxelDataBuffer.read(entryIndex);

        Uint sampleNum = SharcGetSampleNum<D>(voxelDataPacked.w);
        Uint sampleNumPrev = SharcGetSampleNum<D>(voxelDataPackedPrev.w);
        Uint accumulatedFrameNum = SharcGetAccumulatedFrameNum<D>(voxelDataPackedPrev.w) + 1;
        Uint staleFrameNum = SharcGetStaleFrameNum<D>(voxelDataPackedPrev.w);

        voxelDataPacked.xyz() *= SHARC_SAMPLE_NUM_MULTIPLIER;
        sampleNum *= SHARC_SAMPLE_NUM_MULTIPLIER;

        Uint3 accumulatedRadiance = voxelDataPacked.xyz() + voxelDataPackedPrev.xyz();
        Uint accumulatedSampleNum = sampleNum + sampleNumPrev;

        Float3 cameraOffset = sharcParameters.gridParameters.cameraPosition.xyz() -
                              resolveParameters.cameraPositionPrev.xyz();

        $if((ocarina::length_squared(cameraOffset) != 0) && (accumulatedFrameNum < resolveParameters.accumulationFrameNum)) {
            HashGridKey adjacentLevelHashKey = SharcGetAdjacentLevelHashKey<D>(hashKey, sharcParameters.gridParameters,
                                                                               resolveParameters.cameraPositionPrev);

            HashGridIndex cacheIndex = HASH_GRID_INVALID_CACHE_INDEX;
            $if(HashMapFind(sharcParameters.hashMapData, adjacentLevelHashKey, cacheIndex)) {
                Uint4 adjacentPackedDataPrev = sharcParameters.voxelDataBufferPrev.read(cacheIndex);
                Uint adjacentSampleNum = SharcGetSampleNum<D>(adjacentPackedDataPrev.w);
                $if(adjacentSampleNum > SHARC_SAMPLE_NUM_THRESHOLD) {
                    Float blendWeight = adjacentSampleNum / cast<float>(adjacentSampleNum + accumulatedSampleNum);
                    accumulatedRadiance = make_uint3(lerp(make_float3(blendWeight),
                                                          make_float3(accumulatedRadiance.xyz()),
                                                          make_float3(adjacentPackedDataPrev.xyz())));
                    accumulatedSampleNum = cast<uint>(lerp(blendWeight,
                                                           cast<float>(accumulatedSampleNum),
                                                           cast<float>(adjacentSampleNum)));
                };
            };
        };

        $if(accumulatedSampleNum > SHARC_NORMALIZED_SAMPLE_NUM) {
            accumulatedSampleNum >>= 1;
            accumulatedRadiance >>= 1;
        };

        Uint accumulationFrameNum = ocarina::clamp(resolveParameters.accumulationFrameNum,
                                                   SHARC_ACCUMULATED_FRAME_NUM_MIN,
                                                   SHARC_ACCUMULATED_FRAME_NUM_MAX);

        $if(accumulatedFrameNum > accumulationFrameNum) {
            Float normalizedAccumulatedSampleNum = ocarina::round(accumulatedSampleNum * cast<float>(accumulationFrameNum) / accumulatedFrameNum);
            Float normalizationScale = normalizedAccumulatedSampleNum / accumulatedSampleNum;

            accumulatedSampleNum = cast<uint>(normalizedAccumulatedSampleNum);
            accumulatedRadiance = make_uint3(accumulatedRadiance * normalizationScale);
            accumulatedFrameNum = cast<uint>(accumulatedFrameNum * normalizationScale);
        };

        staleFrameNum = ocarina::select((sampleNum != 0), 0u, staleFrameNum + 1);

        Uint4 packedData;
        packedData.xyz() = make_uint3(accumulatedRadiance);

        packedData.w = min(accumulatedSampleNum, SHARC_SAMPLE_NUM_BIT_MASK);
        packedData.w |= (min(accumulatedFrameNum, SHARC_ACCUMULATED_FRAME_NUM_BIT_MASK) << SHARC_ACCUMULATED_FRAME_NUM_BIT_OFFSET);
        packedData.w |= (min(staleFrameNum, SHARC_STALE_FRAME_NUM_BIT_MASK) << SHARC_STALE_FRAME_NUM_BIT_OFFSET);

        Bool isValidElement = staleFrameNum < max(resolveParameters.staleFrameNumMax, SHARC_STALE_FRAME_NUM_MIN);

        $if(!isValidElement) {
            packedData = make_uint4(0);
        };

        Uint validElementNum = warp_active_count_bits(isValidElement);
        Uint validElementMask = warp_active_bit_mask(isValidElement).x;
        Bool cond = (entryIndex % HASH_GRID_HASH_MAP_BUCKET_SIZE) >= validElementNum;
        Bool isMovableElement = isValidElement && cond;
        Uint movableElementIndex = warp_prefix_count_bits(isMovableElement);

        $if(cond) {
            Uint writeOffset = 0;
            sharcParameters.hashMapData.hashEntriesBuffer.write(entryIndex, HASH_GRID_INVALID_HASH_KEY);
            sharcParameters.voxelDataBuffer.write(entryIndex, make_uint4(0));
            $if(isValidElement) {
                Uint emptySlotIndex = 0;
                $while(emptySlotIndex < validElementNum) {
                    $if(((validElementMask >> writeOffset) & 0x1) == 0) {
                        $if(emptySlotIndex == movableElementIndex) {
                            writeOffset += HashGridGetBaseSlot<D>(entryIndex, sharcParameters.hashMapData.capacity);
                            sharcParameters.voxelDataBuffer.write(writeOffset, packedData);
                            $break;
                        };
                        emptySlotIndex += 1;
                    };
                    writeOffset += 1;
                };
            };
            Uint val = ocarina::select(writeOffset != 0, writeOffset, HASH_GRID_INVALID_CACHE_INDEX);
            copyOffsetBuffer.write(entryIndex, val);
        }
        $elif(isValidElement) {
            sharcParameters.voxelDataBuffer.write(entryIndex, packedData);
        };
    };

    impl(entryIndex, sharcParameters, resolveParameters, copyOffsetBuffer);
}

[[nodiscard]] Float3 SharcDebugGetBitsOccupancyColor(const Float &occupancy) {
    static Callable impl = [](const Float &occupancy) -> Float3 {
        Float3 ret = make_float3(1, 0, 0) * occupancy;
        $if(occupancy < SHARC_DEBUG_BITS_OCCUPANCY_THRESHOLD_LOW) {
            ret = make_float3(0.0f, 1.0f, 0.0f) * (occupancy + SHARC_DEBUG_BITS_OCCUPANCY_THRESHOLD_LOW);
        }
        $elif(occupancy < SHARC_DEBUG_BITS_OCCUPANCY_THRESHOLD_MEDIUM) {
            ret = float3(1.0f, 1.0f, 0.0f) * (occupancy + SHARC_DEBUG_BITS_OCCUPANCY_THRESHOLD_MEDIUM);
        };
        return ret;
    };
    impl.set_description("SharcDebugGetBitsOccupancyColor");
    return impl(occupancy);
}

Float3 SharcDebugBitsOccupancySampleNum(SharcParametersVar &sharcParameters,
                                        const SharcHitDataVar &sharcHitData) {
    static Callable impl = [](SharcParametersVar &sharcParameters,
                              const SharcHitDataVar &sharcHitData) -> Float3 {
        HashGridIndex cacheIndex = HashMapFindEntry(sharcParameters.hashMapData, sharcHitData.positionWorld, sharcHitData.normalWorld, sharcParameters.gridParameters);
        SharcVoxelDataVar voxelData = SharcGetVoxelData(sharcParameters.voxelDataBuffer, cacheIndex);
        Float occupancy = cast<float>(voxelData.accumulatedSampleNum) / SHARC_SAMPLE_NUM_BIT_MASK;
        return SharcDebugGetBitsOccupancyColor(occupancy);
    };
    impl.set_description("SharcDebugBitsOccupancySampleNum");
    return impl(sharcParameters, sharcHitData);
}

Float3 SharcDebugBitsOccupancyRadiance(SharcParametersVar &sharcParameters,
                                       const SharcHitDataVar &sharcHitData) {
    static Callable impl = [](SharcParametersVar &sharcParameters,
                              const SharcHitDataVar &sharcHitData) -> Float3 {
        HashGridIndex cacheIndex = HashMapFindEntry(sharcParameters.hashMapData, sharcHitData.positionWorld,
                                                    sharcHitData.normalWorld, sharcParameters.gridParameters);
        SharcVoxelDataVar voxelData = SharcGetVoxelData(sharcParameters.voxelDataBuffer, cacheIndex);
        Float occupancy = cast<float>(max_comp(voxelData.accumulatedRadiance)) / 0xFFFFFFFF;
        return SharcDebugGetBitsOccupancyColor(occupancy);
    };
    impl.set_description("SharcDebugBitsOccupancyRadiance");
    return impl(sharcParameters, sharcHitData);
}

}// namespace vision
