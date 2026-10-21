#include "engine/font_native.hpp"
#include <algorithm>
#include <cstring>
namespace font=sadx::font;
namespace {
float floating(std::uint32_t v){float f;std::memcpy(&f,&v,4);return f;}
std::uint32_t word(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}
struct Calls final:font::Calls {
    font::State &state;std::uint32_t *out;
    Calls(font::State &s,std::uint32_t *o):state(s),out(o){}
    void load_texture(sadx::texture_lifetime::List &)override{out[16]=1;out[17]=word(state.width);out[18]=word(state.height);out[19]=state.source!=nullptr;}
};
}
extern "C" void sadx_native_font_native_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+32,0);sadx::texture_lifetime::List textures;std::array<std::uint8_t,128> proportions{};
    font::Info info{floating(i[4]),floating(i[5]),floating(i[6]),floating(i[7]),i[2]?&textures:nullptr,i[3]?&proportions:nullptr,{i[8],i[9],i[10],i[11]}};
    font::Info old{floating(i[15]),floating(i[16]),floating(i[17]),floating(i[18]),i[19]?&textures:nullptr,i[20]?&proportions:nullptr,{i[21],i[22],i[23],i[24]}};
    font::State state{floating(i[12]),floating(i[13]),i[14]?&old:nullptr,old};Calls calls(state,o);
    if(i[0]==2)font::release(info,[&](sadx::texture_lifetime::List &list){calls.load_texture(list);o[16]=2;});else if(i[0])font::copy(info,state);else font::load(i[1]?&info:nullptr,state,calls);
    o[0]=word(state.width);o[1]=word(state.height);o[2]=!state.source?0:state.source==&info?1:2;
    const auto &current=state.current;o[3]=word(current.char_width);o[4]=word(current.char_height);o[5]=word(current.tex_width);o[6]=word(current.tex_height);o[7]=current.textures!=nullptr;o[8]=current.proportions!=nullptr;std::copy(current.colors.begin(),current.colors.end(),o+9);
}
