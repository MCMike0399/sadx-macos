#include "engine/ninja_keyboard.hpp"
#include <iostream>
#include <stdexcept>
// Owner semantics the oracle does not compare: unknown device, unsupported record
// addressing, missing device owners and the Acquire retry loop.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(){try{
    NinjaKeyboardTable table{};table.kind.fill(2);table.kind[0x1e]=0;table.code[0x1e]=0x04;table.kind[0x2a]=1;table.code[0x2a]=0x02;
    {NinjaKeyboardState s;const auto r=ninja_keyboard_poll_791bd0(s,table);require(r.failure==NinjaKeyboardFailure::unknown_fact&&r.required_fact==0x3d08820u,"unknown device");}
    {NinjaKeyboardState s;NinjaCallbackList list;list.count_at_3ce712c=2;std::array<std::uint8_t,256> dik{};
        const auto r=ninja_keyboard_init_77f0a0(s,list,[&]{auto d=std::make_shared<NinjaKeyboardDevice>();d->get_device_state=[&]{return std::pair<std::int32_t,std::array<std::uint8_t,256>>{0,dik};};d->acquire=[]{return 0;};return d;});
        require(r&&list.count_at_3ce712c==3u&&list.slots_at_3d0fa40[2]==0x77f090u&&s.count_at_3d0881c==4u&&s.words_at_3ce6e68[3][1]==2,"77f0a0 init and 77ffc0 append");
        dik[0x1e]=0x80;dik[0x2a]=0x80;require(bool(ninja_keyboard_poll_791bd0(s,table))&&s.records_at_3ce6e38[3].keys[0]==0x04&&s.records_at_3ce6e38[0].modifiers==0x02,"poll fills every record");
        s.records_at_3d08818=0x1000;require(ninja_keyboard_poll_791bd0(s,table).failure==NinjaKeyboardFailure::unsupported_records,"records outside 3ce6e38 are explicit");}
    {NinjaKeyboardState s;NinjaCallbackList list;list.count_at_3ce712c=0;int acquires=0;
        require(bool(ninja_keyboard_init_77f0a0(s,list,[&]{auto d=std::make_shared<NinjaKeyboardDevice>();d->get_device_state=[]{return std::pair<std::int32_t,std::array<std::uint8_t,256>>{static_cast<std::int32_t>(0x8007000cU),{}};};
            d->acquire=[&]{return ++acquires<3?static_cast<std::int32_t>(0x8007001eU):0;};return d;})),"init");
        s.records_at_3ce6e38[1].keys[0]=9;const auto r=ninja_keyboard_poll_791bd0(s,table);require(r&&r.acquire_calls==3&&s.records_at_3ce6e38[1].keys[0]==0,"failed state clears records; Acquire retried while input lost");}
    {NinjaKeyboardState s;NinjaCallbackList list;list.count_at_3ce712c=0;require(bool(ninja_keyboard_init_77f0a0(s,list,[]{return std::shared_ptr<NinjaKeyboardDevice>{};})),"no device");
        require(bool(ninja_keyboard_poll_791bd0(s,table)),"null device returns");}
    std::cout<<"Ninja keyboard: unknown device, 77f0a0/77ffc0, every-record poll, explicit record addressing, Acquire retry and null device passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
