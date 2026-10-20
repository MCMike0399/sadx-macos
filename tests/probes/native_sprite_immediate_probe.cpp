#include "engine/sprite_immediate.hpp"
#include <algorithm>
#include <cstring>
namespace {
float number(std::uint32_t v) { float f; std::memcpy(&f,&v,4);return f; }
std::uint32_t bits(float v) { std::uint32_t n;std::memcpy(&n,&v,4);return n; }
}
extern "C" void sadx_native_sprite_immediate(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx;
    std::fill_n(out,64,0);
    sprite::TextureList textures;
    std::vector<sprite::AnimationFrame> frames{{
        static_cast<std::int16_t>(in[10]),static_cast<std::int16_t>(in[11]),
        static_cast<std::int16_t>(in[12]),static_cast<std::int16_t>(in[13]),
        static_cast<std::int16_t>(in[14]),static_cast<std::int16_t>(in[15]),
        static_cast<std::int16_t>(in[16]),static_cast<std::int16_t>(in[17]),
        static_cast<std::int16_t>(in[18]),static_cast<std::int16_t>(in[19])}};
    sprite::Sprite item{{number(in[4]),number(in[5]),number(in[6])},number(in[7]),number(in[8]),
                         static_cast<std::int32_t>(in[9]),in[0]==2?nullptr:&textures,frames};
    unsigned transforms=0;
    auto event=[&](unsigned id){out[17+out[16]++]=id;};
    sprite::ImmediateServices services{
        [&](const sprite::TextureList&){event(1);},
        [&]{event(2);},
        [&](std::int32_t a){event(3);out[36]=static_cast<std::uint32_t>(a);},
        [&](const std::array<float,3>& v){event(4);std::array<float,3> r;
            for(unsigned i=0;i<3;++i){out[40+transforms*3+i]=bits(v[i]);r[i]=number(in[24+transforms*3+i]);}
            ++transforms;return r;},
        [&]{event(5);},
        [&](bool alpha){event(6);out[35]=alpha;},
        [&](std::uint32_t color){event(7);out[13]=color;},
        [&](std::int16_t index){event(8);out[14]=static_cast<std::uint32_t>(static_cast<std::int32_t>(index));},
        [&](const sprite::Quad& q){event(9);unsigned n=0;
            for(auto f:q.origin)out[n++]=bits(f);
            for(auto f:q.horizontal)out[n++]=bits(f);
            for(auto f:q.vertical)out[n++]=bits(f);
            for(auto f:q.uv_origin)out[n++]=bits(f);
            for(auto f:q.uv_horizontal)out[n++]=bits(f);
            for(auto f:q.uv_vertical)out[n++]=bits(f);},
        [&]{event(10);}};
    X87Context fp(static_cast<std::uint16_t>(in[3]));
    sprite::draw_sprite(in[0]==1?nullptr:&item,0,number(in[2]),static_cast<std::uint8_t>(in[1]),
        {number(in[20]),number(in[21]),number(in[22]),number(in[23])},services,fp);
    out[62]=fp.control_word();out[63]=fp.exception_flags();
}
extern "C" void sadx_native_sprite_quad(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx;
    std::fill_n(out,40,0);
    sprite::Quad q{{number(in[0]),number(in[1]),number(in[2])},
        {number(in[3]),number(in[4])},{number(in[5]),number(in[6])},
        {number(in[8]),number(in[9])},{number(in[10]),number(in[11])},
        {number(in[12]),number(in[13])}};
    std::array<sprite::ScreenVertex,4> vertices{};
    X87Context fp(static_cast<std::uint16_t>(in[16]));
    sprite::QuadServices services{
        [&](float z){out[28]=bits(z);out[29]++;return fp.from_float(number(in[14]));},
        [&](const std::array<sprite::ScreenVertex,4>&){out[30]++;}};
    sprite::draw_quad(q,in[15],vertices,services,fp);
    for(unsigned i=0;i<4;++i){const auto& v=vertices[i];const std::uint32_t words[]={bits(v.x),bits(v.y),bits(v.depth),bits(v.reciprocal_w),v.color,bits(v.u),bits(v.v)};std::copy_n(words,7,out+i*7);}
    out[38]=fp.control_word();out[39]=fp.exception_flags();
}
