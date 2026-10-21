#include "game/message_text.hpp"
#include <algorithm>
#include <cstring>
namespace text=sadx::message_text;
namespace {
std::shared_ptr<const text::FontData> font(const std::uint8_t *bytes,std::uint32_t size) {
    auto result=std::make_shared<text::FontData>();
    result->glyphs.resize(size/88);
    for(unsigned i=0;i<result->glyphs.size();++i)
        std::copy_n(bytes+i*88+16,72,result->glyphs[i].bitmap.begin());
    std::copy_n(bytes,72,result->fallback.bitmap.begin());
    return result;
}
struct Loads final:text::LoadCalls {
    text::State &state;
    std::shared_ptr<const text::FontData> single,wide;
    std::uint32_t *out;
    unsigned count=0;
    Loads(text::State &s,std::shared_ptr<const text::FontData> a,std::shared_ptr<const text::FontData> b,
            std::uint32_t *o):state(s),single(std::move(a)),wide(std::move(b)),out(o){}
    std::shared_ptr<const text::FontData> load(text::FontFamily family)override {
        const std::uint32_t values[]{static_cast<std::uint32_t>(family),state.loaded_wide?1U:0U,
            state.loaded_single?1U:0U,state.directory.wide?1U:0U,state.directory.single?1U:0U};
        std::copy_n(values,5,out+300+count*5);++count;out[299]=count;
        return family==text::FontFamily::Wide?wide:single;
    }
};
}
extern "C" void sadx_native_message_text_probe(const std::uint32_t *i,const std::uint8_t *a,
        std::uint32_t a_size,const std::uint8_t *b,std::uint32_t b_size,std::uint32_t *o,
        std::uint16_t *pixels,const char *message) {
    std::fill_n(o,512,0);
    auto single=font(a,a_size);auto wide=i[13]?font(b,b_size):std::shared_ptr<const text::FontData>{};
    text::State state;
    if(i[0]!=4){state.directory.single=single;state.directory.wide=wide;}
    for(unsigned n=0;n<224;++n)state.widths[n]=static_cast<std::int8_t>(i[32+n]);
    if(i[14]){
        auto map=std::make_shared<std::vector<std::uint8_t>>(224);
        for(unsigned n=0;n<224;++n)(*map)[n]=static_cast<std::uint8_t>(i[256+n]);
        for(auto &language:state.directory.languages)language.remapping=map;
    }
    const auto language=static_cast<std::int32_t>(i[2]);
    sadx::menu_caption::Context context;
    context.bitmap=i[3]?pixels:nullptr;context.bitmap_capacity=i[12];
    context.buffer_width=static_cast<std::uint16_t>(i[4]);context.buffer_height=static_cast<std::uint16_t>(i[5]);
    context.cursor_x=static_cast<std::int16_t>(i[6]);context.cursor_y=static_cast<std::int16_t>(i[7]);
    context.spacing_x=static_cast<std::int16_t>(i[8]);context.spacing_y=static_cast<std::int16_t>(i[9]);
    context.color=static_cast<std::uint16_t>(i[10]);context.flags=i[11];
    try {
        switch(i[0]) {
        case 0:{
            const auto &g=text::glyph(static_cast<std::int32_t>(i[1]),state,language);
            if(&g==&single->fallback)o[0]=3;
            else {
                for(unsigned n=0;n<single->glyphs.size();++n)if(&g==&single->glyphs[n]){o[0]=1;o[1]=n;}
                if(wide)for(unsigned n=0;n<wide->glyphs.size();++n)if(&g==&wide->glyphs[n]){o[0]=2;o[1]=n;}
            }
            break;
        }
        case 1:o[0]=static_cast<std::uint32_t>(text::advance(static_cast<std::int32_t>(i[1]),state,language));break;
        case 2:text::put_character(context,static_cast<std::int32_t>(i[1]),state,language);break;
        case 3:text::put_text(context,message,state,language);break;
        case 4:{Loads calls(state,single,wide,o);text::initialize(state,language,calls);break;}
        case 5:text::request_texture_upload(context);break;
        }
    }catch(const text::Error &error){o[6]=static_cast<std::uint32_t>(error.boundary())+1;}
    o[2]=static_cast<std::uint16_t>(context.cursor_x);o[3]=static_cast<std::uint16_t>(context.cursor_y);
    o[4]=context.flags;
    for(unsigned n=0;n<224;++n)o[32+n]=static_cast<std::uint8_t>(state.widths[n]);
}
