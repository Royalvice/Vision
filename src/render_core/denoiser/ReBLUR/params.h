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

}// namespace vision::inline reblur
