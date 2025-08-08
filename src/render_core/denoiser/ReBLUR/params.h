//
// Created by ling.zhu on 2025/8/6.
//

#pragma once

#include "math/basic_types.h"
#include "dsl/dsl.h"

namespace vision ::inline reblur {

// Sequence is based on "CommonSettings::frameIndex":
//     Even frame (0)  Odd frame (1)   ...
//         B W             W B
//         W B             B W
//     BLACK and WHITE modes define cells with VALID data
// Checkerboard can be only horizontal
// Notes:
//     - if checkerboarding is enabled, "mode" defines the orientation of even numbered frames
//     - all inputs have the same resolution - logical FULL resolution
//     - noisy input signals ("IN_DIFF_XXX / IN_SPEC_XXX") are tightly packed to the LEFT HALF of the texture (the input pixel = 2x1 screen pixel)
//     - for others the input pixel = 1x1 screen pixel
//     - upsampling will be handled internally in checkerboard mode
enum class CheckerboardMode : uint8_t {
    OFF,
    BLACK,
    WHITE,

    MAX_NUM
};

enum class AccumulationMode : uint8_t {
    // Common mode (accumulation continues normally)
    CONTINUE,

    // Discards history and resets accumulation
    RESTART,

    // Like RESTART, but additionally clears resources from potential garbage
    CLEAR_AND_RESTART,

    MAX_NUM
};

enum class HitDistanceReconstructionMode : uint8_t {
    // Probabilistic split at primary hit is not used, hence hit distance is always valid (reconstruction is not needed)
    OFF,

    // If hit distance is invalid due to probabilistic sampling, reconstruct using 3x3 neighbors.
    // Probability at primary hit must be clamped to [1/4; 3/4] range to guarantee a sample in this area.
    // White noise must be replaced with Bayer dithering to gurantee a sample in this area (see NRD sample)
    AREA_3X3,

    // If hit distance is invalid due to probabilistic sampling, reconstruct using 5x5 neighbors.
    // Probability at primary hit must be clamped to [1/16; 15/16] range to guarantee a sample in this area.
    // White noise must be replaced with Bayer dithering to gurantee a sample in this area (see NRD sample)
    AREA_5X5,

    MAX_NUM
};

}// namespace vision::inline reblur
