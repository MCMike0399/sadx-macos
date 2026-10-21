#include "game/file_select.hpp"
#include <algorithm>
#include <cstring>
namespace files=sadx::file_select;
namespace title=sadx::title_menu;
namespace {
std::uint32_t word(float f){std::uint32_t w;std::memcpy(&w,&f,4);return w;}
float floating(std::uint32_t w){float f;std::memcpy(&f,&w,4);return f;}
struct Stop{};
struct Calls final:files::UpdateCalls {
    title::FileSelectWork &work;title::State &menus;std::uint32_t &ticks;std::uint8_t &marker;const std::uint32_t *input;std::uint32_t *out;unsigned count=0;
    Calls(title::FileSelectWork &w,title::State &m,std::uint32_t &t,std::uint8_t &b,const std::uint32_t *i,std::uint32_t *o):work(w),menus(m),ticks(t),marker(b),input(i),out(o){}
    void snapshot(std::uint32_t *o){o[0]=static_cast<std::uint32_t>(work.presentation.phase);o[1]=word(work.presentation.fade_progress);o[2]=work.presentation.color;o[3]=static_cast<std::uint32_t>(menus.sequence_work->mode);o[4]=static_cast<std::uint32_t>(menus.sequence_work->old_mode);o[5]=static_cast<std::uint32_t>(menus.sequence_work->fixed_mode);o[6]=menus.sequence_work->fade_end;o[7]=static_cast<std::uint32_t>(ticks);o[8]=marker;}
    void call(unsigned kind,std::uint32_t arg=0){auto *o=out+32+count++*12;o[0]=kind;o[1]=arg;snapshot(o+2);if(count==input[9])throw Stop{};if(kind==input[10]){if(input[15])menus.sequence_work=std::make_shared<title::SequenceWork>(*menus.sequence_work);menus.sequence_work->mode=static_cast<title::MenuMode>(input[11]);menus.sequence_work->old_mode=static_cast<title::MenuMode>(input[12]);ticks=input[13];work.presentation.fade_progress=floating(input[14]);}}
    void initialize_file_select(title::FileSelectWork &)override{call(1);}
    std::uint32_t fade_out_color(float p)override{call(2,word(p));return 0x76543210;}
    std::uint32_t fade_in_color(float p)override{call(3,word(p));return 0x87654321;}
    void update_controls(title::FileSelectWork &)override{call(4);}
    void release_mode_textures(title::MenuMode m)override{call(5,static_cast<std::uint32_t>(m));}
    void release_font()override{call(6);}
    void free_save_list()override{call(7);}
};
}
extern "C" void sadx_native_file_select_update_probe(const std::uint32_t *i,std::uint32_t *o){
    std::fill(o,o+256,0);title::FileSelectWork work;work.presentation.phase=static_cast<title::TitlePhase>(i[0]);work.presentation.fade_progress=floating(i[3]);work.presentation.color=i[4];title::State menus;menus.sequence_work=std::make_shared<title::SequenceWork>();menus.sequence_work->mode=static_cast<title::MenuMode>(i[1]);menus.sequence_work->old_mode=static_cast<title::MenuMode>(i[2]);menus.sequence_work->fixed_mode=static_cast<title::MenuMode>(i[5]);menus.sequence_work->fade_end=static_cast<std::uint8_t>(i[6]);auto ticks=i[7];auto marker=static_cast<std::uint8_t>(i[8]);Calls calls(work,menus,ticks,marker,i,o);sadx::X87Context fp{127};
    try{files::update(work,menus,ticks,marker,calls,fp);}catch(const Stop&){o[10]=1;}calls.snapshot(o);o[9]=calls.count;o[11]=fp.exception_flags();
}
