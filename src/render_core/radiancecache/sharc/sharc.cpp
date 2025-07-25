//
// Created by Zero on 2025/7/2.
//

#include "sharc.h"
#include "base/integral/radiance_cache.h"

namespace vision {

class SpatialHashRadianceCache : public RadianceCache {
public:
    SpatialHashRadianceCache() = default;
    explicit SpatialHashRadianceCache(const Desc &desc)
        : RadianceCache(desc) {
      auto typ = Type::of<SharcState>();
      int i  =0;
    }
    VS_MAKE_PLUGIN_NAME_FUNC
};

}// namespace vision

VS_MAKE_CLASS_CREATOR_HOTFIX(vision, SpatialHashRadianceCache)