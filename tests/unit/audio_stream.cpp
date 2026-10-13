#include "game/audio_stream.hpp"
#include <iostream>
#include <stdexcept>
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    const std::optional<std::uint32_t> window=0x1234;
    {sadx::AudioStreamState s;s.initialized_at_3b0ef2c=0;sadx::AudioStreamOwners o;std::uint32_t n=0;
     o.initialize_device=[](sadx::AudioSoundObject& obj,const sadx::AudioDeviceRequest& r)->std::optional<std::int32_t>{require(obj.device_word_at_0==0u&&r.sample_rate==22050&&r.channels==2&&r.bits==16&&r.window_from_3a8b088==0x1234u,"44de80 request");return 0;};
     o.create_event=[&]{return std::optional<std::uint32_t>{100+n++};};o.start_stream_thread_40ff50=[]{return std::optional<std::array<std::uint32_t,2>>{{7,8}};};o.initialize_critical_section=[]{return true;};
     const auto r=sadx::run_audio_stream_init_410510(s,window,o);require(r&&r.returned==1u&&s.initialized_at_3b0ef2c==1u&&s.events_at_3b0ece0[35]==135u&&s.thread_at_3b0ee00==7u&&s.critical_section_at_3b0ecc4_initialized,"successful init");
     require(sadx::run_audio_stream_init_410510(s,window,o).returned==1u&&n==36,"second call returns 1 without reinitializing");}
    {sadx::AudioStreamState s;s.initialized_at_3b0ef2c=0;sadx::AudioStreamOwners o;o.initialize_device=[](auto&,const auto&){return std::optional<std::int32_t>{static_cast<std::int32_t>(0x88780078u)};};
     const auto r=sadx::run_audio_stream_init_410510(s,window,o);require(r&&r.returned==0u&&s.initialized_at_3b0ef2c==0u&&s.sound_object_at_3b0ecc0&&!s.events_at_3b0ece0[0],"no driver: returns 0 after storing the object");}
    {sadx::AudioStreamState s;sadx::AudioStreamOwners o;require(sadx::run_audio_stream_init_410510(s,window,o).source_boundary==0x410526,"unknown init flag");}
    {sadx::AudioStreamState s;s.initialized_at_3b0ef2c=0;sadx::AudioStreamOwners o;o.initialize_device=[](auto&,const auto&){return std::optional<std::int32_t>{0};};
     const auto r=sadx::run_audio_stream_init_410510(s,window,o);require(r.failure==sadx::AudioStreamFailure::missing_owner&&r.source_boundary==0x4105b2&&s.words_at_3b0ec30[0]==0u,"missing event owner after the first table store");}
    std::cout<<"Audio stream: 410510 init, already-initialized and no-driver paths, owner boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
