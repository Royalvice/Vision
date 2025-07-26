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
        Uint4 voxelDataPacked = voxelDataBuffer.at(cacheIndex);
        ret = SharcUnpackVoxelData(voxelDataPacked);
    };
    return ret;
}

using HashGridIndex = Uint;

void SharcAddVoxelData(SharcParametersVar &sharcParameters, const HashGridIndex &cacheIndex,
                       const Float3 &sampleValue, const Float3 &sampleWeight, const Uint &sampleData) {
    $if(cacheIndex != HASH_GRID_INVALID_CACHE_INDEX) {
        $if(sharcParameters.enableAntiFireflyFilter) {
            Float scalarWeight = ocarina::luminance(sampleWeight);
            scalarWeight = max(scalarWeight, 1.0f);

            const float sampleWeightThreshold = 2.0f;

            $if(scalarWeight > sampleWeightThreshold) {
                Uint4 voxelDataPackedPrev = sharcParameters.voxelDataBufferPrev[cacheIndex];
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
}

}// namespace vision
