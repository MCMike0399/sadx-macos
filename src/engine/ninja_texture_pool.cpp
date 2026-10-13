#include "engine/ninja_texture_pool.hpp"
namespace sadx {
NinjaTexturePoolResult nj_init_texture_77f610(NinjaTexturePoolState& s,NinjaTextureSelectionState& selection,std::uint32_t records,std::uint32_t count){
    NinjaTexturePoolResult r;
    s.base_at_3d0f844=records;s.count_at_3d0f80c=count;
    for(std::uint32_t i=0;i<count;++i){ // count is unsigned (JBE/JB)
        const auto at=records+0x44*i;auto& b=s.records_at_3a8b0a0;
        const auto put=[&](std::uint32_t offset,std::uint64_t value,std::size_t width){
            if(!b.write(at+offset,value,width)){r.failure=NinjaTexturePoolFailure::records_outside_owner;r.source_boundary=0x77f633;r.required_fact=at+offset;return false;}return true;};
        // 77f633..77f669 store order: +0, +4 = -1; +8, +0xc, +0x10 = 0; +0x40 = 0; +0x42 = 0xffff; +0x14..+0x3c = 0.
        if(!put(0,0xffffffffu,4)||!put(4,0xffffffffu,4)||!put(8,0,4)||!put(0xc,0,4)||!put(0x10,0,4)||!put(0x40,0,2)||!put(0x42,0xffff,2))return r;
        for(std::uint32_t o=0x14;o<=0x3c;o+=4)if(!put(o,0,4))return r;
    }
    selection.current_list_at_3d0fa24=std::shared_ptr<NinjaTextureListOwner>{};
    s.word_at_3d0f830=0;s.byte_at_3d0f860=0;s.word_at_3d0fa14=2;s.word_at_3d0f960=0;
    for(auto& w:s.words_at_3d0f7c0)w=0;
    selection.requested_memory_at_3ce7128=std::shared_ptr<const NinjaTextureMemoryReference>{};
    return r;
}
} // namespace sadx
