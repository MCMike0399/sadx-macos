#pragma once
#include "engine/ninja_queue_consumer.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace sadx {
// 128-entry fog table (game buffer 3b10f78, njGenerateFogTable3 output).
using NinjaFogTable=std::array<std::optional<float>,128>;
// Ninja fog device globals written by 794ec0 (device setup) and the fog-table
// setters. 3d12798 (fog on) and 3d128a4 (table fog usable) stay owned by
// NinjaQueueConsumerState; 3d0849c is the 787220/787230 mode word there.
struct NinjaFogDeviceState {
    std::optional<std::uint32_t> color_at_3d12898;
    std::optional<std::uint32_t> start_index_at_3d12950,end_index_at_3d0fce0;
    std::optional<float> start_value_at_3d0fac0,end_value_at_3d12894;
    std::optional<std::uint32_t> software_density_at_3d128a0; // no-table-fog fallback bits
    NinjaFogTable w_at_3d12a00,reciprocal_at_3d12c00; // per-entry: a stopped loop leaves later entries as they were
};
struct NinjaFogDeviceOwners {
    NinjaQueueConsumerState& state;
    NinjaFogDeviceState& device;
    NinjaRenderState* render;
};
// Source facts 794ec0 reads at device setup. RasterCaps is D3DCAPS8+0x24
// (3d127e4, bit 20 = D3DPRASTERCAPS_WFOG); 3d08ae0 is the 795930 flag word.
struct NinjaFogDeviceFacts {
    std::optional<std::uint32_t> raster_caps_from_3d127e4,flags_from_3d08ae0;
};
enum class NinjaFogFailure {none,unknown_fact,unlifted_callee,render_failed,fp_failed,unmodeled_operation,index_out_of_range};
struct NinjaFogResult {
    NinjaFogFailure failure=NinjaFogFailure::none;std::uint32_t source_boundary=0,callee_boundary=0;
    std::optional<std::uint32_t> required_fact;NinjaRenderResult render;std::optional<X87Failure> fp_failure;std::uint16_t fp_exceptions=0;
    std::optional<std::uint32_t> last_fp_site; // last x87 source site reached, for a caller's later diagnostics
    explicit operator bool()const noexcept{return failure==NinjaFogFailure::none;}
};
// Actual 794ec0: fog off (render state 28=0), table-fog capability, 3d12a00
// w table (CRT pow of exact powers of two), then 794db0(0x8010) -> 7949f0.
NinjaFogResult ninja_fog_initialize_794ec0(NinjaFogDeviceOwners&,const NinjaFogDeviceFacts&,X87Context&);
// Actual 7949b0 (787240): colour & 0xffffff, render state 34 when changed and supported.
NinjaFogResult ninja_fog_color_7949b0(std::uint32_t color,NinjaFogDeviceOwners&);
// Actual 787260: 794db0 (and its 7949f0 tail), then the 128-entry table.
NinjaFogResult ninja_fog_generate_table_787260(NinjaFogTable&,float start,float end,NinjaFogDeviceOwners&,X87Context&);
// Actual 794e40 (787250): table edges, then 7949f0.
NinjaFogResult ninja_fog_set_table_794e40(const NinjaFogTable&,NinjaFogDeviceOwners&,X87Context&);
// Actual fog switches 411af0 (enable) / 411b40 (disable) with 4119b0, 411a20,
// 794950/794980 (render state 28) and 787220/787230. Without a device owner,
// the 4119b0/411a20 table updates stop at their first fog-device call.
struct NinjaFogOwners {
    NinjaQueueConsumerState& state;
    std::optional<std::uint8_t>& flag_at_3abdcfe;
    const std::optional<std::int32_t>* display_switch_at_3b1117c;
    NinjaRenderState* render;
    float fog_sentinel_at_7dce50;
    NinjaFogDeviceState* device=nullptr;
    NinjaFogTable* table_at_3b10f78=nullptr;
    // 411a20's level fog: start 3abdc60, end 3abdc64, colour 3abdc68.
    const std::optional<float>* level_start_at_3abdc60=nullptr;
    const std::optional<float>* level_end_at_3abdc64=nullptr;
    const std::optional<std::uint32_t>* level_color_at_3abdc68=nullptr;
};
NinjaFogResult ninja_fog_411af0_411b40(bool enable,NinjaFogOwners&,X87Context&);
} // namespace sadx
