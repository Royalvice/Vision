//
// Created by Zero on 2024/2/18.
//

#include "frame_buffer.h"
#include "base/mgr/pipeline.h"

namespace vision {

void ScreenBuffer::update_resolution(ocarina::uint2 res, Device &device) noexcept {
    super().reset_all(device, res.x * res.y, name_);
    register_self();
}

FrameBuffer::FrameBuffer(const vision::FrameBufferDesc &desc)
    : Node(desc),
      tone_mapper_(desc.tone_mapper),
      resolution_(desc["resolution"].as_uint2(make_uint2(1280, 720))),
      screen_window_(make_float2(-1.f), make_float2(1.f)),
      exposure_(desc["exposure"].as_float(1.f)),
      accumulation_(uint(desc["accumulation"].as_bool(true))),
      upsampler_(Node::create_shared<Upsampler>(desc.upsampler_desc)) {
    visualizer_->init();
    update_screen_window();
    resize(resolution_);
}

void FrameBuffer::prepare() noexcept {
    encode_data();
    datas().reset_device_buffer_immediately(device());
    datas().register_self();
    datas().upload_immediately();

    prepare_view_buffer();
    prepare_screen_buffer(output_buffer_);
    prepare_rt_buffer();
    prepare_rays();
}

void FrameBuffer::update_runtime_object(const vision::IObjectConstructor *constructor) noexcept {
    std::tuple tp = {addressof(visualizer_), addressof(tone_mapper_.impl()), addressof(upsampler_)};
    HotfixSystem::replace_objects(constructor, tp);
}

bool FrameBuffer::render_UI(ocarina::Widgets *widgets) noexcept {
    bool ret = widgets->use_folding_header(
        ocarina::format("{} FrameBuffer", impl_type().data()),
        [&] {
            render_sub_UI(widgets);
        });
    ret |= visualizer_->render_UI(widgets);
    return ret;
}

void FrameBuffer::render_sub_UI(ocarina::Widgets *widgets) noexcept {
    auto show_buffer = [&](Managed<float4> &buffer) {
        if (buffer.device_buffer().size() == 0) {
            return;
        }
        if (widgets->radio_button(buffer.name(), cur_view_ == buffer.name())) {
            cur_view_ = buffer.name();
        }
    };
    changed_ |= widgets->drag_float("exposure", &exposure_.hv(), 0.01f, 0.f, 10.f);
    tone_mapper_->render_UI(widgets);
    for (auto iter = screen_buffers_.begin();
         iter != screen_buffers_.end(); ++iter) {
        show_buffer(*iter->second);
    }
}

Float4 FrameBuffer::apply_exposure(const Float &exposure, const Float4 &input) const noexcept {
    return 1.f - exp(-input * exposure);
}

Float4 FrameBuffer::apply_exposure(const Float4 &input) const noexcept {
    return apply_exposure(*exposure_, input);
}

void FrameBuffer::update_screen_window() noexcept {
    float ratio = resolution_.x * 1.f / resolution_.y;
    if (ratio > 1.f) {
        screen_window_.lower.x = -ratio;
        screen_window_.upper.x = ratio;
    } else {
        screen_window_.lower.y = -1.f / ratio;
        screen_window_.upper.y = 1.f / ratio;
    }
}

void FrameBuffer::init_screen_buffer(const SP<ScreenBuffer> &buffer) noexcept {
    buffer->reset_all(device(), pixel_num(), buffer->name());
    vector<float4> vec{};
    vec.assign(pixel_num(), float4{});
    buffer->set_bindless_array(bindless_array());
    buffer->register_self();
}

void FrameBuffer::prepare_screen_buffer(const SP<vision::ScreenBuffer> &buffer) noexcept {
    init_screen_buffer(buffer);
    register_(buffer);
}

void FrameBuffer::compile_accumulation() noexcept {
    Kernel kernel = [&](BufferVar<float4> input, BufferVar<float4> output, Uint frame_index) {
        Float4 accum_prev = output.read(dispatch_id());
        Float4 val = input.read(dispatch_id());
        Float a = 1.f / (frame_index + 1);
        val = lerp(make_float4(a), accum_prev, val);
        output.write(dispatch_id(), val);
    };
    accumulate_ = device().compile(kernel, "RGBFilm-accumulation");
}

void FrameBuffer::update_device_data() noexcept {
    if (has_changed()) {
        update_data();
        EncodedObject::upload_immediately();
    }
}

void FrameBuffer::compile_tone_mapping() noexcept {
    Kernel kernel = [&](BufferVar<float4> input, BufferVar<float4> output, Float exposure) {
        Float4 val = input.read(dispatch_id());
        val = apply_exposure(exposure, val);
        val = tone_mapper_->apply(val);
        val.w = 1.f;
        output.write(dispatch_id(), val);
    };
    tone_mapping_ = device().compile(kernel, "RGBFilm-tone_mapping");
}

void FrameBuffer::compile_gamma() noexcept {
    Kernel kernel = [&](BufferVar<float4> input, BufferVar<float4> output) {
        Float4 val = input.read(dispatch_id());
        val = linear_to_srgb(val);
        val.w = 1.f;
        output.write(dispatch_id(), val);
    };
    gamma_correct_ = device().compile(kernel, "FrameBuffer-gamma_correction");
}

void FrameBuffer::compile_compute_geom() noexcept {
    TSensor &camera = scene().sensor();
    TSampler &sampler = scene().sampler();
    TLightSampler &light_sampler = scene().light_sampler();
    Kernel kernel = [&](Var<GBufferParam> param) {
        auto &frame_index = param.frame_index;
        auto &gbuffer = param.gbuffer;
        auto &motion_vectors = param.motion_vectors;
        auto &albedo_buffer = param.albedo_buffer;
        auto &emission_buffer = param.emission_buffer;
        auto &normal_buffer = param.normal_buffer;
        auto &rays = param.rays;
        RenderEnv render_env;
        render_env.initial(sampler, frame_index, spectrum());
        Uint2 pixel = dispatch_idx().xy();
        sampler->load_data();
        sampler->start(pixel, frame_index, 0);
        camera->load_data();

        const SampledWavelengths &swl = render_env.sampled_wavelengths();

        SensorSample ss = sampler->sensor_sample(pixel, camera->filter());
        RayState rs = camera->generate_ray(ss);
        TriangleHitVar hit = pipeline()->trace_closest(rs.ray);
        RayDataVar ray_data;
        ray_data->from_ray_state(rs);
        rays.write(dispatch_id(), ray_data);

        Float2 motion_vec = make_float2(0.f);

        PixelGeometryVar geom;
        geom.p_film = ss.p_film;
        Float3 albedo;
        Float3 emission;
        $if(hit->is_hit()) {
            Interaction it = pipeline()->compute_surface_interaction(hit, rs.ray, true);
            geom->set_normal(it.shading.normal());
            normal_buffer.write(dispatch_id(), make_float4(it.shading.normal(), 1.f));
            geom.linear_depth = camera->linear_depth(it.pos);
            $if(it.has_material()) {
                scene().materials().dispatch(it.material_id(), [&](const Material *material) {
                    MaterialEvaluator bsdf = material->create_evaluator(it, swl);
                    albedo = spectrum()->linear_srgb(bsdf.albedo(it.wo), swl);
                });
            };
            $if(it.has_emission()) {
                LightSampleContext p_ref;
                p_ref.pos = rs.origin();
                p_ref.ng = rs.direction();
                LightEval eval = light_sampler->evaluate_hit_wi(p_ref, it, swl);
                emission = spectrum()->linear_srgb(eval.L, swl);
            };
            motion_vec = compute_motion_vec(camera, ss.p_film, it.pos, true);
        };
        gbuffer.write(dispatch_id(), geom);
        motion_vectors.write(dispatch_id(), motion_vec);
        albedo_buffer.write(dispatch_id(), make_float4(albedo, 1.f));
        emission_buffer.write(dispatch_id(), make_float4(emission, 1.f));
    };
    compute_geom_ = device().compile(kernel, "rt_geom");
}

void FrameBuffer::compute_gradient(PixelGeometryVar &center_data,
                                   const BufferVar<PixelGeometry> &gbuffer) const noexcept {
    Uint x_sample_num = 0u;
    Uint y_sample_num = 0u;
    Float3 normal_dx = make_float3(0.f);
    Float3 normal_dy = make_float3(0.f);

    Float depth_dx = 0.f;
    Float depth_dy = 0.f;

    Uint2 center = dispatch_idx().xy();
    foreach_neighbor(dispatch_idx().xy(), [&](const Int2 &pixel) {
        Uint index = dispatch_id(pixel);
        PixelGeometryVar neighbor_data = gbuffer.read(index);
        $if(center.x > pixel.x) {
            x_sample_num += 1;
            normal_dx += center_data.normal_fwidth.xyz() - neighbor_data.normal_fwidth.xyz();
            depth_dx += center_data.linear_depth - neighbor_data.linear_depth;
        }
        $elif(pixel.x > center.x) {
            x_sample_num += 1;
            normal_dx += neighbor_data.normal_fwidth.xyz() - center_data.normal_fwidth.xyz();
            depth_dx += neighbor_data.linear_depth - center_data.linear_depth;
        };

        $if(center.y > pixel.y) {
            y_sample_num += 1;
            normal_dy += center_data.normal_fwidth.xyz() - neighbor_data.normal_fwidth.xyz();
            depth_dy += center_data.linear_depth - neighbor_data.linear_depth;
        }
        $elif(pixel.y > center.y) {
            y_sample_num += 1;
            normal_dy += neighbor_data.normal_fwidth.xyz() - center_data.normal_fwidth.xyz();
            depth_dy += neighbor_data.linear_depth - center_data.linear_depth;
        };
    });
    normal_dx /= cast<float>(x_sample_num);
    normal_dy /= cast<float>(y_sample_num);
    Float3 normal_fwidth = abs(normal_dx) + abs(normal_dy);
    center_data->set_normal_fwidth(length(normal_fwidth));

    depth_dx /= x_sample_num;
    depth_dy /= y_sample_num;
    center_data.depth_gradient = abs(depth_dx) + abs(depth_dy);
}

void FrameBuffer::compile_compute_grad() noexcept {
    Kernel kernel = [&](Var<GradParam> param) {
        auto &gbuffer = param.gbuffer;
        PixelGeometryVar center_data = gbuffer.read(dispatch_id());
        compute_gradient(center_data, gbuffer);
        gbuffer.write(dispatch_id(), center_data);
    };
    compute_grad_ = device().compile(kernel, "rt_gradient");
}

void FrameBuffer::compile_compute_hit() noexcept {
    TSensor &camera = scene().sensor();
    TSampler &sampler = scene().sampler();
    Kernel kernel = [&](BufferVar<TriangleHit> hit_buffer, Uint frame_index) {
        Uint2 pixel = dispatch_idx().xy();
        sampler->start(pixel, frame_index, 0);
        camera->load_data();

        SensorSample ss = sampler->sensor_sample(pixel, camera->filter());
        RayState rs = camera->generate_ray(ss);
        TriangleHitVar hit = pipeline()->trace_closest(rs.ray);
        hit_buffer.write(dispatch_id(), hit);
    };
    compute_hit_ = device().compile(kernel, "rt_compute_pixel");
}

void FrameBuffer::compile() noexcept {
    compile_gamma();
    compile_compute_geom();
    compile_compute_grad();
    compile_compute_hit();
    compile_accumulation();
    compile_tone_mapping();
}

CommandList FrameBuffer::compute_hit(uint frame_index) const noexcept {
    CommandList ret;
    ret << compute_hit_(hit_buffer_, frame_index).dispatch(resolution());
    return ret;
}

CommandList FrameBuffer::compute_geom(const vision::GBufferParam &param) const noexcept {
    CommandList ret;
    ret << compute_geom_(param).dispatch(resolution());
    return ret;
}

CommandList FrameBuffer::compute_GBuffer(const GBufferParam &param,
                                         const GradParam &grad_param) const noexcept {
    CommandList ret;
    ret << compute_geom(param);
    ret << compute_grad(grad_param);
    return ret;
}

CommandList FrameBuffer::compute_GBuffer(uint frame_index) const noexcept {
    GBufferParam param;

    auto gbuffer = cur_gbuffer_view(frame_index).descriptor();

    param.frame_index = frame_index;
    param.gbuffer = gbuffer;
    param.motion_vectors = motion_vectors().descriptor();
    param.albedo_buffer = albedo().descriptor();
    param.normal_buffer = normal().descriptor();
    param.emission_buffer = emission().descriptor();
    param.rays = rays().descriptor();

    GradParam grad_param;
    grad_param.frame_index = frame_index;
    grad_param.gbuffer = gbuffer;
    return compute_GBuffer(param, grad_param);
}

CommandList FrameBuffer::compute_grad(const vision::GradParam &param) const noexcept {
    CommandList ret;
    ret << compute_grad_(param).dispatch(resolution());
    return ret;
}

CommandList FrameBuffer::gamma_correct(BufferView<float4> input,
                                       BufferView<float4> output) const noexcept {
    CommandList ret;
    ret << gamma_correct_(input,
                          output)
               .dispatch(resolution());
    return ret;
}

CommandList FrameBuffer::accumulate(BufferView<float4> input, BufferView<float4> output,
                                    uint frame_index) const noexcept {
    CommandList ret;
    ret << accumulate_(input,
                       output,
                       frame_index)
               .dispatch(resolution());
    return ret;
}

CommandList FrameBuffer::tone_mapping(BufferView<float4> input,
                                      BufferView<float4> output) const noexcept {
    CommandList ret;
    ret << tone_mapping_(input,
                         output, exposure_.hv())
               .dispatch(resolution());
    return ret;
}

CommandList FrameBuffer::gamma_correct() const noexcept {
    const Buffer<float4> &input = cur_screen_buffer();
    return gamma_correct(input, view_buffer_);
}

Float3 FrameBuffer::add_sample(const Uint2 &pixel, Float4 val, const Uint &frame_index) noexcept {
    Float a = 1.f / (frame_index + 1);
    Uint index = dispatch_id(pixel);
    val = Env::instance().zero_if_nan_inf(val);
    if (accumulation_.hv()) {
        Float4 accum_prev = rt_buffer_.read(index);
        val = lerp(make_float4(a), accum_prev, val);
    }
    rt_buffer_.write(index, val);
    val = apply_exposure(val);
    val = tone_mapper_->apply(val);
    val.w = 1.f;
    output_buffer_->write(index, val);
    return val.xyz();
}

void FrameBuffer::register_(const SP<ScreenBuffer> &buffer) noexcept {
    auto iter = screen_buffers_.find(buffer->name());
    if (iter != screen_buffers_.end()) {
        OC_ERROR("");
    }
    screen_buffers_.insert(std::make_pair(buffer->name(), buffer));
}

void FrameBuffer::unregister(const SP<ScreenBuffer> &buffer) noexcept {
    unregister(buffer->name());
}

void FrameBuffer::unregister(const std::string &name) noexcept {
    auto iter = screen_buffers_.find(name);
    if (iter == screen_buffers_.end()) {
        OC_ERROR("");
    }
    screen_buffers_.erase(iter);
}

BindlessArray &FrameBuffer::bindless_array() const noexcept {
    return pipeline()->bindless_array();
}

uint FrameBuffer::pixel_index(uint2 pos) const noexcept {
    return pos.y * resolution().x + pos.x;
}

void FrameBuffer::fill_window_buffer(const Buffer<ocarina::float4> &input) noexcept {
    input.download_immediately(window_buffer_.data());
    visualizer_->draw(window_buffer_.data());
}

void FrameBuffer::resize(ocarina::uint2 res) noexcept {
    window_buffer_.resize(res.x * res.y, make_float4(0.f));
    resolution_ = res;
}

void FrameBuffer::update_resolution(ocarina::uint2 res) noexcept {
    resize(res);
    reset_surfaces();
    reset_surface_exts();
    reset_motion_vectors();
    reset_hit_bsdfs();
    reset_emission();
    reset_albedo();
    reset_normal();
    reset_rt_buffer();
    reset_accumulation_buffer();
    reset_rays();
    reset_gbuffer();
    reset_hit_buffer();
    reset_view_buffer();
    update_screen_window();
    for (auto &it : screen_buffers_) {
        it.second->update_resolution(res, device());
    }
}

uint FrameBuffer::pixel_num() const noexcept {
    return resolution_.x * resolution_.y;
}

uint2 FrameBuffer::resolution() const noexcept {
    return resolution_;
}

BindlessArray &FrameBuffer::bindless_array() noexcept {
    return pipeline()->bindless_array();
}

void FrameBuffer::after_render() noexcept {
    fill_window_buffer(view_buffer_);
}

const Buffer<float4> &FrameBuffer::cur_screen_buffer() const noexcept {
    return screen_buffers_.at(cur_view_)->device_buffer();
}

Uint FrameBuffer::checkerboard_value(const Uint2 &coord) noexcept {
    return (coord.x & 1) ^ (coord.y & 1);
}

Uint FrameBuffer::checkerboard_value(const Uint2 &coord, const Uint &frame_index) noexcept {
    return checkerboard_value(coord) ^ (frame_index & 1);
}

Float2 FrameBuffer::compute_motion_vec(const TSensor &camera, const Float2 &p_film,
                                       const Float3 &cur_pos, const Bool &is_hit) noexcept {
    Float2 ret = make_float2(0.f);
    $if(is_hit) {
        Float2 raster_coord = camera->prev_raster_coord(cur_pos).xy();
        ret = p_film - raster_coord;
    };
    return ret;
}

Float3 FrameBuffer::compute_motion_vector(const TSensor &camera, const Float2 &p_film,
                                          const Uint &frame_index) const noexcept {
    Uint2 pixel = make_uint2(p_film);
    Uint pixel_index = dispatch_id(pixel);
    SurfaceDataVar cur_surf = cur_surfaces_var(frame_index).read(pixel_index);
    SurfaceDataVar prev_surf = prev_surfaces_var(frame_index).read(pixel_index);
    return compute_motion_vector(camera, cur_surf->position(), prev_surf->position());
}

Float3 FrameBuffer::compute_motion_vector(const TSensor &camera, const Float3 &cur_pos,
                                          const Float3 &pre_pos) const noexcept {
    Float3 cur_coord = camera->raster_coord(cur_pos);
    Float3 prev_coord = camera->raster_coord(pre_pos);
    return prev_coord - cur_coord;
}
}// namespace vision