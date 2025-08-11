//
// Created by ling.zhu on 2025/8/6.
//

#pragma once

#include "math/basic_types.h"
#include "dsl/dsl.h"
#include "base/denoiser.h"
#include "params.h"

namespace vision ::inline reblur {

class ReBLUR : public Denoiser, public GBufferCallback, enable_shared_from_this<ReBLUR> {
};

}// namespace vision::inline reblur
