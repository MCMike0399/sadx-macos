#include "game/menu_voice.hpp"
#include <cassert>
int main() {
    sadx::TaskScheduler scheduler;sadx::TaskHandle canonical;unsigned allocated=0,destroyed=0;
    sadx::cleanup::TrackState tracks;std::int32_t enabled=1,ready=1;
    sadx::menu_voice::TaskServices calls{
        [&]{return scheduler.append_task(4,nullptr,{}).task;},
        [&](const sadx::TaskHandle &){++allocated;return std::make_shared<sadx::menu_voice::RequestWork>();},
        [&](const sadx::TaskHandle &task){++destroyed;scheduler.destroy_task(task);}
    };
    assert(sadx::menu_voice::request_voice(1,canonical,calls)==1);const auto first=canonical;
    assert(sadx::menu_voice::request_voice(2,canonical,calls)==0);
    sadx::menu_voice::update_request(first,canonical,tracks,enabled,ready,calls);assert(destroyed==0);
    sadx::menu_voice::update_request(first,canonical,tracks,enabled,ready,calls);assert(!canonical&&tracks.current_voice==1);
    assert(sadx::menu_voice::request_voice(7,canonical,calls)==1);const auto second=canonical;
    assert(second!=first&&allocated==2);
    const auto work=std::static_pointer_cast<sadx::menu_voice::RequestWork>(second->auxiliary_native_work);
    assert(work->elapsed==0&&work->voice==7);
    sadx::menu_voice::update_request(second,canonical,tracks,enabled,ready,calls);assert(destroyed==1);
    enabled=0;sadx::menu_voice::update_request(second,canonical,tracks,enabled,ready,calls);assert(!canonical&&destroyed==2&&tracks.current_voice==1);
}
