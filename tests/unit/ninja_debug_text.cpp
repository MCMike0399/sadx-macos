#include "ninja/ninja_debug_text.hpp"
#include <iostream>
#include <stdexcept>
// 793e70: the 78c5a0 registration after the existing callbacks, and an unknown count.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    NinjaCallbackList list;list.count_at_3ce712c=2;list.slots_at_3d0fa40={0x77f110u,0x77efc0u};
    require(bool(run_debug_text_register_793e70(list))&&list.count_at_3ce712c==3u&&list.slots_at_3d0fa40[2]==0x78c5a0u,"appended after two callbacks");
    list.count_at_3ce712c.reset();require(run_debug_text_register_793e70(list).required_fact==0x3ce712cu,"unknown count");
    std::cout<<"Debug text 793e70: registration and unknown count passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
