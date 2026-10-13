#include "game/audio_stream.hpp"
#include <algorithm>
// Native 410510 matching tools/verify_original_audio_stream.py.
// in: 0 initialized,1 window,2 device HRESULT,3 event base,4 thread handle,5 thread id
extern "C" int sadx_native_audio_stream(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=8||!out||capacity<160)return 0;
    sadx::AudioStreamState s;s.initialized_at_3b0ef2c=in[0];
    for(unsigned i=0;i<36;++i){s.words_at_3b0ec30[i]=0xabababab;s.events_at_3b0ece0[i]=0xabababab;s.words_at_3b0ee98[i]=0xabababab;s.words_at_3b0ed70[i]=0xabababab;}
    s.thread_at_3b0ee00=0xabababab;s.thread_id_at_3b0ec28=0xabababab;std::optional<std::uint32_t> window=in[1];
    std::uint32_t events=0,device_calls=0;std::uint32_t seen_window=0,seen_rate=0;
    sadx::AudioStreamOwners o;
    o.initialize_device=[&](sadx::AudioSoundObject& obj,const sadx::AudioDeviceRequest& r)->std::optional<std::int32_t>{++device_calls;seen_window=r.window_from_3a8b088.value_or(0);seen_rate=r.sample_rate|r.channels<<20|r.bits<<24|r.cooperative_level<<28;
        if(obj.device_word_at_0!=0u)return std::nullopt;return static_cast<std::int32_t>(in[2]);};
    o.create_event=[&]{return std::optional<std::uint32_t>{in[3]+events++};};
    o.start_stream_thread_40ff50=[&]{return std::optional<std::array<std::uint32_t,2>>{{in[4],in[5]}};};
    o.initialize_critical_section=[]{return true;};
    const auto r=sadx::run_audio_stream_init_410510(s,window,o);std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.returned.value_or(0xdeadbeef);out[2]=s.initialized_at_3b0ef2c.value_or(0xdeadbeef);out[3]=s.sound_object_at_3b0ecc0.has_value();
    out[4]=device_calls;out[5]=seen_window;out[6]=seen_rate;const auto untouched=[](std::optional<std::uint32_t> v){return !v||*v==0xababababU?0U:*v;};out[7]=untouched(s.thread_at_3b0ee00);out[8]=untouched(s.thread_id_at_3b0ec28);out[9]=s.critical_section_at_3b0ecc4_initialized;
    for(unsigned i=0;i<36;++i){out[10+i]=*s.words_at_3b0ec30[i];out[46+i]=*s.events_at_3b0ece0[i];out[82+i]=*s.words_at_3b0ee98[i];out[118+i]=*s.words_at_3b0ed70[i];}
    return 1;
}
