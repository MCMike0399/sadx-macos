#include "game/game_controller.hpp"
#include <algorithm>
#include <cstring>
// Native 40f3a0 matching tools/verify_original_controller_init.py.
// in: 13 template dwords. out: 52 dwords of ControllersRaw, 4 pointer indexes, cleared-global flags.
extern "C" int sadx_native_controller_init(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=13||!out||capacity<60)return 0;
    sadx::GameLogicController source;for(unsigned w=0;w<13;++w)source.write_u32(w*4,in[w]);
    std::array<std::shared_ptr<sadx::GameLogicController>,4> raw;for(auto& r:raw){r=std::make_shared<sadx::GameLogicController>();for(unsigned w=0;w<13;++w)r->write_u32(w*4,0xcdcdcdcd);}
    sadx::GameLogicControllers pointers;sadx::GameControllerInitGlobals g;
    if(!sadx::init_controllers_40f3a0(source,raw,pointers,g))return 0;
    std::fill(out,out+capacity,0);
    for(unsigned r=0;r<4;++r)for(unsigned w=0;w<13;++w)out[r*13+w]=raw[r]->read_u32(w*4).value_or(0xdeadbeef);
    for(unsigned i=0;i<4;++i){unsigned index=99;for(unsigned r=0;r<4;++r)if(pointers[i]&&*pointers[i]==raw[r])index=r;out[52+i]=index;}
    bool cleared=true;for(auto w:g.words_at_3b0e768)cleared&=w==0u;for(auto w:g.words_at_3b0e344)cleared&=w==0u;for(auto w:g.words_at_3b0e9b8)cleared&=w==0u;for(auto b:g.bytes_at_3b0e9a4)cleared&=b==0;for(auto w:g.words_at_3b0e990)cleared&=w==0u;
    out[56]=cleared;return 1;
}
