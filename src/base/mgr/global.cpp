//
// Created by Zero on 2023/6/14.
//

#include "global.h"
#include "pipeline.h"
#include "hotfix/hotfix.h"

namespace vision {

OC_MAKE_INSTANCE_FUNC_DEF_WITH_HOTFIX(Global, s_global)

Global::~Global() {
    RHIContext::destroy_instance();
    ImagePool::destroy_instance();
}

void Global::set_pipeline(SP<Pipeline> pipeline) {
    pipeline_ = pipeline;
}

Pipeline *Global::pipeline() {
    return pipeline_.lock().get();
}

BindlessArray &Global::bindless_array() {
    return pipeline()->bindless_array();
}

void Global::set_scene_path(const fs::path &sp) noexcept {
    scene_path_ = sp;
}

fs::path Global::scene_path() const noexcept {
    return scene_path_;
}

fs::path Global::scene_cache_path() const noexcept {
    return scene_path_ / ".cache";
}

Pipeline *Context::pipeline() noexcept {
    return Global::instance().pipeline();
}

Device &Context::device() noexcept {
    return pipeline()->device();
}

Scene &Context::scene() noexcept {
    return pipeline()->scene();
}

FrameBuffer &Context::frame_buffer() noexcept {
    return *pipeline()->frame_buffer();
}

TSpectrum &Context::spectrum() noexcept {
    return scene().spectrum();
}

Stream &Context::stream() noexcept {
    return pipeline()->stream();
}

}// namespace vision