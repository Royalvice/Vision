//
// Created by Zero on 14/07/2025.
//

#include "base/frame_buffer.h"
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


};

}// namespace vision