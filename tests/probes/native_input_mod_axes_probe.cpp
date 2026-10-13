#include "input/input_mod_axes.hpp"
#include <cstdint>
namespace {
std::int16_t signed16(std::uint32_t a) {auto v=a&65535;return static_cast<std::int16_t>(v<32768?static_cast<int>(v):static_cast<int>(v)-65536);}
void output(const sadx::InputModAxesOutput& o,std::uint32_t* p) {p[0]=o.x?std::uint32_t(std::int32_t(*o.x)):0xdeadbeef;p[1]=o.y?std::uint32_t(std::int32_t(*o.y)):0xdeadbeef;p[2]=o.magnitude?o.magnitude->bits:0xdeadbeef;}
}
extern "C" int sadx_native_input_mod_axes(const std::uint32_t* in,std::uint32_t mode,
    const std::uint8_t* constants,std::uint32_t mxcsr,std::uint32_t* out) {
    auto c=sadx::decode_input_mod_axes_constants(constants,20);if(!c)return -1;
    sadx::SseContext fp(mxcsr);sadx::InputModAxesOutput a,b;int failed=0;
    try {
        if(mode==0)sadx::convert_input_mod_261_axes({signed16(in[0]),signed16(in[1])},signed16(in[2]),in[3]!=0,*c,fp,a);
        else sadx::normalize_input_mod_261_keyboard({signed16(in[0]),signed16(in[1])},{signed16(in[2]),signed16(in[3])},*c,fp,a,b);
    } catch(const sadx::SseError& e) {failed=static_cast<int>(e.exceptions());}
    output(a,out);output(b,out+3);out[6]=fp.mxcsr();out[7]=failed;return 0;
}
