#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
namespace sadx {
// Native renderer identity. This is never a guest COM-pointer cast.
struct NinjaRenderDevice {std::uint64_t identity;};
using NinjaRenderDeviceHandle=std::shared_ptr<NinjaRenderDevice>;
// Source 88ad64: two state words, then twelve value words at 88ad6c. Actual
// 77ecb7 indexes values with an unchecked 4-bit nibble, so values 12..15 read
// the next contiguous words (the depth table at 88ad9c). Those are present only
// when the caller supplies that contiguous backing (88-byte decode); otherwise
// they remain an explicit boundary rather than a guessed value.
struct NinjaBlendTables {
    std::array<std::uint32_t,2> states;std::array<std::uint32_t,12> values;
    std::optional<std::array<std::uint32_t,4>> following_words_at_88ad9c;
};
// Accepts 56 bytes (88ad64..88ad9b) or 88 contiguous bytes (88ad64..88adbb).
std::optional<NinjaBlendTables> decode_ninja_blend_tables(const std::uint8_t*,std::size_t);
struct NinjaRenderStateCommand {
    NinjaRenderDeviceHandle device;
    std::uint32_t state,value,source_call;
};
struct NinjaRenderStageCommand {
    NinjaRenderDeviceHandle device;
    std::uint32_t stage,state,value,source_call;
};
struct NinjaRenderCallResult {bool completed;std::optional<std::int32_t> original_hresult;};
enum class NinjaRenderFailure {none,unknown_packed_blend,missing_tables,unsupported_state_index,unsupported_value_index,unknown_device,null_device,missing_renderer,renderer_unavailable};
struct NinjaRenderResult {
    NinjaRenderFailure failure=NinjaRenderFailure::none;
    std::uint32_t source_boundary=0;
    std::vector<NinjaRenderStateCommand> commands;
    // Source ignores SetRenderState's HRESULT, including failure HRESULTs.
    std::vector<std::optional<std::int32_t>> ignored_hresults;
    std::optional<NinjaRenderStateCommand> pending_command;
    explicit operator bool()const noexcept{return failure==NinjaRenderFailure::none;}
};
struct NinjaRenderStageResult {
    NinjaRenderFailure failure=NinjaRenderFailure::none;
    std::uint32_t source_boundary=0;
    std::vector<NinjaRenderStageCommand> commands;
    std::vector<std::optional<std::int32_t>> ignored_hresults;
    std::optional<NinjaRenderStageCommand> pending_command;
    explicit operator bool()const noexcept{return failure==NinjaRenderFailure::none;}
};
struct NinjaRenderSourceSites {std::uint32_t device_load,device_dereference,call;};
struct NinjaRenderState {
    std::optional<std::uint8_t> packed_blend_at_3ab9890;
    std::optional<NinjaRenderDeviceHandle> device_at_3d128b0;
    const NinjaBlendTables* blend_tables=nullptr;
    // Required real native renderer boundary. Incomplete return is unavailable;
    // original_hresult is omitted on native APIs without HRESULT; a test/source
    // receipt can be retained. completed never means a missing backend is okay.
    std::function<NinjaRenderCallResult(const NinjaRenderStateCommand&)> set_render_state;
    std::function<NinjaRenderCallResult(const NinjaRenderStageCommand&)> set_texture_stage_state;
};
// Shared native issue owner for the actual two D3D8 method contracts. Source
// producers supply verified instruction sites; both reread the live device.
bool ninja_issue_render_state(NinjaRenderResult&,NinjaRenderState&,std::uint32_t state,std::uint32_t value,NinjaRenderSourceSites);
bool ninja_issue_texture_stage_state(NinjaRenderStageResult&,NinjaRenderState&,std::uint32_t stage,std::uint32_t state,std::uint32_t value,NinjaRenderSourceSites);
NinjaRenderResult ninja_set_blend(std::uint32_t selector,std::uint32_t value,NinjaRenderState&); //4030d0→77ec60
NinjaRenderResult ninja_set_blend_77ec60(std::uint32_t selector,std::uint32_t value,NinjaRenderState&); //77ec60 alone (no 3ab9890 update)
NinjaRenderResult ninja_set_depth_comparison(std::uint32_t,const std::array<std::uint32_t,8>*,NinjaRenderState&); //77ed00
NinjaRenderResult ninja_set_depth_write(std::uint32_t,NinjaRenderState&); //77ed20
} // namespace sadx
