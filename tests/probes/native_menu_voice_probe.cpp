#include "game/menu_voice.hpp"
#include <algorithm>
namespace voice=sadx::menu_voice;
namespace {
struct Stop{};
struct Fixture {
    sadx::TaskScheduler scheduler;
    sadx::TaskHandle task,other,canonical;
    sadx::cleanup::TrackState tracks;
    const std::uint32_t *input;
    std::uint32_t *output;
    unsigned count=0;
    Fixture(const std::uint32_t *i,std::uint32_t *o):input(i),output(o) {
        task=scheduler.append_task(4,nullptr,{}).task;other=scheduler.append_task(4,nullptr,{}).task;
        if(i[2])canonical=i[2]==1?task:other;
        tracks.current_voice=static_cast<std::int32_t>(i[7]);
        if(i[0]==3)task->auxiliary_native_work=std::make_shared<voice::RequestWork>(voice::RequestWork{static_cast<std::int32_t>(i[1]),static_cast<std::int32_t>(i[4])});
    }
    unsigned identity(const sadx::TaskHandle &t)const{return !t?0:t==task?1:2;}
    void snapshot(std::uint32_t *o)const {
        o[0]=identity(canonical);o[1]=static_cast<std::uint32_t>(tracks.current_voice);
        const auto work=std::static_pointer_cast<voice::RequestWork>(task->auxiliary_native_work);
        o[2]=work?1:0;if(work){o[3]=static_cast<std::uint32_t>(work->voice);o[4]=static_cast<std::uint32_t>(work->elapsed);}
    }
    void call(unsigned id,unsigned a=0,unsigned b=0,unsigned c=0) {
        auto *o=output+16+12*count++;o[0]=id;o[1]=a;o[2]=b;o[3]=c;snapshot(o+4);
        if(count==input[10])throw Stop{};
    }
    voice::TaskServices services() {
        return {[this]{call(1,0,4,1);return input[3]?sadx::TaskHandle{}:task;},
            [this](const sadx::TaskHandle &){call(2,1,8);return std::make_shared<voice::RequestWork>();},
            [this](const sadx::TaskHandle &t){call(3,identity(t));}};
    }
};
}
extern "C" void sadx_native_menu_voice_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+64,0);Fixture f(i,o);auto services=f.services();auto enabled=static_cast<std::int32_t>(i[8]);auto ready=static_cast<std::int32_t>(i[5]);
    try {
        if(i[0]==0)voice::queue_voice(static_cast<std::int32_t>(i[1]),f.tracks,enabled);
        if(i[0]==1)o[8]=static_cast<std::uint32_t>(voice::request_voice(static_cast<std::int32_t>(i[1]),f.canonical,services));
        if(i[0]==2){const auto character=static_cast<std::uint8_t>(i[6]);voice::Catalog catalog{{character,static_cast<std::int32_t>(i[9]),static_cast<std::int32_t>(i[11])}};o[8]=static_cast<std::uint32_t>(voice::request_character_voice(static_cast<std::int32_t>(i[9]),character,catalog,f.canonical,services));}
        if(i[0]==3)voice::update_request(f.task,f.canonical,f.tracks,enabled,ready,services);
    }catch(const Stop &){o[9]=1;}
    f.snapshot(o);o[7]=f.count;
}
