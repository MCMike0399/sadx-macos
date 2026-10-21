#include "game/title_menu.hpp"
#include "engine/pvr_draw.hpp"
#include <algorithm>
#include <cstring>
namespace menu = sadx::title_menu;
namespace {
float number(std::uint32_t word) { float value;std::memcpy(&value,&word,4);return value; }
std::uint32_t bits(float value) { std::uint32_t word;std::memcpy(&word,&value,4);return word; }
struct Pipeline : menu::BackgroundCalls, sadx::pvr::Calls {
    menu::TitleWork &work;
    sadx::X87Context &fp;
    std::uint32_t *out;
    sadx::pvr::State pvr;
    std::uint32_t current=0,depth=3,depth_calls=0,submitted=0;
    explicit Pipeline(menu::TitleWork &owner,sadx::X87Context &context,std::uint32_t *result)
        : work(owner),fp(context),out(result) { sadx::pvr::initialize(pvr); }
    void set_base_color(std::uint32_t color) override { sadx::pvr::set_base_color(color,pvr); }
    void set_material(std::uint32_t color) override { sadx::pvr::set_material(color,pvr); }
    void select_title_texture(menu::TitleTextureList list) override { current=static_cast<std::uint32_t>(list); }
    void draw_scaled_texture(std::uint32_t index,float x,float y,float z,float sx,float sy) override {
        sadx::pvr::draw_scaled_texture(index,x,y,z,sx,sy,pvr,0,*this,fp);
    }
    void set_depth_compare(std::int32_t value) override { depth=static_cast<std::uint32_t>(value);++depth_calls; }
    sadx::pvr::TextureInfo texture(std::uint32_t index) override {
        return {current*100+index+1,536870912U,256+index%3,128+index%5};
    }
    void draw_texture(const std::array<sadx::pvr::Vertex,4> &vertices,std::uint32_t id,
                      std::uint32_t alpha) override {
        auto cursor=out+8+submitted++*32;
        *cursor++=current;*cursor++=id;*cursor++=alpha;*cursor++=depth;
        for (const auto &v:vertices) {
            const std::uint32_t words[]{bits(v.x),bits(v.y),bits(v.reciprocal_depth),bits(v.u),bits(v.v),v.color};
            cursor=std::copy(words,words+6,cursor);
        }
        *cursor++=work.wave_timer;*cursor++=bits(work.cloud_offset);*cursor=work.cloud_index;
    }
    void draw_polygon(const std::array<sadx::pvr::Point,4> &,std::uint32_t,float,std::uint32_t) override {
        throw std::runtime_error("unexpected title polygon");
    }
    void queue_box(float,float,float,float,float,std::uint32_t) override {
        throw std::runtime_error("unexpected title queued box");
    }
};
}
extern "C" void sadx_native_title_background_pipeline_probe(const std::uint32_t *input,std::uint32_t *out) {
    std::fill(out,out+4096,0);
    menu::TitleWork work;
    work.wave_timer=input[0];work.cloud_offset=number(input[1]);work.cloud_index=input[2];
    work.timer=static_cast<std::int32_t>(input[3]);work.blink_timer=input[4];
    const auto horizontal=number(input[5]),vertical=number(input[6]);
    const auto elapsed=static_cast<std::int32_t>(input[7]);const auto loops=input[8];
    const auto language=static_cast<std::int32_t>(input[9]);
    menu::BackgroundView view{horizontal,vertical,elapsed,loops,language};
    menu::BackgroundLayout layout{number(input[11]),number(input[12]),number(input[13]),number(input[14]),number(input[15]),
        number(input[16]),number(input[17]),number(input[18]),number(input[19])};
    sadx::X87Context fp{127};Pipeline pipeline(work,fp,out);
    if (input[20]) {
        pipeline.pvr.base_color=input[21];
        for (unsigned i=0;i<4;++i) pipeline.pvr.vertices[i].color=input[22+i];
    }
    menu::draw_title_background(work,view,layout,pipeline,fp);
    out[0]=work.wave_timer;out[1]=bits(work.cloud_offset);out[2]=work.cloud_index;
    out[3]=pipeline.submitted;out[4]=fp.exception_flags();out[5]=pipeline.depth;out[6]=pipeline.depth_calls;
}
