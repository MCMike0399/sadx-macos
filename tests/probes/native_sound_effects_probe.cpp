#include "game/sound_effects.hpp"
#include "cpu/x87_math.hpp"
#include <array>
#include <cstring>
#include <algorithm>
namespace sfx=sadx::sound_effects;
extern "C" void sadx_native_sound_effects_probe(const std::uint32_t *i,std::uint32_t *o) {
    sadx::SoundInitialState sound;sadx::sound_bank::ChannelBindings bindings;sfx::State work;
    std::array<std::uint32_t,37> identities{};
    sound.queue_count=static_cast<std::int32_t>(i[5]);work.alternating_pan=static_cast<std::int32_t>(i[6]);
    for(std::size_t n=0;n<36;++n) {
        const auto *r=i+7+n*14;auto &a=sound.records[n];auto &b=work.records[n];
        b.priority=static_cast<std::int32_t>(r[0]);a.timer=static_cast<std::int32_t>(r[1]);
        identities[n]=r[2];bindings.identities[n]=r[2]?&identities[n]:nullptr;
        a.flags=r[3];a.tone=static_cast<std::int32_t>(r[4]);a.pan_angle=static_cast<std::int32_t>(r[5]);
        a.volume=static_cast<std::int32_t>(r[6]);b.volume_limit=static_cast<std::int32_t>(r[7]);
        a.pitch=static_cast<std::int32_t>(r[8]);std::memcpy(b.position.data(),r+9,12);
        b.queue_number=static_cast<std::int32_t>(r[12]);bindings.bank_slots[n]=r[13];
    }
    sfx::Queue queue{sound,bindings,work};identities[36]=i[2];
    std::int32_t result=0;
    if(i[0]==0)result=sfx::find_free_channel(queue,static_cast<std::int32_t>(i[3]));
    if(i[0]==1)result=sfx::play_one_shot(static_cast<std::int32_t>(i[1]),i[2]?&identities[36]:nullptr,
        static_cast<std::int32_t>(i[3]),static_cast<std::int32_t>(i[4]),queue);
    if(i[0]==2)sfx::play_title_confirmation(queue);
    o[0]=static_cast<std::uint32_t>(result);o[1]=static_cast<std::uint32_t>(sound.queue_count);
    o[2]=static_cast<std::uint32_t>(work.alternating_pan);
    for(std::size_t n=0;n<36;++n) {
        auto *r=o+3+n*14;const auto &a=sound.records[n];const auto &b=work.records[n];
        r[0]=static_cast<std::uint32_t>(b.priority);r[1]=static_cast<std::uint32_t>(a.timer);
        r[2]=bindings.identities[n]?*static_cast<const std::uint32_t *>(bindings.identities[n]):0;
        r[3]=a.flags;r[4]=static_cast<std::uint32_t>(a.tone);r[5]=static_cast<std::uint32_t>(a.pan_angle);
        r[6]=static_cast<std::uint32_t>(a.volume);r[7]=static_cast<std::uint32_t>(b.volume_limit);
        r[8]=static_cast<std::uint32_t>(a.pitch);std::memcpy(r+9,b.position.data(),12);
        r[12]=static_cast<std::uint32_t>(b.queue_number);r[13]=bindings.bank_slots[n];
    }
}

namespace {
struct ParameterCalls final : sfx::PcmParameterCalls {
    std::uint32_t *out;
    unsigned count = 0;
    explicit ParameterCalls(std::uint32_t *o):out(o) {}
    void call(std::uint32_t id,std::int32_t channel,std::uint32_t a,std::uint32_t b=0,std::uint32_t c=0) {
        auto *p=out+4+count++*5;p[0]=id;p[1]=static_cast<std::uint32_t>(channel);p[2]=a;p[3]=b;p[4]=c;
    }
    void set_volume(std::int32_t ch,std::int32_t v) override {call(1,ch,static_cast<std::uint32_t>(v));}
    void set_pan(std::int32_t ch,std::int32_t v) override {call(2,ch,static_cast<std::uint32_t>(v));}
    void set_frequency(std::int32_t ch,std::uint32_t v) override {call(3,ch,v);}
    void set_position(std::int32_t ch,float,float,float) override {call(4,ch,0,0x459c4000,0);}
};
}
extern "C" void sadx_native_sound_parameter_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+16,0);sadx::SoundInitialState sound;sadx::sound_bank::ChannelBindings bindings;sfx::State state;
    sound.records[i[2]].flags=i[3];sfx::Queue queue{sound,bindings,state};ParameterCalls calls(o);sadx::X87Context fp(static_cast<std::uint16_t>(i[4]));
    if(i[0]==0)sfx::set_volume(static_cast<std::int32_t>(i[1]),static_cast<std::int32_t>(i[2]),queue,calls);
    if(i[0]==1)sfx::set_pan(static_cast<std::int32_t>(i[1]),static_cast<std::int32_t>(i[2]),calls);
    if(i[0]==2)sfx::set_pitch(static_cast<std::int32_t>(i[1]),static_cast<std::int32_t>(i[2]),calls,fp);
    o[0]=calls.count;o[1]=fp.exception_flags();
}
