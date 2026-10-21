#include "game/menu_dialog_file_select.hpp"
#include <algorithm>
#include <cstring>
namespace d=sadx::menu_dialog;namespace f=sadx::file_select;
namespace {
struct Calls final:d::ConfirmationCalls {
 d::ConfirmationState *confirmation=nullptr;
 const std::uint32_t *in;std::uint32_t *out;f::ControlState &state;char copied[32]{};unsigned count=0;
 Calls(const std::uint32_t *i,std::uint32_t *o,f::ControlState &s):in(i),out(o),state(s){}
 void update_confirmation(sadx::title_menu::FileSelectWork &work,f::ControlState &)override{d::confirm_selection(work,*confirmation,*this);}
 void record(unsigned k,unsigned arg=0){out[64+count*2]=k;out[65+count*2]=arg;++count;out[63]=count;}
 std::int32_t dialog_result()override{record(1);return static_cast<std::int32_t>(in[0]);}
 void transition_menu(sadx::title_menu::MenuMode mode)override{record(2,static_cast<std::uint32_t>(mode));if(in[16]!=0xffffffffU)state.card_file=static_cast<std::uint8_t>(in[16]);}
 void request_delete_confirmation()override{record(3);}
 void free_save_name(const char *)override{record(4);}
 char *allocate_save_name(std::size_t n)override{record(5,static_cast<unsigned>(n));return copied;}
 void copy_save_name(char *p,std::string_view text)override{record(6,static_cast<unsigned>(text.back()));std::memcpy(p,text.data(),text.size());p[text.size()]=0;}
 std::uint32_t pressed_buttons()override{return 0;}
 void request_dialog(std::int32_t)override{throw std::runtime_error("unexpected");}
 void prepare_new_save()override{throw std::runtime_error("unexpected");}
 void apply_save()override{throw std::runtime_error("unexpected");}
 void request_save()override{throw std::runtime_error("unexpected");}
 std::int32_t close_warning()override{throw std::runtime_error("unexpected");}
 void load_selected_save()override{throw std::runtime_error("unexpected");}
 void play_navigation_sound()override{throw std::runtime_error("unexpected");}
 void play_cancel_sound()override{throw std::runtime_error("unexpected");}
};
float number(std::uint32_t a){float v;std::memcpy(&v,&a,4);return v;}
}
extern "C" void sadx_native_menu_dialog_confirmation_probe(const std::uint32_t *i,std::uint32_t *o){
 std::fill_n(o,128,0);auto displayed=static_cast<std::uint8_t>(i[1]),card=static_cast<std::uint8_t>(i[2]),selected=static_cast<std::uint8_t>(i[3]),count=static_cast<std::uint8_t>(i[4]);std::uint16_t backup=static_cast<std::uint16_t>(i[5]);auto marker=static_cast<std::uint8_t>(i[6]),character=static_cast<std::uint8_t>(i[7]),cursor=static_cast<std::uint8_t>(i[8]);std::int16_t first=0;std::uint8_t selected_card=0,pending=0,alert=0,new_file=0;std::int32_t ready=1;std::uint16_t player=0;const auto ticks=i[13];f::ControlState selection{count,selected,displayed,card,first,selected_card,pending,alert,ready,ticks,player,new_file};
 auto list=std::make_shared<f::ListNode>();auto tail=list;for(unsigned n=1;n<4;++n){tail->next=std::make_shared<f::ListNode>();tail=tail->next;tail->file=f::FileInfo{"Slot"+std::to_string(n),0};}
 const char *name=i[9]?"old":nullptr;d::ConfirmationState state{selection,backup,marker,character,cursor,list,name};Calls calls(i,o,selection);calls.confirmation=&state;sadx::title_menu::FileSelectWork work{};work.submode=2;work.presentation.next=static_cast<sadx::title_menu::MenuMode>(i[10]);work.wait_frames=static_cast<std::int16_t>(i[11]);work.cursor_angle=static_cast<std::int32_t>(i[12]);work.cursor_scale=number(i[14]);work.selected_character=static_cast<std::int32_t>(i[15]);sadx::X87Context fp{127};sadx::NinjaMath math(sadx::NinjaMath::SineTable{},fp);f::update_controls(work,selection,calls,math,fp);
 o[0]=static_cast<std::uint32_t>(work.submode);o[1]=card;o[2]=selected;o[3]=backup;o[4]=marker;o[5]=cursor;o[6]=static_cast<std::uint16_t>(work.wait_frames);o[7]=static_cast<std::uint32_t>(work.cursor_angle);std::memcpy(o+8,&work.cursor_scale,4);o[9]=work.cursor_color;o[10]=work.cursor_alpha;o[11]=static_cast<std::uint32_t>(work.selected_character);o[12]=name==nullptr?0:name==calls.copied?2:1;if(name==calls.copied)std::memcpy(o+16,calls.copied,32);o[62]=fp.exception_flags();
}
