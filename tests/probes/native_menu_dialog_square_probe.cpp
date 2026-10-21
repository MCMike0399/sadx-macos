#include "game/menu_dialog.hpp"
#include <algorithm>
#include <cstring>
namespace d=sadx::menu_dialog;
namespace {
float number(std::uint32_t a){float v;std::memcpy(&v,&a,4);return v;}
std::uint32_t bits(float a){std::uint32_t v;std::memcpy(&v,&a,4);return v;}
struct Calls final:d::DrawCalls {
 std::uint32_t *out;std::uint32_t &flag;std::uint32_t mutation;unsigned count=0;
 Calls(std::uint32_t *o,std::uint32_t &f,std::uint32_t m):out(o),flag(f),mutation(m){}
 void receipt(std::initializer_list<std::uint32_t> values){std::copy(values.begin(),values.end(),out+32+count*8);++count;out[1]=count;}
 void select_square()override{receipt({1,6});}
 void select_cursors()override{receipt({1,4});}
 void select_panels(sadx::texture_lifetime::List &)override{receipt({1,5});}
 void set_material(std::uint32_t a)override{receipt({2,a});}
 void set_base_color(std::uint32_t a)override{receipt({3,a});}
 void set_vertex_colors(const std::array<std::uint32_t,4> &a)override{receipt({4,a[0],a[1],a[2],a[3]});}
 void default_blending()override{receipt({5,0,8});receipt({5,1,6});}
 void draw_scaled(std::uint32_t id,float x,float y,float z,float sx,float sy)override{receipt({6,id,bits(x),bits(y),bits(z),bits(sx),bits(sy)});}
 void draw_shadow_window(float x,float y,float z,float w,float h)override{receipt({7,bits(x),bits(y),bits(z),bits(w),bits(h)});}
 void draw_content(sadx::title_menu::DialogContent,const d::ContentArguments &a)override{receipt({8,bits(a.center_x),bits(a.center_y),bits(a.depth),bits(a.scale_x),bits(a.scale_y),a.cursor});if(mutation!=0xffffffffU)flag=mutation;}
};
}
extern "C" void sadx_native_menu_dialog_square_probe(const std::uint32_t *i,std::uint32_t *o){
 std::fill_n(o,1024,0);sadx::TaskScheduler scheduler;auto task=scheduler.append_task(3,nullptr,{}).task;auto work=std::make_shared<d::Work>();task->auxiliary_native_work=work;
 d::Parameters p{};sadx::texture_lifetime::List list{};p.style=sadx::title_menu::DialogStyle::CyanSecond;p.content=i[9]?sadx::title_menu::DialogContent::FileSelectHelp:sadx::title_menu::DialogContent::None;p.textures=&list;std::vector<sadx::title_menu::DialogPanel> panels;for(unsigned n=0;n<i[40];++n)panels.push_back({number(i[41+n*3]),number(i[42+n*3]),static_cast<std::uint8_t>(i[43+n*3])});p.panels=std::make_shared<const std::vector<sadx::title_menu::DialogPanel>>(panels);p.choice_count=static_cast<std::int8_t>(i[40]);p.upper_color=i[10];p.lower_color=i[11];p.center_x=number(i[12]);p.center_y=number(i[13]);p.depth=number(i[14]);p.width=number(i[15]);p.height=number(i[16]);p.scale_x=number(i[17]);p.scale_y=number(i[18]);work->parameters=&p;work->color=i[0];work->scale_x=number(i[1]);work->scale_y=number(i[2]);work->fade=number(i[3]);work->cursor=static_cast<std::uint8_t>(i[4]);work->previous_choice=static_cast<std::int32_t>(i[5]);work->animation_value=static_cast<std::uint8_t>(i[6]);work->upper_color=i[19];work->lower_color=i[20];for(unsigned n=0;n<16;++n)work->choice_ids[n]=static_cast<std::int8_t>(i[24+n]);
 auto flag=i[7];const auto sx=number(i[21]),sy=number(i[22]);Calls calls(o,flag,i[8]);sadx::X87Context fp{127};d::draw(*work,{task},{sx,sy,flag},calls,fp);o[0]=flag;o[2]=fp.exception_flags();
}
