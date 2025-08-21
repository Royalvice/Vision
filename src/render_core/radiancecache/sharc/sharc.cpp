//
// Created by Zero on 2025/7/2.
//

#include "sharc.h"
#include "base/integral/radiance_cache.h"

namespace vision {

class SpatialHashRadianceCache : public RadianceCache {
private:
    RegistrableBuffer<uint4> voxel_data_;
    RegistrableBuffer<uint4> prev_voxel_data_;
    RegistrableBuffer<ulong> hash_entries_;
    RegistrableBuffer<uint> copy_offset_;

    static constexpr size_t buffer_size = Pow<22>(2);

public:
    SpatialHashRadianceCache() = default;
    VS_MAKE_PLUGIN_NAME_FUNC
    explicit SpatialHashRadianceCache(const Desc &desc)
        : RadianceCache(desc) {
    }

    void prepare() noexcept override {
        voxel_data_.super() = device().create_buffer<uint4>(buffer_size, "Sharc::voxel_data_");
        prev_voxel_data_.super() = device().create_buffer<uint4>(buffer_size, "Sharc::prev_voxel_data_");
        hash_entries_.super() = device().create_buffer<ulong>(buffer_size, "Sharc::hash_entries_");
        copy_offset_.super() = device().create_buffer<uint>(buffer_size, "Sharc::copy_offset_");
    }

    void update() noexcept override {
    }

    void resolve() noexcept override {
    }

    void compaction() noexcept override {
    }

    void query() noexcept override {
    }
};

}// namespace vision

VS_MAKE_CLASS_CREATOR_HOTFIX(vision, SpatialHashRadianceCache)