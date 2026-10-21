#include "game/title_menu.hpp"
#include "cpu/x87_math.hpp"
#include <algorithm>
#include <cstring>
namespace menu = sadx::title_menu;
namespace {
float number(std::uint32_t word) { float value;std::memcpy(&value,&word,4);return value; }
std::uint32_t bits(float value) { std::uint32_t word;std::memcpy(&word,&value,4);return word; }
struct Stop {};
struct Calls : menu::BackgroundCalls {
    menu::TitleWork &work;
    std::uint32_t *out;
    std::uint32_t stop,count=0;
    Calls(menu::TitleWork &owner,std::uint32_t *result,std::uint32_t stop_at)
        : work(owner),out(result),stop(stop_at) {}
    void event(std::uint32_t kind,std::array<std::uint32_t,6> values) {
        auto cursor=out+8+count*10;
        *cursor++=kind;
        for (auto value:values) *cursor++=value;
        *cursor++=work.wave_timer;*cursor++=bits(work.cloud_offset);*cursor=work.cloud_index;
        if (++count==stop) throw Stop{};
    }
    void set_base_color(std::uint32_t color) override { event(1,{color,0,0,0,0,0}); }
    void set_material(std::uint32_t color) override { event(2,{color,0,0,0,0,0}); }
    void select_title_texture(menu::TitleTextureList list) override {
        event(3,{static_cast<std::uint32_t>(list),0,0,0,0,0});
    }
    void draw_scaled_texture(std::uint32_t index,float x,float y,float depth,
                               float scale_x,float scale_y) override {
        event(4,{index,bits(x),bits(y),bits(depth),bits(scale_x),bits(scale_y)});
    }
    void set_depth_compare(std::int32_t mode) override {
        event(5,{static_cast<std::uint32_t>(mode),0,0,0,0,0});
    }
};
}
extern "C" void sadx_native_title_background_probe(const std::uint32_t *input,std::uint32_t *out) {
    std::fill(out,out+2048,0);
    menu::TitleWork work;
    work.wave_timer=input[0];work.cloud_offset=number(input[1]);work.cloud_index=input[2];
    work.timer=static_cast<std::int32_t>(input[3]);work.blink_timer=input[4];
    const auto horizontal=number(input[5]),vertical=number(input[6]);
    const auto elapsed=static_cast<std::int32_t>(input[7]);const auto loops=input[8];
    const auto language=static_cast<std::int32_t>(input[9]);
    menu::BackgroundView view{horizontal,vertical,elapsed,loops,language};
    menu::BackgroundLayout layout{number(input[11]),number(input[12]),number(input[13]),
        number(input[14]),number(input[15]),number(input[16]),number(input[17]),number(input[18]),number(input[19])};
    sadx::X87Context fp{127};Calls calls(work,out,input[10]);
    try { menu::draw_title_background(work,view,layout,calls,fp); }
    catch (const Stop &) { out[3]=1; }
    out[0]=work.wave_timer;out[1]=bits(work.cloud_offset);out[2]=work.cloud_index;
    out[4]=calls.count;out[5]=fp.exception_flags();
}
