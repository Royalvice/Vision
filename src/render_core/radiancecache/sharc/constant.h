//
// Created by Zero on 2025/7/20.
//

#pragma once

#include "math/basic_traits.h"

namespace vision {

using namespace ocarina;

namespace hash_grid {
static constexpr auto HASH_GRID_POSITION_BIT_NUM = 17;
static constexpr auto HASH_GRID_POSITION_BIT_MASK = ((1u << HASH_GRID_POSITION_BIT_NUM) - 1);
static constexpr auto HASH_GRID_LEVEL_BIT_NUM = 10;
static constexpr auto HASH_GRID_LEVEL_BIT_MASK = ((1u << HASH_GRID_LEVEL_BIT_NUM) - 1);
static constexpr auto HASH_GRID_NORMAL_BIT_NUM = 3;
static constexpr auto HASH_GRID_NORMAL_BIT_MASK = ((1u << HASH_GRID_NORMAL_BIT_NUM) - 1);
static constexpr auto HASH_GRID_HASH_MAP_BUCKET_SIZE = 32;
static constexpr auto HASH_GRID_INVALID_HASH_KEY = 0;
static constexpr auto HASH_GRID_INVALID_CACHE_INDEX = 0xFFFFFFFF;

static constexpr auto HASH_GRID_USE_NORMALS = 1;// account for the normal data in the hash key;
static constexpr auto HASH_GRID_ALLOW_COMPACTION = (HASH_GRID_HASH_MAP_BUCKET_SIZE == 32);
static constexpr auto HASH_GRID_POSITION_OFFSET = float3(0.0f, 0.0f, 0.0f);
static constexpr auto HASH_GRID_POSITION_BIAS = 1e-4f;// may require adjustment for extreme scene scales;
static constexpr auto HASH_GRID_NORMAL_BIAS = 1e-3f;
}// namespace hash_grid

namespace sharc {

}// namespace sharc

}// namespace vision