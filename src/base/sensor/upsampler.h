//
// Created by Zero on 2025/7/19.
//

#pragma once

#include "dsl/dsl.h"
#include "base/node.h"
#include "hotfix/hotfix.h"

namespace vision {
struct UpsamplingParam {
    uint2 src_res;
    uint2 dst_res;
    BufferDesc<float4> src_buffer;
    BufferDesc<float4> dst_buffer;
};
}// namespace vision

OC_PARAM_STRUCT(vision, UpsamplingParam, src_res, dst_res,
                src_buffer, dst_buffer){};

namespace vision {

class Upsampler : public Node {
public:
    using Desc = UpsamplerDesc;
    Upsampler() = default;
    explicit Upsampler(const Desc &desc)
        : Node(desc) {}
    virtual void compile() noexcept = 0;
    [[nodiscard]] virtual CommandList apply(const UpsamplingParam &param) const noexcept = 0;
};

}// namespace vision