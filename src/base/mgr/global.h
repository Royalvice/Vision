//
// Created by Zero on 2023/6/14.
//

#pragma once

#include "rhi/context.h"
#include "image_pool.h"
#include "rhi/common.h"

namespace vision {
class Pipeline;
class Spectrum;
class Global {
    OC_MAKE_INSTANCE_CONSTRUCTOR(Global, s_global)
    ~Global();

private:
    weak_ptr<Pipeline> pipeline_{};
    Device *device_{nullptr};
    fs::path scene_path_;

public:
    OC_MAKE_INSTANCE_FUNC_DECL(Global)
    void set_pipeline(SP<Pipeline> pipeline);
    [[nodiscard]] Pipeline *pipeline();
    [[nodiscard]] ImagePool &image_pool() {
        return ImagePool::instance();
    }
    [[nodiscard]] Device &device() noexcept { return *device_; }
    void set_device(Device *val) noexcept { device_ = val; }
    [[nodiscard]] BindlessArray &bindless_array();
    void set_scene_path(const fs::path &sp) noexcept;
    [[nodiscard]] fs::path scene_path() const noexcept;
    [[nodiscard]] fs::path scene_cache_path() const noexcept;
    [[nodiscard]] static decltype(auto) file_manager() {
        return RHIContext::instance();
    }
};

class FrameBuffer;

class Spectrum;
template<typename impl_t, typename desc_t>
class TObject;
using TSpectrum = TObject<Spectrum, SpectrumDesc>;

class Context {
protected:
    Context() = default;

public:
    [[nodiscard]] static Device &device() noexcept;
    [[nodiscard]] static Stream &stream() noexcept;
    [[nodiscard]] static Pipeline *pipeline() noexcept;
    [[nodiscard]] static Scene &scene() noexcept;
    [[nodiscard]] static FrameBuffer &frame_buffer() noexcept;
    [[nodiscard]] static TSpectrum &spectrum() noexcept;
};

}// namespace vision