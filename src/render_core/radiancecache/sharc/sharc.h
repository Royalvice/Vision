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

OC_PARAM_STRUCT(vision, SharcParameters,gridParameters, hashMapData, enableAntiFireflyFilter,
                voxelDataBuffer, voxelDataBufferPrev){};

namespace vision {
struct SharcState {
    std::array<uint, SHARC_PROPOGATION_DEPTH> cacheIndices{};
    std::array<float, 2> sampleWeights;
    uint pathLength{};
};
}// namespace vision

OC_STRUCT(vision, SharcState, cacheIndices,
          sampleWeights, pathLength){};
