#include "game/file_select.hpp"
#include "math/ninja_sine_table.hpp"
#include <algorithm>
#include <cstring>
namespace files=sadx::file_select;
namespace title=sadx::title_menu;
namespace {
std::uint32_t word(float f){std::uint32_t w;std::memcpy(&w,&f,4);return w;}
float floating(std::uint32_t w){float f;std::memcpy(&f,&w,4);return f;}
struct Stop{};
struct Calls final:files::ControlCalls {
    title::FileSelectWork &work;files::ControlState &state;const std::uint32_t *input;std::uint32_t *out;unsigned count=0;
    Calls(title::FileSelectWork &w,files::ControlState &s,const std::uint32_t *i,std::uint32_t *o):work(w),state(s),input(i),out(o){}
    void snapshot(std::uint32_t *o){o[0]=static_cast<std::uint32_t>(work.submode);o[1]=word(work.cursor_scale);o[2]=static_cast<std::uint32_t>(work.cursor_angle);o[3]=work.cursor_alpha;o[4]=work.cursor_color;o[5]=state.file_count;o[6]=state.selected_file;o[7]=state.displayed_save;o[8]=state.card_file;o[9]=static_cast<std::uint16_t>(state.first_visible_file);o[10]=state.selected_card;o[11]=state.file_operation_pending;o[12]=state.alert_blocked;o[13]=static_cast<std::uint32_t>(state.ready);o[14]=static_cast<std::uint32_t>(state.logic_ticks);o[15]=state.character;o[16]=state.new_file_requested;o[17]=static_cast<std::uint32_t>(work.presentation.previous);o[18]=static_cast<std::uint32_t>(work.presentation.next);o[19]=static_cast<std::uint16_t>(work.wait_frames);}
    void call(unsigned kind,std::uint32_t arg=0){auto *o=out+32+count++*24;o[0]=kind;o[1]=arg;snapshot(o+2);if(count==input[21])throw Stop{};if(kind==input[22]){state.displayed_save=static_cast<std::uint8_t>(input[23]);state.selected_file=static_cast<std::uint8_t>(input[24]);}}
    std::uint32_t pressed_buttons()override{return input[17];}
    void request_dialog(std::int32_t n)override{call(1,static_cast<std::uint32_t>(n));}
    void prepare_new_save()override{call(2);}
    void apply_save()override{call(3);}
    void request_save()override{call(4);}
    std::int32_t close_warning()override{call(5);return static_cast<std::int32_t>(input[20]);}
    void load_selected_save()override{call(6);}
    void transition_menu(title::MenuMode mode)override{call(7,static_cast<std::uint32_t>(mode));}
    void play_navigation_sound()override{call(8);}
    void play_cancel_sound()override{call(9);}
};
}
extern "C" void sadx_native_file_select_controls_probe(const std::uint32_t *i,std::uint32_t *o){
    std::fill(o,o+256,0);title::FileSelectWork work;work.submode=static_cast<std::int32_t>(i[0]);work.cursor_scale=floating(i[1]);work.cursor_angle=static_cast<std::int32_t>(i[2]);work.cursor_alpha=static_cast<std::uint8_t>(i[3]);work.cursor_color=i[18];work.wait_frames=static_cast<std::int16_t>(i[19]);work.presentation.previous=static_cast<title::MenuMode>(i[16]);work.presentation.next=static_cast<title::MenuMode>(i[25]);
    auto selected=static_cast<std::uint8_t>(i[4]),card=static_cast<std::uint8_t>(i[5]),count=static_cast<std::uint8_t>(i[6]),displayed=static_cast<std::uint8_t>(i[7]),selected_card=static_cast<std::uint8_t>(i[9]),pending=static_cast<std::uint8_t>(i[10]),alert=static_cast<std::uint8_t>(i[11]),new_file=static_cast<std::uint8_t>(i[15]);auto first=static_cast<std::int16_t>(i[8]);auto ready=static_cast<std::int32_t>(i[12]);auto ticks=i[13];auto character=static_cast<std::uint16_t>(i[14]);
    files::ControlState state{count,selected,displayed,card,first,selected_card,pending,alert,ready,ticks,character,new_file};Calls calls(work,state,i,o);sadx::X87Context fp{127};sadx::NinjaMath math(sadx::ninja_sine_table(),fp);
    try{files::update_controls(work,state,calls,math,fp);}catch(const Stop&){o[24]=1;}catch(const files::ControlStageError &e){o[26]=1;o[27]=static_cast<std::uint32_t>(e.stage());}calls.snapshot(o);o[23]=calls.count;o[25]=fp.exception_flags();
}
