//
// Created by Zero on 2024/2/18.
//

#include "base/sensor/frame_buffer.h"
#include "rhi/resources/shader.h"
#include "base/mgr/scene.h"
#include "base/mgr/pipeline.h"

namespace vision {

class NormalFrameBuffer : public FrameBuffer {
public:
    NormalFrameBuffer() = default;
    explicit NormalFrameBuffer(const FrameBufferDesc &desc)
        : FrameBuffer(desc) {}
    VS_MAKE_PLUGIN_NAME_FUNC

    RayState custom_generate_ray(const vision::Sensor *sensor, const SensorSample &ss) const noexcept override {
        return sensor->generate_ray(ss);
    }
};

}// namespace vision

VS_MAKE_CLASS_CREATOR_HOTFIX(vision, NormalFrameBuffer)