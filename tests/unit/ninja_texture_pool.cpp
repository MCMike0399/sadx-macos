#include "ninja/ninja_texture_pool.hpp"
#include <iostream>
#include <stdexcept>
// 77f610: record initialisation, the pool words, the cleared selection owners and a count past the owner.
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    using namespace sadx;NinjaTexturePoolState s;NinjaTextureSelectionState sel;
    const auto r=nj_init_texture_77f610(s,sel,0x3a8b0a0,0x800);
    require(r&&s.base_at_3d0f844==0x3a8b0a0u&&s.count_at_3d0f80c==0x800u&&s.word_at_3d0fa14==2u&&sel.current_list_at_3d0fa24&&!*sel.current_list_at_3d0fa24,"pool words and cleared list");
    require(s.records_at_3a8b0a0.read(0x3a8b0a0,4)==0xffffffffu&&s.records_at_3a8b0a0.read(0x3a8b0a0+0x42,2)==0xffffu&&s.records_at_3a8b0a0.read(0x3a8b0a0+0x44*0x7ff+0x40,2)==0u,"record fields");
    NinjaTexturePoolState t;require(nj_init_texture_77f610(t,sel,0x3a8b0a0,0x801).required_fact==0x3a8b0a0u+0x44*0x800,"a count past the owned pool stops");
    std::cout<<"Texture pool 77f610: records, pool words, cleared selection owners and the owner bound passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
