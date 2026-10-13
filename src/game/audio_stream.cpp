#include "game/audio_stream.hpp"

namespace sadx {
AudioStreamResult run_audio_stream_init_410510(AudioStreamState& s,const std::optional<std::uint32_t>& window,AudioStreamOwners& o){
    AudioStreamResult r;const auto stop=[&](AudioStreamFailure f,std::uint32_t at){r.failure=f;r.source_boundary=at;return r;};
    if(!s.initialized_at_3b0ef2c)return stop(AudioStreamFailure::unknown_state,0x410526);
    if(*s.initialized_at_3b0ef2c!=0){r.returned=1;return r;}
    auto object=std::make_shared<AudioSoundObject>();object->device_word_at_0=0; // 6443e7 new(4), 513e60
    s.sound_object_at_3b0ecc0=object;
    if(!o.initialize_device)return stop(AudioStreamFailure::missing_owner,0x410579);
    AudioDeviceRequest request;request.window_from_3a8b088=window;
    r.device_hresult=o.initialize_device(*object,request);
    if(!r.device_hresult)return stop(AudioStreamFailure::owner_failed,0x410579);
    if(*r.device_hresult<0){r.returned=0;return r;}
    for(std::size_t i=0;i<36;++i){
        s.words_at_3b0ec30[i]=0;
        if(!o.create_event)return stop(AudioStreamFailure::missing_owner,0x4105b2);
        const auto event=o.create_event();if(!event)return stop(AudioStreamFailure::owner_failed,0x4105b2);
        s.events_at_3b0ece0[i]=*event;
    }
    for(auto& w:s.words_at_3b0ee98)w=0;for(auto& w:s.words_at_3b0ed70)w=0;
    s.initialized_at_3b0ef2c=1;
    if(!o.start_stream_thread_40ff50)return stop(AudioStreamFailure::missing_owner,0x4105f7);
    const auto thread=o.start_stream_thread_40ff50();if(!thread)return stop(AudioStreamFailure::owner_failed,0x4105f7);
    s.thread_id_at_3b0ec28=(*thread)[1];s.thread_at_3b0ee00=(*thread)[0];
    if(!o.initialize_critical_section)return stop(AudioStreamFailure::missing_owner,0x410607);
    if(!o.initialize_critical_section())return stop(AudioStreamFailure::owner_failed,0x410607);
    s.critical_section_at_3b0ecc4_initialized=true;r.returned=1;return r;
}
} // namespace sadx
