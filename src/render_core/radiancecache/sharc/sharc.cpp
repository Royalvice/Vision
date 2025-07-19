//
// Created by Zero on 2025/7/2.
//

#include "hash_grid.h"
#include "base/integral/radiance_cache.h"

namespace vision {

class SpatialHashRadianceCache : public RadianceCache {
public:
    SpatialHashRadianceCache() = default;
    explicit SpatialHashRadianceCache(const Desc &desc)
        : RadianceCache(desc) {}
    VS_MAKE_PLUGIN_NAME_FUNC
};

}// namespace vision