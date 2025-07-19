//
// Created by Zero on 2025/7/19.
//

#pragma once

#include "dsl/dsl.h"
#include "base/node.h"

namespace vision {
struct SuperResParam {
    uint2 src_res;
    uint2 dst_res;
    BufferDesc<float4> src_buffer;
    BufferDesc<float4> dst_buffer;
};
}// namespace vision

OC_PARAM_STRUCT(vision, SuperResParam, src_res, dst_res,
                src_buffer, dst_buffer){};

namespace vision {

class SuperResReconstructor : public Node {
public:
    using Desc = SuperResDesc;
    SuperResReconstructor() = default;
    explicit SuperResReconstructor(const Desc &desc)
        : Node(desc) {}

    [[nodiscard]] virtual CommandList apply(const SuperResParam &param) const noexcept = 0;
};

}// namespace vision