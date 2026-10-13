#include "engine/ninja_keyboard.hpp"
#include <algorithm>
#include <cstring>
// Native 791bd0 / 77f0a0 with the layout of tools/verify_original_keyboard.py:
// in[0] kind (0 poll, 1 init), [1] device (0 null), [2] GetDeviceState HRESULT,
// [3] Acquire input-lost repeats, [4] 3d0881c, [5] 3d08818, [6] 3ce712c,
// [8..55] record bytes, [56..71] 3ce6e68 bytes, [72..327] DIK state, [328..1275] table bytes.
extern "C" int sadx_native_keyboard(const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    if(!in||count!=1276||!out||capacity<80)return 0;
    std::array<std::uint8_t,0xed*4> raw{};for(std::size_t i=0;i<raw.size();++i)raw[i]=static_cast<std::uint8_t>(in[328+i]);
    const auto table=sadx::decode_ninja_keyboard_table(raw.data(),raw.size());if(!table)return 0;
    sadx::NinjaKeyboardState s;s.device_known=true;s.count_at_3d0881c=in[4];s.records_at_3d08818=in[5];
    for(unsigned i=0;i<4;++i){auto& r=s.records_at_3ce6e38[i];const auto* b=in+8+12*i;r.modifiers=static_cast<std::uint8_t>(b[0]);r.byte1=static_cast<std::uint8_t>(b[1]);
        for(unsigned k=0;k<6;++k)r.keys[k]=static_cast<std::uint8_t>(b[2+k]);r.pointer=b[8]|(b[9]<<8)|(b[10]<<16)|(b[11]<<24);
        for(unsigned k=0;k<4;++k)s.words_at_3ce6e68[i][k]=static_cast<std::uint8_t>(in[56+4*i+k]);}
    std::array<std::uint8_t,256> dik{};for(unsigned i=0;i<256;++i)dik[i]=static_cast<std::uint8_t>(in[72+i]);
    std::uint32_t lost=0;
    const auto make=[&]{auto d=std::make_shared<sadx::NinjaKeyboardDevice>();
        d->get_device_state=[&]{return std::pair<std::int32_t,std::array<std::uint8_t,256>>{static_cast<std::int32_t>(in[2]),dik};};
        d->acquire=[&]{return lost++<in[3]?static_cast<std::int32_t>(0x8007001eU):0;};return d;};
    sadx::NinjaCallbackList list;list.count_at_3ce712c=in[6];
    sadx::NinjaKeyboardResult r;
    if(in[0]==0){if(in[1])s.device_at_3d08820=make();r=sadx::ninja_keyboard_poll_791bd0(s,*table);}
    else r=sadx::ninja_keyboard_init_77f0a0(s,list,[&]{return in[1]?make():std::shared_ptr<sadx::NinjaKeyboardDevice>{};});
    std::fill(out,out+capacity,0);out[0]=r?1U:0U;out[1]=r.acquire_calls;out[2]=r.source_boundary;
    for(unsigned i=0;i<4;++i){const auto& rec=s.records_at_3ce6e38[i];auto* o=out+8+12*i;o[0]=rec.modifiers.value_or(0xef);o[1]=rec.byte1.value_or(0xef);
        for(unsigned k=0;k<6;++k)o[2+k]=rec.keys[k].value_or(0xef);const auto p=rec.pointer.value_or(0xdeadbeef);for(unsigned k=0;k<4;++k)o[8+k]=(p>>(8*k))&0xff;
        for(unsigned k=0;k<4;++k)out[56+4*i+k]=s.words_at_3ce6e68[i][k].value_or(0xef);}
    out[72]=s.records_at_3d08818.value_or(0xdeadbeef);out[73]=s.count_at_3d0881c.value_or(0xdeadbeef);out[74]=list.count_at_3ce712c.value_or(0xdeadbeef);
    out[75]=(list.count_at_3ce712c&&*list.count_at_3ce712c&&list.slots_at_3d0fa40.size()>=*list.count_at_3ce712c)?list.slots_at_3d0fa40[*list.count_at_3ce712c-1].value_or(0xdeadbeef):0;
    out[76]=s.device_at_3d08820?1U:0U;
    return 1;
}
