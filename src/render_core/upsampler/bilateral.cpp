//
// Created by Zero on 2025/7/19.
//

#include "base/sensor/upsampler.h"

namespace vision {

class BilateralUpsampler : public Upsampler {
public:
    BilateralUpsampler() = default;
    BilateralUpsampler(const Desc &desc)
        : Upsampler(desc) {}
    VS_MAKE_PLUGIN_NAME_FUNC
    [[nodiscard]] CommandList apply(const vision::UpsamplingParam &param) const noexcept override {
        return {};
    }
};

}// namespace vision

VS_MAKE_CLASS_CREATOR_HOTFIX(vision, BilateralUpsampler)