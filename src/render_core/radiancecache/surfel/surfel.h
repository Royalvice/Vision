//
// Created by Zero on 2024/2/26.
//

#pragma once

#include "math/basic_types.h"
#include "dsl/dsl.h"
#include "core/stl.h"
#include "base/integral/radiance_cache.h"

namespace vision {

class SurfaceElementRadianceCache : public RadianceCache {
public:
    SurfaceElementRadianceCache() = default;
    explicit SurfaceElementRadianceCache(const Desc &desc)
        : RadianceCache(desc) {}
    VS_MAKE_PLUGIN_NAME_FUNC
};

}// namespace vision