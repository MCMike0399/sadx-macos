#include "game/menu_dialog.hpp"
#include "math/ninja_sine_table.hpp"
#include <algorithm>
#include <cstring>
namespace d=sadx::menu_dialog;
namespace {
float number(std::uint32_t x){float y;std::memcpy(&y,&x,4);return y;}
struct Calls final:d::UpdateCalls {
 std::uint32_t pressed;std::uint32_t *out;unsigned count=0;
 Calls(std::uint32_t p,std::uint32_t *o):pressed(p),out(o){}
 void receipt(unsigned kind,std::uint32_t arg=0){out[48+count*2]=kind;out[49+count*2]=arg;++count;out[47]=count;}
 void play_sound(std::int32_t cue)override{receipt(1,static_cast<std::uint32_t>(cue));}
 std::uint32_t pressed_buttons()override{return pressed;}
 void clear_screen_fade()override{receipt(2);}
 void set_back_colors(std::uint32_t a,std::uint32_t b,std::uint32_t c)override{if(a!=b||a!=c)throw std::runtime_error("back colors");receipt(3,a);}
 void set_material(std::uint32_t a)override{receipt(4,a);}
 void set_base_color(std::uint32_t a)override{receipt(5,a);}
 void reset_material()override{receipt(6);}
 void default_blending()override{receipt(7,8);receipt(8,6);}
};
}
extern "C" void sadx_native_menu_dialog_update_probe(const std::uint32_t *i,std::uint32_t *o,const std::uint32_t *table){
 std::fill_n(o,128,0);sadx::TaskScheduler scheduler;auto task=scheduler.append_task(3,nullptr,{}).task;auto work=std::make_shared<d::Work>();task->auxiliary_native_work=work;
 d::Parameters p{};p.style=static_cast<sadx::title_menu::DialogStyle>(i[1]);p.choice_count=static_cast<std::int8_t>(i[9]);p.cancel_choice=static_cast<std::int8_t>(i[10]);if(i[11])p.sounds=std::make_shared<sadx::title_menu::DialogSounds>(sadx::title_menu::DialogSounds{21,22,1,2,3});work->parameters=&p;
 work->phase=static_cast<std::int32_t>(i[0]);work->animation_angle=i[3];work->fade=number(i[4]);work->cursor=static_cast<std::uint8_t>(i[5]);work->previous_choice=static_cast<std::int32_t>(i[6]);work->opening_frames=static_cast<std::int16_t>(i[7]);work->repeat_frames=static_cast<std::int16_t>(i[8]);work->blink_step=i[12];work->color=i[14];work->upper_color=i[15];work->lower_color=i[16];work->scale_x=number(i[17]);work->scale_y=number(i[18]);for(unsigned n=0;n<16;++n)work->choice_ids[n]=static_cast<std::int8_t>(i[20+n]);
 d::Tables tables{};for(unsigned n=0;n<16;++n){tables.opening[n]={number(table[n*2]),number(table[n*2+1])};tables.closing[n]={number(table[32+n*2]),number(table[33+n*2])};}for(unsigned n=0;n<6;++n)tables.directions[n]={table[64+n*2],table[65+n*2]};
 sadx::X87Context fp{127};sadx::NinjaMath math(sadx::ninja_sine_table(),fp);Calls calls(i[13],o);d::update(*work,{task},tables,i[2],calls,math,fp);
 o[0]=static_cast<std::uint32_t>(work->phase);o[1]=*work->animation_angle;std::memcpy(o+2,&work->fade,4);o[3]=work->animation_value;o[4]=work->upper_color;o[5]=work->lower_color;o[6]=work->color;std::memcpy(o+7,&work->scale_x,4);std::memcpy(o+8,&work->scale_y,4);o[9]=static_cast<std::uint16_t>(work->repeat_frames);o[10]=static_cast<std::uint16_t>(work->opening_frames);o[11]=work->cursor;o[12]=static_cast<std::uint32_t>(work->previous_choice);for(unsigned n=0;n<16;++n)o[13+n]=static_cast<std::uint8_t>(work->choice_ids[n]);o[46]=fp.exception_flags();
}
