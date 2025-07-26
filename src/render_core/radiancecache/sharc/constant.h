//
// Created by Zero on 2025/7/20.
//

#pragma once

#include "math/basic_traits.h"

namespace vision ::inline sharc {

using namespace ocarina;

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
static constexpr auto SHARC_SAMPLE_NUM_BIT_NUM = 18;
static constexpr auto SHARC_SAMPLE_NUM_BIT_OFFSET = 0;
static constexpr auto SHARC_SAMPLE_NUM_BIT_MASK = ((1u << SHARC_SAMPLE_NUM_BIT_NUM) - 1);
static constexpr auto SHARC_ACCUMULATED_FRAME_NUM_BIT_NUM = 6;
static constexpr auto SHARC_ACCUMULATED_FRAME_NUM_BIT_OFFSET = (SHARC_SAMPLE_NUM_BIT_NUM);
static constexpr auto SHARC_ACCUMULATED_FRAME_NUM_BIT_MASK = ((1u << SHARC_ACCUMULATED_FRAME_NUM_BIT_NUM) - 1);
static constexpr auto SHARC_STALE_FRAME_NUM_BIT_NUM = 8;
static constexpr auto SHARC_STALE_FRAME_NUM_BIT_OFFSET = (SHARC_SAMPLE_NUM_BIT_NUM + SHARC_ACCUMULATED_FRAME_NUM_BIT_NUM);
static constexpr auto SHARC_STALE_FRAME_NUM_BIT_MASK = ((1u << SHARC_STALE_FRAME_NUM_BIT_NUM) - 1);
static constexpr auto SHARC_GRID_LOGARITHM_BASE = 2.0f;
static constexpr auto SHARC_GRID_LEVEL_BIAS = 0;// positive bias adds extra levels with content magnification (can be negative as well);
static constexpr auto SHARC_ENABLE_COMPACTION = HASH_GRID_ALLOW_COMPACTION;
static constexpr auto SHARC_BLEND_ADJACENT_LEVELS = 1;// combine the data from adjacent levels on camera movement;
static constexpr auto SHARC_DEFERRED_HASH_COMPACTION = (SHARC_ENABLE_COMPACTION && SHARC_BLEND_ADJACENT_LEVELS);
static constexpr auto SHARC_NORMALIZED_SAMPLE_NUM = (1u << (SHARC_SAMPLE_NUM_BIT_NUM - 1));
static constexpr auto SHARC_ACCUMULATED_FRAME_NUM_MIN = 1;                                   // minimum number of frames to use for data accumulation;
static constexpr auto SHARC_ACCUMULATED_FRAME_NUM_MAX = SHARC_ACCUMULATED_FRAME_NUM_BIT_MASK;// maximum number of frames to use for data accumulation;
                                                                                             // increase sample count internally to make resolve step with low sample count more robust, power of 2 usage may help compiler with optimizations;
static constexpr auto SHARC_SAMPLE_NUM_MULTIPLIER = 16;
static constexpr auto SHARC_SAMPLE_NUM_THRESHOLD = 0u;// elements with sample count above this threshold will be used for early-out/resampling;

static constexpr auto SHARC_SEPARATE_EMISSIVE = 0;// if set, emissive values should be passed separately on updates and added to the cache query;

static constexpr auto SHARC_INCLUDE_DIRECT_LIGHTING = 1;// if set cache values include both direct and indirect lighting;

static constexpr auto SHARC_PROPOGATION_DEPTH = 4u;// controls the amount of vertices stored in memory for signal backpropagation;

static constexpr auto SHARC_UPDATE = 1;
static constexpr auto SHARC_ENABLE_CACHE_RESAMPLING = (SHARC_UPDATE && (SHARC_PROPOGATION_DEPTH > 1));// resamples the cache during update step;

static constexpr auto SHARC_RESAMPLING_DEPTH_MIN = 1;// controls minimum path depth which can be used with cache resampling;

static constexpr auto SHARC_RADIANCE_SCALE = 1e3f;// scale used for radiance values accumulation. Each component uses 32-bit integer for data storage;

static constexpr auto SHARC_STALE_FRAME_NUM_MIN = 8;// minimum number of frames to keep the element in the cache;

static constexpr auto SHARC_DEBUG_BITS_OCCUPANCY_THRESHOLD_LOW = 0.125f;
static constexpr auto SHARC_DEBUG_BITS_OCCUPANCY_THRESHOLD_MEDIUM = 0.5f;

using HashGridIndex = Uint;
using HashGridKey = Uint64t;

}// namespace vision::inline sharc