//
// Created by Zero on 16/12/2022.
//

#include "base/warper.h"
#include "core/stl.h"
#include "rhi/common.h"
#include "ocarina/src/core/image.h"
#include "ocarina/src/rhi/context.h"
#include "base/import/node_desc.h"
#include "base/node.h"

using namespace vision;
using namespace ocarina;

int main(int argc, char *argv[]) {
    fs::path path(argv[0]);
    RHIContext &file_manager = RHIContext::instance();
    Device device = file_manager.create_device("cuda");
    device.init_rtx();
    Stream stream = device.create_stream();
    file_manager.clear_cache();

    auto path1 = R"(E:/work/compile/vision/res/spruit_sunrise_2k.hdr)";

    auto image_io = Image::load(path1, LINEAR);
    auto image_out = device.create_texture(image_io.resolution(), image_io.pixel_storage());

    auto image = device.create_texture(image_io.resolution(), image_io.pixel_storage());
    stream << image.upload_sync(image_io.pixel_ptr());

    WarperDesc desc = WarperDesc("Warper");
    desc.sub_type = "alias";

    const DynamicModule *module = file_manager.obtain_module(desc.plugin_name());
    SP<Warper2D> warper2d = vision::Node::create_shared<vision::Warper2D>(desc);

    uint2 res = image_io.resolution();
    vector<float> weights(res.x * res.y, 0);
    image_io.for_each_pixel([&](const std::byte *pixel, int idx, PixelStorage pixel_storage) {
        float f = 0;
        float v = idx / res.y + 0.5f;
        float theta = v / res.x;
        float sinTheta = std::sin(Pi * theta);
        switch (pixel_storage) {
            case PixelStorage::FLOAT4: {
                float4 val = *(reinterpret_cast<const float4 *>(pixel));
                f = luminance(val.xyz());
                break;
            }
            case PixelStorage::BYTE4: {
                uchar4 val = *(reinterpret_cast<const uchar4 *>(pixel));
                float4 f4 = make_float4(val) / 255.f;
                f = luminance(f4.xyz());
                break;
            }
            default:
                break;
        }
        weights[idx] = f;
    });



    Kernel kernel = [&]() {
        uint2 res = image_io.resolution();
        Uint2 pixel = dispatch_idx().xy();
        Float u = cast<float>(pixel.x) / res.x;
        Float v = cast<float>(pixel.y) / res.y;
        Float2 uv = make_float2(u,v);
    };

    return 0;
}