#include "input/input_mod_axes.hpp"
#include <cassert>
#include <initializer_list>
using namespace sadx;
int main() {
    // Values pinned to retained Input2.6.1 .rdata, not production defaults.
    const InputModAxesConstants c{{0x34000000},{0x46fffe00},{0x42fe0000},{0x43000000},{0x3f800000}};
    SseContext fp(0x1f80);InputModAxesOutput a,b;
    normalize_input_mod_261_keyboard({0,0},{0,0},c,fp,a,b);
    assert(a.x==0&&a.y==0&&a.magnitude->bits==0&&b.x==0&&b.y==0);
    normalize_input_mod_261_keyboard({8192,0},{0,-8192},c,fp,a,b);
    assert(a.x==127&&a.y==0&&a.magnitude->bits==0x3e800100);assert(b.x==0&&b.y==-127);
    normalize_input_mod_261_keyboard({32767,32767},{-32768,0},c,fp,a,b);
    assert(a.x==89&&a.y==89&&a.magnitude->bits==0x3f800000);assert(b.x==-127&&b.y==0&&b.magnitude->bits==0x3f800000);
    convert_input_mod_261_axes({7848,0},7849,true,c,fp,a);assert(a.x==0&&a.y==0&&a.magnitude->bits==0);
    convert_input_mod_261_axes({32767,0},7849,true,c,fp,a);assert(a.x==127&&a.y==0&&a.magnitude->bits==0x3f800000);
    convert_input_mod_261_axes({100,20000},7849,false,c,fp,a);assert(a.x==0&&*a.y>0);
    convert_input_mod_261_axes({500,32767},7849,true,c,fp,a);assert(*a.x>0&&*a.y==127);
    // All four source ramp steps and opposing/release ease must be supplied
    // raw, rather than reconstructed from an already truncated direction.
    std::uint32_t expected[]={0x3e800100,0x3f000100,0x3f400180,0x3f800000};
    for(int i=0;i<4;++i) {normalize_input_mod_261_keyboard({std::int16_t(i==3?32767:8192*(i+1)),0},{0,0},c,fp,a,b);assert(a.magnitude->bits==expected[i]);}
    // Raw ramp release/opposing easing and diagonal transition retain separate
    // direction and magnitude, including overshoot cancellation to zero.
    for(auto raw : {24575,16383,8191,0,-8192,-16384,-24576,-32767}) {
        normalize_input_mod_261_keyboard({std::int16_t(raw),0},{0,0},c,fp,a,b);
        assert(a.x==(raw>0?127:raw<0?-127:0));assert(a.y==0);
    }
    normalize_input_mod_261_keyboard({8192,8192},{0,0},c,fp,a,b);
    assert(a.x==89&&a.y==89&&a.magnitude->bits!=0x3f800000);
    // A source right-stick precision fault occurs after committed left stores.
    SseContext precision_fault(0x0f80);InputModAxesOutput left_partial,right_partial;
    bool trapped=false;
    try { normalize_input_mod_261_keyboard({0,0},{8192,8192},c,precision_fault,left_partial,right_partial); }
    catch(const SseError& e) { trapped=e.exceptions()==32; }
    assert(trapped&&left_partial.x==0&&left_partial.y==0&&left_partial.magnitude->bits==0);
    assert(!right_partial.x&&!right_partial.y&&!right_partial.magnitude);
    // Source permits singular configured deadzone; keep real invalid/NaN and
    // source CVTT integer indefinite low16=0 rather than guessing a clamp.
    convert_input_mod_261_axes({32767,0},32767,true,c,fp,a);assert(a.x==0&&a.y==0&&a.magnitude->bits==0xffc00000);
    assert(!decode_input_mod_axes_constants(nullptr,20));
}
