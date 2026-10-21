#include "game/sound_effects.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace sfx=sadx::sound_effects;
namespace {
struct Stop {};
struct Resource final : sfx::Sample { std::uint32_t identity; explicit Resource(std::uint32_t i):identity(i){} };
struct Fixture final : sfx::ServerCalls {
    sadx::SoundInitialState sound;
    sadx::sound_bank::ChannelBindings bindings;
    sfx::State work;
    sfx::Queue queue{sound,bindings,work};
    sadx::TaskScheduler scheduler;
    std::array<sadx::TaskHandle,36> tasks{};
    std::array<std::uint32_t,36> identities{};
    sadx::TaskHandle boss,listener;
    std::int32_t character;
    sfx::BankTable table{};
    const std::uint32_t *input;
    std::uint32_t *output;
    std::vector<std::string> names;
    unsigned count=0;
    Fixture(const std::uint32_t *i,std::uint32_t *o):character(static_cast<std::int32_t>(i[3])),input(i),output(o) {
        sound.paused=i[0];sound.positional_paused=i[1];sound.frame_counter=static_cast<std::uint8_t>(i[2]);sound.queue_count=static_cast<std::int32_t>(i[13]);
        for(std::size_t n=0;n<36;++n) {
            const auto *r=i+14+n*14;auto &a=sound.records[n];auto &b=work.records[n];
            b.priority=static_cast<std::int32_t>(r[0]);a.timer=static_cast<std::int32_t>(r[1]);identities[n]=r[2];bindings.identities[n]=r[2]?&identities[n]:nullptr;
            a.flags=r[3];a.tone=static_cast<std::int32_t>(r[4]);a.pan_angle=static_cast<std::int32_t>(r[5]);a.volume=static_cast<std::int32_t>(r[6]);b.volume_limit=static_cast<std::int32_t>(r[7]);a.pitch=static_cast<std::int32_t>(r[8]);std::memcpy(b.position.data(),r+9,12);b.queue_number=static_cast<std::int32_t>(r[12]);bindings.bank_slots[n]=r[13];
            a.current_volume=static_cast<std::int32_t>(i[518+n]);
            if(i[554+n])tasks[n]=scheduler.append_task(0,nullptr,std::make_shared<std::uint32_t>(i[554+n])).task;
        }
        auto marker=scheduler.append_task(0,nullptr,{}).task;
        if(i[4])boss=marker;if(i[5])listener=marker;
        std::memcpy(sound.overlap.data(),i+590,72);std::memcpy(work.overlap_tail.data(),reinterpret_cast<const char *>(i+590)+72,2);
        for(std::size_t n=0;n<64;++n)table[n]={static_cast<std::int32_t>(i[609+n*2]),i[610+n*2]};
        for(int cue=-1;cue<=1400;++cue) {std::array<char,64> name{};std::snprintf(name.data(),name.size(),"B00_00_%02d.wav",cue);names.emplace_back(name.data());}
    }
    std::uint32_t sample_id() const {const auto *sample=dynamic_cast<const Resource *>(work.selected_sample.resource.get());return sample?sample->identity:0;}
    std::uint32_t task_id(std::size_t n) const {return tasks[n]?*std::static_pointer_cast<std::uint32_t>(tasks[n]->field30):0;}
    void row(std::size_t n,std::uint32_t *r) const {
        const auto &a=sound.records[n];const auto &b=work.records[n];
        r[0]=static_cast<std::uint32_t>(b.priority);r[1]=static_cast<std::uint32_t>(a.timer);r[2]=bindings.identities[n]?*static_cast<const std::uint32_t *>(bindings.identities[n]):0;r[3]=a.flags;r[4]=static_cast<std::uint32_t>(a.tone);r[5]=static_cast<std::uint32_t>(a.pan_angle);r[6]=static_cast<std::uint32_t>(a.volume);r[7]=static_cast<std::uint32_t>(b.volume_limit);r[8]=static_cast<std::uint32_t>(a.pitch);std::memcpy(r+9,b.position.data(),12);r[12]=static_cast<std::uint32_t>(b.queue_number);r[13]=bindings.bank_slots[n];
    }
    void overlap(std::uint32_t *o) const {std::memcpy(o,sound.overlap.data(),72);std::memcpy(reinterpret_cast<char *>(o)+72,work.overlap_tail.data(),2);}
    int active() const {for(std::size_t n=0;n<36;++n)if(sound.records[n].flags&1U)return static_cast<int>(n);return -1;}
    void call(std::uint32_t id,int channel,std::uint32_t a=0,std::uint32_t b=0,std::uint32_t c=0,std::uint32_t d=0) {
        auto *o=output+640+48*count++;o[0]=id;o[1]=a;o[2]=b;o[3]=c;o[4]=d;o[5]=static_cast<std::uint32_t>(channel);
        if(channel>=0) {row(static_cast<std::size_t>(channel),o+6);o[20]=static_cast<std::uint32_t>(sound.records[channel].current_volume);o[47]=task_id(channel);}
        o[21]=sound.frame_counter;o[22]=sample_id();o[23]=work.selected_sample.source_size;overlap(o+24);o[43]=sound.paused;o[44]=sound.positional_paused;o[45]=static_cast<std::uint32_t>(sound.queue_count);o[46]=static_cast<std::uint32_t>(work.alternating_pan);
        if(input[7]==count)throw Stop{};
    }
    sfx::BankView bank(std::uint32_t slot) override {sfx::BankView view{(input[10]&(1U<<slot))!=0,{}};for(const auto &name:names)view.names.push_back(name);return view;}
    sfx::SelectedSample sample(std::uint32_t,std::size_t ordinal) override {return {input[8]?std::make_shared<Resource>(static_cast<std::uint32_t>(ordinal+1)):nullptr,static_cast<std::uint32_t>(999+ordinal)};}
    void load_pcm(std::int32_t ch,const sfx::SelectedSample &,std::int32_t loop) override {call(1,ch,ch,sample_id(),static_cast<std::uint32_t>(loop),work.selected_sample.source_size);}
    void load_3d_pcm(std::int32_t ch,const sfx::SelectedSample &) override {call(2,ch,ch,sample_id(),2,work.selected_sample.source_size);}
    void set_3d_min_max(std::int32_t ch,float,float) override {call(3,ch,ch,0x41200000,0x43fa0000);}
    std::int32_t distance_volume(std::int32_t ch) override {call(12,ch,ch);return static_cast<std::int32_t>(input[6]);}
    void volume(std::int32_t v,std::int32_t ch) override {call(4,ch,static_cast<std::uint32_t>(v),ch);}
    void pan(std::int32_t v,std::int32_t ch) override {call(5,ch,static_cast<std::uint32_t>(v),ch);}
    void set_pan(std::int32_t ch,std::int32_t v) override {call(6,ch,ch,static_cast<std::uint32_t>(v));}
    void pitch(std::int32_t v,std::int32_t ch) override {call(7,ch,static_cast<std::uint32_t>(v),ch);}
    void play_pcm(std::int32_t ch) override {call(8,ch,ch);}
    void stop_pcm(std::int32_t ch) override {call(9,ch,ch);}
    void play_voice(std::int32_t voice) override {call(10,active(),static_cast<std::uint32_t>(voice));}
    void update_positions() override {call(11,-1);}
    void finish() {
        output[0]=count;output[2]=sample_id();output[3]=work.selected_sample.source_size;output[4]=sound.frame_counter;output[5]=sound.paused;output[6]=sound.positional_paused;output[7]=static_cast<std::uint32_t>(sound.queue_count);output[8]=static_cast<std::uint32_t>(work.alternating_pan);
        for(std::size_t n=0;n<36;++n) {row(n,output+9+n*14);output[513+n]=static_cast<std::uint32_t>(sound.records[n].current_volume);output[549+n]=task_id(n);}
        overlap(output+585);
    }
};
}
extern "C" void sadx_native_sound_server_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+32768,0);Fixture fixture(i,o);
    sfx::ServerView view{fixture.queue,fixture.tasks,fixture.table,{fixture.character,fixture.boss},fixture.listener};
    try {if(i[12])sfx::play_title_confirmation(fixture.queue);sfx::update_sound_server(view,fixture);}
    catch(const Stop &) {o[1]=1;}
    fixture.finish();
}
