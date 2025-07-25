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

}// namespace vision
