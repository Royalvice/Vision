//
// Created by Zero on 2024/9/21.
//

#include "visualizer.h"
#include "base/sensor/sensor.h"
#include "base/mgr/pipeline.h"
#include "math/util.h"

namespace vision {

Sensor *Visualizer::sensor() const noexcept {
    return scene().sensor().get();
}

uint2 Visualizer::resolution() const noexcept {
    return pipeline()->resolution();
}

void Visualizer::init() noexcept {
#define ALLOCATE(member, Type, num)                                             \
    member.set_bindless_array(pipeline()->bindless_array());                    \
    member.set_list(device().create_list<Type>(10000, "Visualizer::" #member)); \
    member.register_self();

    ALLOCATE(line_segments_, LineSegment, 10000)
    ALLOCATE(frames_, float3x3, 100)

#undef ALLOCATE
    clear();
}

void Visualizer::clear() noexcept {
    line_segments_.clear_immediately();
    frames_.clear_immediately();
}

void Visualizer::write(int x, int y, ocarina::float4 val, ocarina::float4 *pixel) const noexcept {
    uint2 res = resolution();
    if (x >= res.x || x < 0 || y >= res.y || y < 0) {
        return;
    }
    uint index = y * res.x + x;
    pixel[index] = val;
}

void Visualizer::add_line_segment(const Float3 &p0, const Float3 &p1) noexcept {
    if (state_ != ERay) { return; }
    LineSegmentVar line_segment;
    line_segment.p0 = p0;
    line_segment.p1 = p1;
    line_segments_.push_back(line_segment);
}

void Visualizer::add_frame(const Interaction &it) noexcept {
    if (state_ != ENormal) { return; }
    Float3x3 mat = make_float3x3(it.shading.x, it.shading.y, it.shading.z);
    frames_.push_back(mat);
}

void Visualizer::draw_line_segments(ocarina::float4 *data) const noexcept {
    static vector<LineSegment> host;
    host.resize(line_segments_.capacity());
    stream() << line_segments_.storage_segment().download(host.data(), false);
    uint count = line_segments_.host_count();

    for (int index = 0; index < count; ++index) {
        LineSegment ls = host[index];
        ls = sensor()->clipping(ls);
        float2 p0 = sensor()->raster_coord(ls.p0).xy();
        float2 p1 = sensor()->raster_coord(ls.p1).xy();

        p0.x = ocarina::isnan(p0.x) ? resolution().x / 2 : p0.x;
        p0.y = ocarina::isnan(p0.y) ? resolution().y / 2 : p0.y;
        p1.x = ocarina::isnan(p1.x) ? resolution().x / 2 : p1.x;
        p1.y = ocarina::isnan(p1.y) ? resolution().y / 2 : p1.y;

        for (int i = -width_; i <= width_; ++i) {
            for (int j = -width_; j <= width_; ++j) {
                safe_line_bresenham(p0, p1 + make_float2(i, j), [&](int x, int y) {
                    write(x, y, make_float4(color_, 1), data);
                });
            }
        }
    }
}

void Visualizer::draw_frames(ocarina::float4 *data) const noexcept {
    static vector<float3x3> host;
    host.resize(frames_.capacity());
    stream() << frames_.storage_segment().download(host.data(), false);
    uint count = frames_.host_count();
}

void Visualizer::draw(ocarina::float4 *data) const noexcept {
    if (!show_) { return; }
    switch (state_) {
        case ERay:
            draw_line_segments(data);
            break;
        case ENormal:
            draw_frames(data);
            break;
        default:
            break;
    }
}

bool Visualizer::render_UI(ocarina::Widgets *widgets) noexcept {
    bool ret = widgets->use_folding_header("Visualizer", [&] {
#define visualize_macro(name)                              \
    if (widgets->radio_button(#name, state_ == E##name)) { \
        state_ = E##name;                                  \
    }
        visualize_macro(Off);
        visualize_macro(Ray);
        visualize_macro(Normal);
#undef visualize_macro
        render_sub_UI(widgets);
    });
    return ret;
}

void Visualizer::render_sub_UI(ocarina::Widgets *widgets) noexcept {
    widgets->button_click("clear", [&] {
        clear();
    });
    widgets->check_box("show", &show_);
    widgets->color_edit("color", &color_);
    widgets->input_int_limit("width", &width_, 0, 3);
}

}// namespace vision

VS_REGISTER_HOTFIX(vision, Visualizer)
VS_REGISTER_CURRENT_PATH(1, "vision-base.dll")