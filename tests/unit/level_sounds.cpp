#include "game/level_sounds.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>

int main() {
    using namespace sadx::level_sounds;
    std::int16_t stage=0,act=0;
    std::uint16_t character=0;
    std::int32_t voice=1,queue_reset=17,bank_reset=18;
    std::uint8_t metal=0;
    std::uint32_t paused=5,positional=6;
    State state{stage,act,character,voice,metal,paused,positional,queue_reset,bank_reset};
    std::vector<int> banks;
    unsigned stops=0;
    Services services{[&](std::int32_t bank) {
        assert(paused==0 && positional==0);
        assert(queue_reset==17 && bank_reset==18);
        banks.push_back(bank);
    },[&] {
        assert(queue_reset==-6 && bank_reset==-6);
        ++stops;
        return 0;
    }};
    assert(load(state,services)==0);
    assert((banks==std::vector<int>{36,1,72}) && stops==1);
    stage=9;character=2;voice=0;metal=1;queue_reset=17;bank_reset=18;banks.clear();
    load(state,services);
    assert((banks==std::vector<int>{24,62,59}));
    stage=9;character=0;voice=0;metal=1;queue_reset=17;bank_reset=18;banks.clear();
    load(state,services);
    assert((banks==std::vector<int>{24,1,62}));
    stage=39;character=3;voice=0;queue_reset=17;bank_reset=18;banks.clear();
    load(state,services);
    assert((banks==std::vector<int>{61,69}));
    stage=0;act=255;character=0;voice=1;metal=0;queue_reset=17;bank_reset=18;banks.clear();
    load(state,services);
    assert(banks[0]==36);
    act=256;queue_reset=17;bank_reset=18;banks.clear();
    load(state,services);
    assert(banks[0]==0); // high act byte ORs with stage0 to select1
    act=0;paused=5;positional=6;queue_reset=17;bank_reset=18;
    services.select_sound_bank=[&](std::int32_t) {
        assert(paused==0 && positional==0);
        throw std::runtime_error("bank boundary");
    };
    try { load(state,services); assert(false); } catch(const std::runtime_error &) {}
    assert(queue_reset==17 && bank_reset==18);
    // The character read follows the first backend, and the voice read follows
    // the selected character bank; neither may be cached at function entry.
    banks.clear();
    services.select_sound_bank=[&](std::int32_t bank) {
        banks.push_back(bank);
        if(banks.size()==1) character=5;
        else if(banks.size()==2) voice=0;
    };
    character=0;voice=1;
    load(state,services);
    assert((banks==std::vector<int>{36,46,63}));
}
