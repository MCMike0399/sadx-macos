#include "ninja/ninja_keyboard.hpp"
namespace sadx {
namespace {
struct Stop{};
constexpr std::uint32_t records_base=0x3ce6e38,words_base=0x3ce6e68;
constexpr std::int32_t input_lost=static_cast<std::int32_t>(0x8007001eU);
}
std::optional<NinjaKeyboardTable> decode_ninja_keyboard_table(const std::uint8_t* bytes,std::size_t size){
    if(!bytes||size!=0xed*4)return std::nullopt;
    NinjaKeyboardTable t{};
    for(std::size_t i=0;i<0xed;++i){t.kind[i]=static_cast<std::uint16_t>(bytes[4*i]|(bytes[4*i+1]<<8));t.code[i]=bytes[4*i+2];}
    return t;
}
NinjaKeyboardResult ninja_keyboard_poll_791bd0(NinjaKeyboardState& s,const NinjaKeyboardTable& table){
    NinjaKeyboardResult r;
    const auto stop=[&](NinjaKeyboardFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};};
    const auto need=[&](const auto& v,std::uint32_t at,std::uint32_t fact){if(!v)stop(NinjaKeyboardFailure::unknown_fact,at,fact);return *v;};
    // Records are addressed through 3d08818; only the 77f0a0 array is owned here.
    const auto records=[&](std::uint32_t at)->std::size_t{
        const auto count=static_cast<std::int32_t>(need(s.count_at_3d0881c,at,0x3d0881c));if(count<=0)return 0;
        if(need(s.records_at_3d08818,at+8,0x3d08818)!=records_base||count>4)stop(NinjaKeyboardFailure::unsupported_records,at+8);
        return static_cast<std::size_t>(count);};
    try{
        if(!s.device_known)stop(NinjaKeyboardFailure::unknown_fact,0x791be2,0x3d08820);
        if(!s.device_at_3d08820)return r; // 791d46: only the stack-cookie check
        auto& device=*s.device_at_3d08820;
        if(!device.get_device_state)stop(NinjaKeyboardFailure::missing_owner,0x791c00);
        const auto [hresult,state]=device.get_device_state();
        if(hresult!=0){
            for(std::size_t i=0,n=records(0x791d14);i<n;++i){auto& rec=s.records_at_3ce6e38[i];rec.modifiers=0;for(auto& k:rec.keys)k=0;}
            for(;;){if(!device.acquire)stop(NinjaKeyboardFailure::missing_owner,0x791d3a);++r.acquire_calls;if(device.acquire()!=input_lost)break;}
            return r;
        }
        for(std::size_t i=0,n=records(0x791c0b);i<n;++i){
            auto& rec=s.records_at_3ce6e38[i];
            std::array<std::uint8_t,6> previous{};for(std::size_t k=0;k<6;++k)previous[k]=need(rec.keys[k],0x791c28,static_cast<std::uint32_t>(records_base+0xc*i+2+k));
            rec.modifiers=0;for(auto& k:rec.keys)k=0;
            std::array<std::uint8_t,6> pressed{};std::size_t count=0;std::uint8_t modifiers=0;
            for(std::size_t dik=0;dik<0xed&&count!=6;++dik){
                if(!(state[dik]&0x80))continue;
                if(table.kind[dik]==0)pressed[count++]=table.code[dik];
                else if(table.kind[dik]==1)modifiers|=table.code[dik];
            }
            rec.modifiers=modifiers;
            std::size_t out=0;
            for(std::size_t k=0;k<6&&previous[k]!=0;++k)
                for(std::size_t j=0;j<count;++j)if(pressed[j]==previous[k]){rec.keys[out++]=pressed[j];break;}
            for(std::size_t j=0;j<count;++j){
                bool held=false;
                for(std::size_t k=0;k<6;++k){if(previous[k]==0)break;if(pressed[j]==previous[k]){held=true;break;}}
                if(!held&&out!=6)rec.keys[out++]=pressed[j];
            }
        }
    }catch(Stop){}
    return r;
}
NinjaKeyboardResult ninja_callback_register_77ffc0(NinjaCallbackList& list,std::uint32_t callback){
    NinjaKeyboardResult r;
    if(!list.count_at_3ce712c){r.failure=NinjaKeyboardFailure::unknown_fact;r.source_boundary=0x77ffc0;r.required_fact=0x3ce712c;return r;}
    const auto index=*list.count_at_3ce712c;
    if(list.slots_at_3d0fa40.size()<=index)list.slots_at_3d0fa40.resize(index+1);
    list.slots_at_3d0fa40[index]=callback;list.count_at_3ce712c=index+1;return r;
}
NinjaKeyboardResult ninja_keyboard_init_77f0a0(NinjaKeyboardState& s,NinjaCallbackList& list,const NinjaKeyboardCreate& create,NinjaKeyboardProfile profile){
    for(std::size_t i=0;i<4;++i){auto& rec=s.records_at_3ce6e38[i];rec.modifiers=0;rec.byte1=0;for(auto& k:rec.keys)k=0;rec.pointer=static_cast<std::uint32_t>(words_base+4*i);}
    if(profile==NinjaKeyboardProfile::input_mod){s.input_pointer_at_3b0e340=kInputModSadxKeyboard;return ninja_callback_register_77ffc0(list,0x77f090);}
    // 791b10: 3d08820 is cleared before CreateDevice; the record pointer and count are stored first.
    s.device_at_3d08820.reset();s.device_known=true;s.records_at_3d08818=records_base;s.count_at_3d0881c=4;
    if(!create){NinjaKeyboardResult r;r.failure=NinjaKeyboardFailure::missing_owner;r.source_boundary=0x791b42;return r;}
    s.device_at_3d08820=create();
    for(std::size_t i=0;i<4;++i)s.words_at_3ce6e68[i]={std::uint8_t{1},std::uint8_t{2},std::uint8_t{0},std::uint8_t{0}};
    return ninja_callback_register_77ffc0(list,0x77f090);
}
} // namespace sadx
