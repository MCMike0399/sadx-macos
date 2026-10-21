#include "game/saving_message.hpp"
#include "engine/cooperative_thread_native.hpp"
#include <algorithm>
#include <cstring>
namespace {
std::uint32_t bits(float f){std::uint32_t u;std::memcpy(&u,&f,4);return u;}
float value(std::uint32_t u){float f;std::memcpy(&f,&u,4);return f;}
std::shared_ptr<const sadx::message_text::FontData> font(const std::uint8_t *bytes,unsigned size){
    auto f=std::make_shared<sadx::message_text::FontData>();f->glyphs.resize(size/88);
    for(unsigned n=0;n<f->glyphs.size();++n)std::copy_n(bytes+n*88+16,72,f->glyphs[n].bitmap.begin());
    std::copy_n(bytes,72,f->fallback.bitmap.begin());return f;
}
struct Calls final:sadx::saving_message::Calls {
    sadx::menu_caption::Context &context;std::uint8_t *arena;std::uint32_t *out;
    std::uint32_t &pause;std::int32_t &mode,&inhibit; sadx::cleanup::MessageState &messages;
    std::uint8_t &gate;unsigned events=0;
    Calls(sadx::menu_caption::Context &c,std::uint8_t *a,std::uint32_t *o,std::uint32_t &p,
          std::int32_t &m,std::int32_t &i,sadx::cleanup::MessageState &q,std::uint8_t &g)
        :context(c),arena(a),out(o),pause(p),mode(m),inhibit(i),messages(q),gate(g){}
    void event(std::uint32_t tag){out[80+events++]=tag;out[79]=events;}
    sadx::menu_caption::BitmapView bitmap_from_arena_end(std::uint32_t bytes)override{
        event(1);out[64]=bytes;return{arena,bytes};
    }
    void disable_fog()override{event(3);}
    void draw_black_box(float r,float b,float priority,std::uint32_t color)override{
        event(4);out[65]=bits(r);out[66]=bits(b);out[67]=bits(priority);out[68]=color;
    }
    void reset_draw_queue()override{event(5);std::fill_n(arena,131104,0);inhibit=0;}
    void skip_queue_execution()override{event(6);mode=1;}
    void wait_for_cooperative_thread()override{
        event(2);if(pause!=1)throw std::runtime_error("wait must see live pause1");
        const std::shared_ptr<void> current{};
        if(sadx::cooperative_threads::wait(current)!=sadx::cooperative_threads::WaitResult::Completed)
            throw std::runtime_error("null cooperative wait did not complete");
    }
    void draw_message(sadx::menu_caption::Context &c)override{
        event(7);sadx::cleanup::display_message(&c,messages,gate,inhibit);
    }
    std::shared_ptr<sadx::menu_caption::Buffer> allocate_buffer(std::size_t)override{throw std::runtime_error("heap allocation is not the saving arena");}
    void free_buffer(std::shared_ptr<sadx::menu_caption::Buffer>)override{throw std::runtime_error("not called");}
    void append_message(sadx::menu_caption::Context &,const std::string &)override{throw std::runtime_error("not called");}
    void load_message_texture(sadx::menu_caption::Context &)override{throw std::runtime_error("not called");}
    void release_texture_list(sadx::texture_lifetime::List &)override{throw std::runtime_error("not called");}
    void release_queue_overflow()override{throw std::runtime_error("not called");}
};
}
extern "C" void sadx_native_saving_message_probe(const std::uint32_t *i,const std::uint8_t *single,
 unsigned single_size,const std::uint8_t *wide,unsigned wide_size,const std::uint8_t *widths,
 std::uint32_t *out,std::uint8_t *pixels){
    std::fill_n(out,128,0);alignas(32)std::uint8_t storage[131136];std::fill_n(storage,131136,0xa5);
    auto *arena=storage+i[12];std::memcpy(arena,pixels,131104);
    sadx::menu_caption::Context context;
    context.kind=static_cast<std::uint8_t>(i[13]);context.flags=i[14];
    sadx::message_text::State fonts;fonts.directory.single=font(single,single_size);
    fonts.directory.wide=font(wide,wide_size);
    for(unsigned n=0;n<224;++n)fonts.widths[n]=static_cast<std::int8_t>(widths[n]);
    auto pending=static_cast<std::uint8_t>(i[0]);auto pause=i[1];auto reset=static_cast<std::uint8_t>(i[2]);
    auto horizontal=value(i[3]),vertical=value(i[4]);auto language=static_cast<std::int32_t>(i[5]);
    auto mode=static_cast<std::int32_t>(i[6]),inhibit=static_cast<std::int32_t>(i[7]);
    auto gate=static_cast<std::uint8_t>(i[8]);sadx::cleanup::MessageState messages;
    messages.count=i[9];std::array<sadx::cleanup::Message,4> existing{{{0},{0},{0},{0}}};
    for(unsigned n=0;n<4;++n)messages.entries[n]=&existing[n];
    Calls calls(context,arena,out,pause,mode,inhibit,messages,gate);
    sadx::X87Context fp(static_cast<std::uint16_t>(i[10]));
    sadx::saving_message::display({pending,context,pause,reset,horizontal,vertical,language,fonts,"Now Saving..."},calls,fp);
    out[0]=pending;out[1]=pause;out[2]=reset;out[3]=static_cast<std::uint32_t>(mode);
    out[4]=static_cast<std::uint32_t>(inhibit);out[5]=messages.count;
    for(unsigned n=0;n<4;++n)out[6+n]=messages.entries[n]==&context?1U:n+2;
    out[16]=context.kind;out[17]=static_cast<std::uint16_t>(context.x);out[18]=static_cast<std::uint16_t>(context.y);
    out[19]=context.width;out[20]=context.height;out[21]=context.buffer_width;out[22]=context.buffer_height;
    out[23]=context.bitmap_width;out[24]=context.bitmap_height;out[25]=static_cast<std::uint16_t>(context.cursor_x);
    out[26]=static_cast<std::uint16_t>(context.cursor_y);out[27]=static_cast<std::uint16_t>(context.spacing_x);
    out[28]=static_cast<std::uint16_t>(context.spacing_y);out[29]=context.color;out[30]=context.foreground;
    out[31]=context.background;out[32]=bits(context.scale);out[33]=context.bitmap?static_cast<unsigned>(reinterpret_cast<std::uint8_t*>(context.bitmap)-arena)+1U:0;
    out[34]=context.global_index;out[35]=context.flags;out[36]=fp.exception_flags();
    std::memcpy(pixels,arena,131104);
}
