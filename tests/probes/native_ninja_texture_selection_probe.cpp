#include "engine/ninja_texture_selection.hpp"
namespace {
std::uint32_t identity(const std::optional<std::shared_ptr<const sadx::NinjaTextureMemoryReference>>& r,const std::array<std::shared_ptr<sadx::NinjaTextureMemoryReference>,3>& m){if(!r)return 0xfffffffeU;if(!*r)return 0;for(std::size_t i=0;i<3;++i)if(r->get()==m[i].get())return i+1;return 0xffffffffU;}
}
extern "C" int sadx_native_ninja_texture_selection(const std::uint32_t* w,std::uint32_t* out){
    if(!w||!out)return -1;std::array<std::shared_ptr<sadx::NinjaTextureMemoryReference>,3> memories;
    for(std::size_t i=0;i<3;++i){auto m=std::make_shared<sadx::NinjaTextureMemoryReference>();const auto* a=w+16+i*6;const auto mask=a[5];if(mask&1)m->global_id_at_0=a[0];if(mask&2)m->palette_id_at_4=a[1];if(mask&4)m->format_flags_at_20=a[2];if(mask&8)m->source_surface_word_at_34=a[3];if(mask&16)m->palette_version_at_42=static_cast<std::uint16_t>(a[4]);memories[i]=m;}
    auto list=std::make_shared<sadx::NinjaTextureListOwner>();list->count=w[4];if(w[2]!=4){if(w[2]==3)list->entries=std::shared_ptr<sadx::NinjaTextureListOwner::Entries>{};else{list->entries=std::make_shared<sadx::NinjaTextureListOwner::Entries>(w[41]);for(std::size_t i=0;i<w[41]&&i<3;++i)(**list->entries)[i]=memories[i];if(w[2]==5&&w[41])(**list->entries)[0].reset();}}
    std::optional<std::shared_ptr<sadx::NinjaTextureListOwner>> selected=list;if(w[2]==0)selected=std::shared_ptr<sadx::NinjaTextureListOwner>{};if(w[2]==2)selected.reset();
    sadx::NinjaTextureSelectionState state;state.current_list_at_3d0fa24=w[0]==1?std::optional<std::shared_ptr<sadx::NinjaTextureListOwner>>{std::shared_ptr<sadx::NinjaTextureListOwner>{}}:selected;if(w[12])state.global_id_at_3d0fdf4=w[5];state.requested_memory_at_3ce7128=w[46]?std::shared_ptr<const sadx::NinjaTextureMemoryReference>{memories[(w[46]-1)%3]}:std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};state.active_memory_at_3d129a0=w[47]?std::shared_ptr<const sadx::NinjaTextureMemoryReference>{memories[(w[47]-1)%3]}:std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};
    sadx::NinjaRenderState render;if(w[8]!=2)render.device_at_3d128b0=w[8]?std::make_shared<sadx::NinjaRenderDevice>(sadx::NinjaRenderDevice{1}):sadx::NinjaRenderDeviceHandle{};
    const std::array<std::uint32_t,4> values{{1,0,1,0}};sadx::NinjaTexturePaletteTable palette;for(std::size_t i=0;i<128;++i)palette[i].id_at_0=w[34]+i;if(w[42]&1)palette[0].id_at_0=w[35];else palette[0].id_at_0.reset();if(w[42]&2)palette[0].version_at_c=static_cast<std::uint16_t>(w[36]);if(w[37]==1)palette[0].colors_at_8=std::make_shared<const std::array<std::uint32_t,256>>();if(w[37]==2)palette[0].colors_at_8=std::shared_ptr<const std::array<std::uint32_t,256>>{};
    std::vector<std::array<std::uint32_t,3>> calls;const auto receipt=[&](std::uint32_t a,std::uint32_t b,std::uint32_t c){calls.push_back({a,b,c});std::int32_t hr;const auto bits=w[11];__builtin_memcpy(&hr,&bits,4);return sadx::NinjaRenderCallResult{w[9]!=2,hr};};
    sadx::NinjaTextureSelectionRuntime runtime{render,w[40]?&values:nullptr,w[39]?&palette:nullptr,{}};
    if(w[9]!=1){runtime.set_texture=[&](const auto& c){return receipt(0,c.stage,c.source_surface_word?*c.source_surface_word:0);};render.set_render_state=[&](const auto& c){return receipt(1,c.state,c.value);};}
    sadx::NinjaTextureSelectionResult result;
    if(w[0]==0)result=sadx::ninja_select_cached_texture(memories[w[3]%3],state,runtime);
    else if(w[0]==1)result=sadx::ninja_select_texture_list(selected,state,runtime);
    else if(w[0]==2)result=sadx::ninja_select_texture_index(w[1],state,runtime);
    else{auto r=sadx::ninja_prepare_texture_draw_states(render);out[0]=static_cast<unsigned>(r.failure);out[1]=r.source_boundary;out[2]=static_cast<bool>(r);out[3]=0;}
    if(w[0]!=3){out[0]=static_cast<unsigned>(result.failure);out[1]=result.source_boundary;out[2]=static_cast<bool>(result);out[3]=result.outcome?*result.outcome:0;}
    out[4]=!state.current_list_at_3d0fa24?0xfffffffeU:!*state.current_list_at_3d0fa24?0:1;out[5]=state.global_id_at_3d0fdf4?*state.global_id_at_3d0fdf4:0;out[6]=identity(state.requested_memory_at_3ce7128,memories);out[7]=identity(state.active_memory_at_3d129a0,memories);for(std::size_t i=0;i<3;++i)out[8+i]=memories[i]->palette_version_at_42?*memories[i]->palette_version_at_42:0xffff;
    out[11]=calls.size();for(std::size_t i=0;i<calls.size();++i)for(std::size_t k=0;k<3;++k)out[12+i*3+k]=calls[i][k];return 0;
}
extern "C" const char* sadx_native_ninja_texture_selection_compiler(){return "Clang " __clang_version__ "; ARM64 C++17 O2 original texture list/index/cache and render triplet";}
