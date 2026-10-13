#include "input/input_mod_axes.hpp"
#include <algorithm>
namespace sadx {
namespace {
std::uint32_t word(const std::uint8_t* b) { return std::uint32_t(b[0]) | std::uint32_t(b[1])<<8 | std::uint32_t(b[2])<<16 | std::uint32_t(b[3])<<24; }
std::int32_t absolute(std::int16_t v) { return v<0 ? -std::int32_t(v) : v; }
std::int32_t clipped(std::int16_t v) { return std::max(-32767,std::int32_t(v)); }
std::int16_t low16(std::int32_t v) { auto low=std::uint32_t(v)&65535;return static_cast<std::int16_t>(low<32768?std::int32_t(low):std::int32_t(low)-65536); }
struct Direction { Sse32 magnitude, x, y; };
Direction direction(InputModRawAxes raw,Sse32 threshold,SseContext& fp) {
    auto x=fp.from_int32(clipped(raw.x)); auto y=fp.from_int32(clipped(raw.y));
    auto yy=fp.multiply(y,y); auto xx=fp.multiply(x,x);
    auto sum=fp.add(yy,xx); auto wide=fp.to_double(sum);
    // Valid signed16 inputs make the original fallback CRT sqrt branch
    // unreachable. Retain the UCOMISD operation/status before SQRTSD.
    (void)fp.ucomisd(Sse64{0},wide);
    auto magnitude=fp.to_single(fp.sqrt(wide));
    auto compare=fp.comiss(threshold,magnitude);
    Sse32 nx{0},ny{0};
    if(compare!=SseComparison::greater) { nx=fp.divide(x,magnitude); ny=fp.divide(y,magnitude); }
    return {magnitude,nx,ny};
}
}
std::optional<InputModAxesConstants> decode_input_mod_axes_constants(const std::uint8_t* b,std::size_t n) {
    if(!b||n!=20)return std::nullopt;
    return InputModAxesConstants{{word(b)},{word(b+4)},{word(b+8)},{word(b+12)},{word(b+16)}};
}
void convert_input_mod_261_axes(InputModRawAxes raw,std::int16_t deadzone,bool radial,
    const InputModAxesConstants& c,SseContext& fp,InputModAxesOutput& out) {
    if(absolute(raw.x)<deadzone && absolute(raw.y)<deadzone) { out.x=0;out.y=0;out.magnitude=Sse32{0};return; }
    auto dead=fp.from_int32(deadzone); auto d=direction(raw,dead,fp);
    auto comparison=fp.comiss(c.maximum,d.magnitude);
    // COMISS + CMOVBE selects max for less/equal/unordered, no MINSS.
    auto capped=comparison==SseComparison::greater ? d.magnitude : c.maximum;
    auto numerator=fp.subtract(capped,dead);
    auto denominator=fp.from_int32(32767-std::int32_t(deadzone));
    auto magnitude=fp.divide(numerator,denominator);
    auto component=[&](Sse32 axis) {
        auto value=fp.multiply(magnitude,axis);
        value=fp.multiply(value,c.controller_scale);
        return static_cast<std::int16_t>(std::clamp(std::int32_t(low16(fp.truncate_int32(value))),-127,127));
    };
    out.x=!radial && absolute(raw.x)<deadzone ? std::int16_t{0} : component(d.x);
    out.y=!radial && absolute(raw.y)<deadzone ? std::int16_t{0} : component(d.y);
    out.magnitude=magnitude; // source XMM0 return
}
void normalize_input_mod_261_keyboard(InputModRawAxes left,InputModRawAxes right,
    const InputModAxesConstants& c,SseContext& fp,InputModAxesOutput& lo,InputModAxesOutput& ro) {
    auto l=direction(left,c.epsilon,fp);
    auto lm=fp.divide(l.magnitude,c.maximum);
    auto ly=fp.multiply(l.y,c.keyboard_scale);
    lm=fp.minimum(lm,c.magnitude_cap);
    auto lx=fp.multiply(l.x,c.keyboard_scale);
    lo.y=low16(fp.truncate_int32(ly)); // 0x10004f21
    lo.magnitude=lm; // 0x10004f2c
    lo.x=low16(fp.truncate_int32(lx)); // 0x10004f4e
    auto r=direction(right,c.epsilon,fp);
    auto rm=fp.divide(r.magnitude,c.maximum);
    auto rx=fp.multiply(r.x,c.keyboard_scale);
    auto ry=fp.multiply(r.y,c.keyboard_scale);
    rm=fp.minimum(rm,c.magnitude_cap);
    auto x=low16(fp.truncate_int32(rx));
    auto y=low16(fp.truncate_int32(ry));
    ro.x=x; // 0x10004ff1
    ro.magnitude=rm; // 0x10004ff8
    ro.y=y; // 0x10005000, before excluded half-press phase
}
} // namespace sadx
