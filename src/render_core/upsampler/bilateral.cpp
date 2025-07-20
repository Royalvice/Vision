//
// Created by Zero on 2025/7/19.
//

#include "base/sensor/upsampler.h"
#include "rhi/resources/shader.h"

namespace vision {
using namespace ocarina;

//void bilateralFilter(const cv::Mat& input, cv::Mat& output, int d, double sigmaColor, double sigmaSpace) {
//    int halfD = d / 2;
//    output = cv::Mat::zeros(input.size(), input.type());
//
//    for (int y = 0; y < input.rows; y++) {
//        for (int x = 0; x < input.cols; x++) {
//            double sum = 0.0;
//            double sumWeight = 0.0;
//
//            for (int j = -halfD; j <= halfD; j++) {
//                for (int i = -halfD; i <= halfD; i++) {
//                    int neighborY = std::min(std::max(y + j, 0), input.rows - 1);
//                    int neighborX = std::min(std::max(x + i, 0), input.cols - 1);
//
//                    double spatialWeight = exp(-(i*i + j*j) / (2 * sigmaSpace * sigmaSpace));
//                    double colorWeight = exp(-pow(input.at<uchar>(y, x) - input.at<uchar>(neighborY, neighborX), 2) / (2 * sigmaColor * sigmaColor));
//                    double weight = spatialWeight * colorWeight;
//
//                    sum += weight * input.at<uchar>(neighborY, neighborX);
//                    sumWeight += weight;
//                }
//            }
//            output.at<uchar>(y, x) = sum / sumWeight;
//        }
//    }
//}

class BilateralUpsampler : public Upsampler {
private:
    ocarina::Shader<void(UpsamplingParam)> shader_;

public:
    BilateralUpsampler() = default;
    explicit BilateralUpsampler(const Desc &desc)
        : Upsampler(desc) {}
    VS_MAKE_PLUGIN_NAME_FUNC

    void compile() noexcept override {
        Kernel kernel = [&](UpsamplingParamVar param) {

        };
        shader_ = device().compile(kernel, "bilateral_sampling");
    }

    [[nodiscard]] CommandList apply(const vision::UpsamplingParam &param) const noexcept override {
        return {};
    }
};

}// namespace vision

VS_MAKE_CLASS_CREATOR_HOTFIX(vision, BilateralUpsampler)