#include "engine/ninja_render.hpp"
#include "engine/ninja_blend_internal.hpp"
namespace sadx {
namespace {
std::uint32_t word(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}

// NOTSA: shared command publication after the caller has acquired its live device.
bool issue_on_device(NinjaRenderResult &result, NinjaRenderState &runtime,
                     const NinjaRenderDeviceHandle &device, std::uint32_t state,
                     std::uint32_t value, std::uint32_t call) {
    NinjaRenderStateCommand command{device, state, value, call};
    result.commands.push_back(command);
    const auto reject = [&](NinjaRenderFailure failure) {
        result.failure = failure;
        result.source_boundary = call;
        result.pending_command = command;
        return false;
    };
    if (!runtime.set_render_state)
        return reject(NinjaRenderFailure::missing_renderer);
    const auto receipt = runtime.set_render_state(command);
    if (!receipt.completed)
        return reject(NinjaRenderFailure::renderer_unavailable);
    result.ignored_hresults.push_back(receipt.original_hresult);
    return true;
}
}
bool ninja_issue_render_state(NinjaRenderResult &result, NinjaRenderState &runtime,
                              std::uint32_t state, std::uint32_t value,
                              NinjaRenderSourceSites sites) {
    auto reject = [&](NinjaRenderFailure f, std::uint32_t source) {
        result.failure = f;
        result.source_boundary = source;
        return false;
    };
    if (!runtime.device_at_3d128b0)
        return reject(NinjaRenderFailure::unknown_device, sites.device_load);
    if (!*runtime.device_at_3d128b0)
        return reject(NinjaRenderFailure::null_device, sites.device_dereference);
    return issue_on_device(result, runtime, *runtime.device_at_3d128b0, state, value, sites.call);
}
bool ninja_issue_texture_stage_state(NinjaRenderStageResult &result, NinjaRenderState &runtime,
                                     std::uint32_t stage, std::uint32_t state, std::uint32_t value,
                                     NinjaRenderSourceSites sites) {
    auto reject=[&](NinjaRenderFailure failure,std::uint32_t source){result.failure=failure;result.source_boundary=source;return false;};
    if(!runtime.device_at_3d128b0)return reject(NinjaRenderFailure::unknown_device,sites.device_load);
    if(!*runtime.device_at_3d128b0)return reject(NinjaRenderFailure::null_device,sites.device_dereference);
    NinjaRenderStageCommand command{*runtime.device_at_3d128b0,stage,state,value,sites.call};result.commands.push_back(command);
    if(!runtime.set_texture_stage_state){result.pending_command=command;return reject(NinjaRenderFailure::missing_renderer,sites.call);}
    const auto receipt=runtime.set_texture_stage_state(command);if(!receipt.completed){result.pending_command=command;return reject(NinjaRenderFailure::renderer_unavailable,sites.call);}
    result.ignored_hresults.push_back(receipt.original_hresult);return true;
}
std::optional<NinjaBlendTables> decode_ninja_blend_tables(const std::uint8_t* data,std::size_t size){
    if(!data||(size!=56&&size!=88))return {};
    NinjaBlendTables tables;for(unsigned i=0;i<2;++i)tables.states[i]=word(data+i*4);
    for(unsigned i=0;i<12;++i)tables.values[i]=word(data+8+i*4);
    if(size==88){std::array<std::uint32_t,4> following;for(unsigned i=0;i<4;++i)following[i]=word(data+56+i*4);tables.following_words_at_88ad9c=following;}
    return tables;
}
NinjaRenderResult ninja_set_blend(std::uint32_t selector, std::uint32_t value,
                                  NinjaRenderState &runtime) {
    NinjaRenderResult result;
    if (selector < 2) {
        if (!runtime.packed_blend_at_3ab9890) {
            result.failure = NinjaRenderFailure::unknown_packed_blend;
            result.source_boundary = selector == 0 ? 0x4030dd : 0x403103;
            return result;
        }
        auto &packed = *runtime.packed_blend_at_3ab9890;
        render_detail::store_packed_blend(selector, value, packed);
    }
    return ninja_set_blend_77ec60(selector, value, runtime);
}
NinjaRenderResult ninja_set_blend_77ec60(std::uint32_t selector, std::uint32_t value,
                                         NinjaRenderState &runtime) {
    NinjaRenderResult result;
    // NOTSA: source diagnostics remain at this legacy compatibility adapter.
    struct ReadStopped {};
    const auto stop = [&](NinjaRenderFailure failure, std::uint32_t boundary) {
        result.failure = failure;
        result.source_boundary = boundary;
        throw ReadStopped{};
    };
    const auto sites = [value](render_detail::BlendWrite write) -> NinjaRenderSourceSites {
        if (write == render_detail::BlendWrite::Source)
            return value == 0 ? NinjaRenderSourceSites{0x77ec68, 0x77ec6d, 0x77ec74}
                              : NinjaRenderSourceSites{0x77ec92, 0x77ec97, 0x77ec9e};
        if (write == render_detail::BlendWrite::Destination)
            return value == 0 ? NinjaRenderSourceSites{0x77ec7a, 0x77ec7f, 0x77ec86}
                              : NinjaRenderSourceSites{0x77eca4, 0x77eca9, 0x77ecb0};
        return {0x77ecbe, 0x77ecc3, 0x77ecd3};
    };
    try {
        render_detail::apply_device_blend(
            selector, value,
            [&](std::uint32_t mode) {
                if (!runtime.blend_tables)
                    stop(NinjaRenderFailure::missing_tables, 0x77ecb7);
                const auto &tables = *runtime.blend_tables;
                const auto count = tables.values.size();
                if (mode >= count + 4 || (mode >= count && !tables.following_words_at_88ad9c))
                    stop(NinjaRenderFailure::unsupported_value_index, 0x77ecb7);
                return mode < count ? tables.values[mode]
                                    : (*tables.following_words_at_88ad9c)[mode - count];
            },
            [&](std::uint32_t target) {
                if (target >= runtime.blend_tables->states.size())
                    stop(NinjaRenderFailure::unsupported_state_index, 0x77ecca);
                return runtime.blend_tables->states[target];
            },
            [&](render_detail::BlendWrite write) {
                const auto source = sites(write);
                const auto &current_device = runtime.device_at_3d128b0;
                if (!current_device)
                    stop(NinjaRenderFailure::unknown_device, source.device_load);
                if (!*current_device)
                    stop(NinjaRenderFailure::null_device, source.device_dereference);
                return *current_device;
            },
            [&](const NinjaRenderDeviceHandle &device, std::uint32_t setting, std::uint32_t mapped,
                render_detail::BlendWrite write) {
                return issue_on_device(result, runtime, device, setting, mapped, sites(write).call);
            });
    } catch (const ReadStopped &) {
        // Prior packed-byte stores and completed device writes remain published.
    }
    return result;
}
NinjaRenderResult ninja_set_depth_comparison(std::uint32_t value,
                                             const std::array<std::uint32_t, 8> *table,
                                             NinjaRenderState &runtime) {
    NinjaRenderResult result;
    if(!runtime.device_at_3d128b0){result.failure=NinjaRenderFailure::unknown_device;result.source_boundary=0x77ed04;return result;}
    if(!*runtime.device_at_3d128b0){result.failure=NinjaRenderFailure::null_device;result.source_boundary=0x77ed09;return result;}
    if(!table){result.failure=NinjaRenderFailure::missing_tables;result.source_boundary=0x77ed0e;return result;}
    ninja_issue_render_state(result,runtime,0x17,(*table)[value&7],{0x77ed04,0x77ed09,0x77ed19});return result;
}
NinjaRenderResult ninja_set_depth_write(std::uint32_t value,NinjaRenderState& runtime){
    NinjaRenderResult result;ninja_issue_render_state(result,runtime,0xe,value,{0x77ed24,0x77ed29,0x77ed2f});return result;
}
} // namespace sadx
