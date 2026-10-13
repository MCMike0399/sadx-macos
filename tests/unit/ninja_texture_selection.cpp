#include "engine/ninja_texture_selection.hpp"
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
void require(bool v,const char* text){if(!v)throw std::runtime_error(text);}
std::shared_ptr<NinjaTextureMemoryReference> memory(std::uint32_t id=7,std::uint32_t format=0){auto m=std::make_shared<NinjaTextureMemoryReference>();m->global_id_at_0=id;m->palette_id_at_4=0xffffffffU;m->format_flags_at_20=format;m->source_surface_word_at_34=0x11110000;m->palette_version_at_42=0;return m;}
void tests(){
    const std::array<std::uint32_t,4> table{{1,0,1,0}};NinjaRenderState render;render.device_at_3d128b0=std::make_shared<NinjaRenderDevice>(NinjaRenderDevice{1});NinjaTextureSelectionState state;state.global_id_at_3d0fdf4=0;auto m=memory();NinjaTextureSelectionRuntime runtime{render,&table,nullptr,{}};
    auto result=ninja_select_cached_texture(m,state,runtime);require(result.failure==NinjaTextureSelectionFailure::missing_renderer&&result.pending_bind&&state.global_id_at_3d0fdf4==0&&!state.active_memory_at_3d129a0,"Missing binding owner does not fabricate cache stores");
    runtime.set_texture=[](const auto&){return NinjaRenderCallResult{true,-1};};result=ninja_select_cached_texture(m,state,runtime);require(result&&result.outcome==1&&state.global_id_at_3d0fdf4==7&&state.active_memory_at_3d129a0==m&&result.ignored_hresults[0]==-1,"Original ignores HRESULT and commits source cache after completed call");
    auto alias=memory(7);alias->source_surface_word_at_34.reset();result=ninja_select_cached_texture(alias,state,runtime);require(result&&result.binds.empty()&&state.active_memory_at_3d129a0==m,"Equal global ID preserves prior bound owner without reading new surface");
    m->global_id_at_0=8;m->source_surface_word_at_34=0;result=ninja_select_cached_texture(m,state,runtime);require(result&&result.binds.empty()&&state.global_id_at_3d0fdf4==7,"Known null surface does not update outer cache");
    auto list=std::make_shared<NinjaTextureListOwner>();list->count=0;list->entries=std::make_shared<NinjaTextureListOwner::Entries>(1);(**list->entries)[0]=alias;result=ninja_select_texture_list(list,state,runtime);require(result&&state.current_list_at_3d0fa24==list&&state.requested_memory_at_3ce7128==alias,"Source list selection ignores advertised count");
    result=ninja_select_texture_index(0x40000000U,state,runtime);require(result&&result.array_byte_offset==0&&result.selected_ordinal==0,"Original modulo32 12-byte addressing wraps to owned entry0");
    result=ninja_select_texture_index(0x15555556U,state,runtime);require(result.failure==NinjaTextureSelectionFailure::unsupported_array_word&&result.array_byte_offset==8,"Cross-field address is not guessed as a texmem slot");
    result=ninja_select_texture_index(0xffffffffU,state,runtime);require(result.failure==NinjaTextureSelectionFailure::unsupported_array_word&&result.array_byte_offset==0xfffffff4U,"Underflow byte address remains unmapped, not a guessed pointer");
    result=ninja_select_texture_index(1,state,runtime);require(result.failure==NinjaTextureSelectionFailure::array_short,"Aligned entry beyond owned array remains unavailable");
    auto broken=std::make_shared<NinjaTextureListOwner>();result=ninja_select_texture_list(broken,state,runtime);require(result.failure==NinjaTextureSelectionFailure::unknown_array&&state.current_list_at_3d0fa24==broken,"Current-list store precedes missing array failure");
    result=ninja_select_texture_list(std::shared_ptr<NinjaTextureListOwner>{},state,runtime);require(result&&result.outcome==1&&!*state.current_list_at_3d0fa24,"Known null list follows complete source setter return");result=ninja_select_texture_index(0,state,runtime);require(result&&result.outcome==0xffffffffU,"Getter's known-null list returns source minus1");
    NinjaTexturePaletteTable palette;for(auto& p:palette)p.id_at_0=99;palette[0].id_at_0=2;palette[0].version_at_c=3;runtime.palette_table_at_3d11f80=&palette;m->format_flags_at_20=0x28000000;m->palette_id_at_4=2;m->palette_version_at_42=0;m->source_surface_word_at_34=0x11110000;
    result=ninja_select_cached_texture(m,state,runtime);require(result.failure==NinjaTextureSelectionFailure::unknown_palette_colors&&m->palette_version_at_42==3,"Dirty palette version store survives missing color ownership");
    m->palette_version_at_42=0;palette[0].colors_at_8=std::make_shared<const std::array<std::uint32_t,256>>();result=ninja_select_cached_texture(m,state,runtime);require(result.failure==NinjaTextureSelectionFailure::palette_lock_unavailable&&result.source_boundary==0x78ce5a&&m->palette_version_at_42==3,"No successful palette lock placeholder; earlier version store retained");
    m->palette_version_at_42=3;result=ninja_select_cached_texture(m,state,runtime);require(result&&result.palette_index==0&&result.palette_changed==false,"Unchanged palette revision follows real cached path");
    std::vector<NinjaRenderStateCommand> commands;render.set_render_state=[&](const auto& c){commands.push_back(c);return NinjaRenderCallResult{true,-1};};auto states=ninja_prepare_texture_draw_states(render);require(states&&commands.size()==3&&commands[0].state==22&&commands[0].value==1&&commands[1].state==137&&commands[1].value==0&&commands[2].state==145&&commands[2].value==1&&states.ignored_hresults.size()==3,"Actual78b840 triplet retains ignored receipts");
}
}
int main(){try{tests();std::cout<<"Texture selection: source list/index/cache aliasing, partial stores, palette boundaries and render triplet passed.\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
