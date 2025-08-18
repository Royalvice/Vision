//
// Created by Zero on 2025/7/16.
//

#pragma once

#include "base/node.h"
#include "base/encoded_object.h"
#include "rhi/common.h"
#include "integrator.h"

namespace vision {

class Pipeline;

class Sampler;

class RenderEnv;

class RadianceCache : public Node {
private:
    uint div_{5u};
    using IntegratorPtr = weak_ptr<IlluminationIntegrator>;
    IntegratorPtr integrator_{};

public:
    using Desc = RadianceCacheDesc;
    RadianceCache() = default;
    explicit RadianceCache(const Desc &desc)
        : Node(desc), div_(desc["div"].as_uint(5u)) {}
    OC_MAKE_MEMBER_SETTER(integrator)
    [[nodiscard]] IlluminationIntegrator *integrator() noexcept { return integrator_.lock().get(); }
    [[nodiscard]] const IlluminationIntegrator *integrator() const noexcept { return integrator_.lock().get(); }
    bool render_UI(ocarina::Widgets *widgets) noexcept override {
        return widgets->use_tree(ocarina::format("{} cache", impl_type().data()), [&] {
            render_sub_UI(widgets);
        });
    }
    void render_sub_UI(ocarina::Widgets *widgets) noexcept override {
        widgets->slider_uint("div", &div_, 1, 5);
    }
    virtual void update() noexcept = 0;
    virtual void resolve() noexcept = 0;
    virtual void compaction() noexcept = 0;
    virtual void query() noexcept = 0;
};

}// namespace vision