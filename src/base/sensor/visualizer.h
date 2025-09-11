//
// Created by Zero on 2024/9/21.
//

#pragma once

#include "core/stl.h"
#include "math/basic_types.h"
#include "base/node.h"
#include "dsl/dsl.h"
#include "UI/GUI.h"
#include "hotfix/hotfix.h"
#include "sensor.h"

namespace vision {

using namespace ocarina;

class Visualizer : public Context, public GUI, public RuntimeObject {
public:
    enum State {
        EOff,
        ERay,
        EGNormal,
        ESFrame,
    };

private:
    State state_{EOff};
    bool show_{false};
    float3 color_{make_float3(1, 0, 0)};
    int width_{0};
    mutable RegistrableList<LineSegment> line_segments_{};
    mutable RegistrableList<float3x4> shading_frames_{};
    mutable RegistrableList<LineSegment> geometry_normals_{};

public:
    Visualizer() = default;
    HOTFIX_VIRTUAL void init() noexcept;
    HOTFIX_VIRTUAL void draw(float4 *data) const noexcept;
    HOTFIX_VIRTUAL void draw_rays(float4 *data) const noexcept;
    HOTFIX_VIRTUAL void draw_frames(float4 *data) const noexcept;
    HOTFIX_VIRTUAL void draw_normals(float4 *data) const noexcept;
    HOTFIX_VIRTUAL void draw_line_segment(LineSegment ls, ocarina::float4 *data) const noexcept;
    HOTFIX_VIRTUAL LineSegment scaling(LineSegment ls) const noexcept;
    HOTFIX_VIRTUAL void write(int x, int y, float4 val, float4 *pixel) const noexcept;
    HOTFIX_VIRTUAL void add_line_segment(const Float3 &p0, const Float3 &p1) noexcept;
    HOTFIX_VIRTUAL void add_frame(const Interaction &it) noexcept;
    template<typename... Args>
    void condition_add_line_segment(Args &&...args) noexcept {
        $condition_execute {
            add_line_segment(OC_FORWARD(args)...);
        };
    }
    template<typename... Args>
    void condition_add_frame(Args &&...args) noexcept {
        $condition_execute {
            add_frame(OC_FORWARD(args)...);
        };
    }
    HOTFIX_VIRTUAL void clear() noexcept;
    OC_MAKE_MEMBER_GETTER(show, )
    VS_HOTFIX_MAKE_RESTORE(RuntimeObject, state_, show_, line_segments_,
                           shading_frames_, geometry_normals_, color_, width_)
    [[nodiscard]] Sensor *sensor() const noexcept;
    [[nodiscard]] uint2 resolution() const noexcept;
    bool render_UI(ocarina::Widgets *widgets) noexcept override;
    void render_sub_UI(ocarina::Widgets *widgets) noexcept override;
};

}// namespace vision
