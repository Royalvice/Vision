//
// Created by Zero on 14/07/2025.
//

#include "base/sensor/frame_buffer.h"
#include "rhi/resources/shader.h"
#include "base/mgr/scene.h"
#include "base/mgr/pipeline.h"

namespace vision {

class LightFieldFrameBuffer : public FrameBuffer {

public:
    LightFieldFrameBuffer() = default;
    explicit LightFieldFrameBuffer(const FrameBufferDesc &desc)
        : FrameBuffer(desc) {}
    VS_MAKE_PLUGIN_NAME_FUNC

    [[nodiscard]] RayState custom_generate_ray(const vision::Sensor *sensor, const SensorSample &ss) const noexcept override {
        return sensor->generate_ray(ss);
    }

    [[nodiscard]] uint2 raytracing_resolution() const noexcept override {
        return {};
    }
};

}// namespace vision

VS_MAKE_CLASS_CREATOR_HOTFIX(vision, LightFieldFrameBuffer)