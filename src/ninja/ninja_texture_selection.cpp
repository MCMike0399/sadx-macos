#include "ninja/ninja_texture_selection.hpp"
namespace sadx {
namespace {
struct Boundary{};
struct Operation {
    NinjaTextureSelectionState& state;NinjaTextureSelectionRuntime& runtime;NinjaTextureSelectionResult result;
    std::shared_ptr<const NinjaTextureMemoryReference> memory;
    [[noreturn]]void stop(NinjaTextureSelectionFailure why,std::uint32_t site){result.failure=why;result.source_boundary=site;throw Boundary{};}
    template<class T>const T& need(const std::optional<T>& value,NinjaTextureSelectionFailure why,std::uint32_t site){if(!value)stop(why,site);return *value;}
    NinjaTextureBindCommand surface(std::uint32_t site){
        NinjaTextureBindCommand out;out.memory=memory;out.source_surface_word=memory->source_surface_word_at_34;out.surface_owner=memory->runtime_owner;
        if(out.source_surface_word){out.surface_is_null=*out.source_surface_word==0;if(out.surface_owner&&out.surface_is_null!=!(*out.surface_owner))stop(NinjaTextureSelectionFailure::surface_owner_conflict,site);}
        else if(out.surface_owner)out.surface_is_null=!*out.surface_owner;
        else stop(NinjaTextureSelectionFailure::unknown_surface,site);
        return out;
    }
    void bind(NinjaTextureBindCommand command,std::uint32_t load,std::uint32_t dereference,std::uint32_t call,std::uint32_t id){
        command.device=need(runtime.render.device_at_3d128b0,NinjaTextureSelectionFailure::unknown_device,load);
        if(!command.device)stop(NinjaTextureSelectionFailure::null_device,dereference);
        command.source_call=call;result.binds.push_back(command);
        if(!runtime.set_texture){result.pending_bind=command;stop(NinjaTextureSelectionFailure::missing_renderer,call);}
        const auto receipt=runtime.set_texture(command);
        if(!receipt.completed){result.pending_bind=command;stop(NinjaTextureSelectionFailure::renderer_unavailable,call);}
        result.ignored_hresults.push_back(receipt.original_hresult);
        state.global_id_at_3d0fdf4=id;state.active_memory_at_3d129a0=memory;
    }
    bool palette_check(){
        const auto flags=need(memory->format_flags_at_20,NinjaTextureSelectionFailure::unknown_format,0x78cdc9);
        const auto palette=need(memory->palette_id_at_4,NinjaTextureSelectionFailure::unknown_palette_id,0x78cdcc);
        const auto id=need(memory->global_id_at_0,NinjaTextureSelectionFailure::unknown_global_id,0x78cdd0);
        if(flags!=0x28000000)return false;
        auto selected_surface=surface(0x78cde8);
        if(palette!=0xffffffffU){
            if(!runtime.palette_table_at_3d11f80)stop(NinjaTextureSelectionFailure::unknown_palette_table,0x78cdf8);
            for(std::size_t i=0;i<128;++i){const auto& entry=(*runtime.palette_table_at_3d11f80)[i];const auto bank=need(entry.id_at_0,NinjaTextureSelectionFailure::unknown_palette_entry,0x78cdf8);
                if(bank!=palette)continue;result.palette_index=i;
                const auto version=need(entry.version_at_c,NinjaTextureSelectionFailure::unknown_palette_version,0x78ce37);
                const auto prior=need(memory->palette_version_at_42,NinjaTextureSelectionFailure::unknown_palette_version,0x78ce3b);
                if(version!=prior){
                    memory->palette_version_at_42=version; // exact store before palette/surface owners
                    const auto colors=need(entry.colors_at_8,NinjaTextureSelectionFailure::unknown_palette_colors,0x78ce4e);
                    if(selected_surface.surface_is_null)stop(NinjaTextureSelectionFailure::null_surface,0x78ce51);
                    // Actual surface lock/data writes are not replaced by success.
                    (void)colors;stop(NinjaTextureSelectionFailure::palette_lock_unavailable,0x78ce5a);
                }
                break;
            }
        }
        const auto cached=need(state.global_id_at_3d0fdf4,NinjaTextureSelectionFailure::unknown_cache_id,0x78ce08);
        if(cached!=id)bind(std::move(selected_surface),0x78ce10,0x78ce15,0x78ce1b,id);
        return false;
    }
    void cache(const std::optional<std::shared_ptr<const NinjaTextureMemoryReference>>& reference){
        memory=need(reference,NinjaTextureSelectionFailure::unknown_memory,0x78cf24);
        if(!memory)stop(NinjaTextureSelectionFailure::null_memory,0x78cf24);
        const auto id=need(memory->global_id_at_0,NinjaTextureSelectionFailure::unknown_global_id,0x78cf24);
        const auto changed=palette_check();result.palette_changed=changed;
        bool binding=changed;
        if(!changed)binding=need(state.global_id_at_3d0fdf4,NinjaTextureSelectionFailure::unknown_cache_id,0x78cf33)!=id;
        if(binding){auto selected_surface=surface(0x78cf3b);if(!selected_surface.surface_is_null)bind(std::move(selected_surface),0x78cf42,0x78cf48,0x78cf4e,id);}
        const auto format=need(memory->format_flags_at_20,NinjaTextureSelectionFailure::unknown_format,0x78cf60);
        if(!runtime.return_table_at_389d458)stop(NinjaTextureSelectionFailure::missing_return_table,0x78cf69);
        result.outcome=(*runtime.return_table_at_389d458)[(format>>27)&3U];
    }
    std::optional<std::shared_ptr<const NinjaTextureMemoryReference>> entry(const std::shared_ptr<NinjaTextureListOwner>& list,std::uint32_t offset,std::uint32_t load,std::uint32_t dereference){
        const auto array=need(list->entries,NinjaTextureSelectionFailure::unknown_array,load);
        if(!array)stop(NinjaTextureSelectionFailure::null_array,dereference);
        result.array_byte_offset=offset;
        if(offset%12)stop(NinjaTextureSelectionFailure::unsupported_array_word,dereference);
        const auto ordinal=std::size_t(offset/12);result.selected_ordinal=ordinal;
        if(ordinal>=array->size())stop(NinjaTextureSelectionFailure::array_short,dereference);
        const auto& selected=(*array)[ordinal];if(!selected)stop(NinjaTextureSelectionFailure::unknown_memory,dereference);
        return selected;
    }
};
}
NinjaTextureSelectionResult ninja_select_cached_texture(const std::optional<std::shared_ptr<const NinjaTextureMemoryReference>>& reference,NinjaTextureSelectionState& state,NinjaTextureSelectionRuntime& runtime){
    Operation op{state,runtime,{},{}};try{op.cache(reference);}catch(Boundary){}return op.result;
}
NinjaTextureSelectionResult ninja_select_texture_list(const std::optional<std::shared_ptr<NinjaTextureListOwner>>& reference,NinjaTextureSelectionState& state,NinjaTextureSelectionRuntime& runtime){
    Operation op{state,runtime,{},{}};try{
        const auto list=op.need(reference,NinjaTextureSelectionFailure::unknown_list,0x77f3d0);
        state.current_list_at_3d0fa24=list; // source store precedes every array/callee boundary
        if(list){const auto selected=op.entry(list,0,0x77f3da,0x77f3dc);state.requested_memory_at_3ce7128=selected;op.cache(selected);}
        op.result.outcome=1;
    }catch(Boundary){}return op.result;
}
NinjaTextureSelectionResult ninja_select_texture_index(std::uint32_t index,NinjaTextureSelectionState& state,NinjaTextureSelectionRuntime& runtime){
    Operation op{state,runtime,{},{}};try{
        const auto list=op.need(state.current_list_at_3d0fa24,NinjaTextureSelectionFailure::unknown_list,0x77f440);
        if(!list){op.result.outcome=0xffffffffU;return op.result;}
        const auto offset=index*12U;const auto selected=op.entry(list,offset,0x77f449,0x77f44e);state.requested_memory_at_3ce7128=selected;op.cache(selected);op.result.outcome=1;
    }catch(Boundary){}return op.result;
}
NinjaRenderResult ninja_prepare_texture_draw_states(NinjaRenderState& runtime){
    NinjaRenderResult r;constexpr std::array<std::uint32_t,3> loads{{0x78b840,0x78b852,0x78b867}},calls{{0x78b84c,0x78b861,0x78b876}},states{{22,137,145}},values{{1,0,1}};
    for(std::size_t i=0;i<3;++i){if(!runtime.device_at_3d128b0){r.failure=NinjaRenderFailure::unknown_device;r.source_boundary=loads[i];return r;}if(!*runtime.device_at_3d128b0){r.failure=NinjaRenderFailure::null_device;r.source_boundary=loads[i]+5;return r;}
        NinjaRenderStateCommand command{*runtime.device_at_3d128b0,states[i],values[i],calls[i]};r.commands.push_back(command);
        if(!runtime.set_render_state){r.failure=NinjaRenderFailure::missing_renderer;r.source_boundary=calls[i];r.pending_command=command;return r;}
        const auto receipt=runtime.set_render_state(command);if(!receipt.completed){r.failure=NinjaRenderFailure::renderer_unavailable;r.source_boundary=calls[i];r.pending_command=command;return r;}r.ignored_hresults.push_back(receipt.original_hresult);
    }
    return r;
}
} // namespace sadx
