#include "game/message_frame.hpp"
#include "game/saved_fov.hpp"
#include "cpu/fidelity_float.hpp"
#include <algorithm>
#include <cstring>
namespace {
std::uint32_t bits(float f){std::uint32_t u;std::memcpy(&u,&f,4);return u;}
float value(std::uint32_t u){float f;std::memcpy(&f,&u,4);return f;}
struct Calls final:sadx::message_frame::Calls {
 const std::uint32_t *input;std::uint32_t *out;std::array<sadx::menu_caption::Context,4> &contexts;
 sadx::cleanup::MessageState &queue;std::int32_t &saved;std::uint32_t &fov;float &horizontal,&vertical;
 sadx::X87Context &fp;std::array<sadx::sprite::AnimationFrame,4> animations{};
 unsigned count=0,allocated=0; sadx::menu_caption::Context *current=nullptr;
 sadx::message_frame::SpriteInput sprite{0,0,0,0,nullptr,nullptr};
 Calls(const std::uint32_t*i,std::uint32_t*o,std::array<sadx::menu_caption::Context,4>&c,
       sadx::cleanup::MessageState&q,std::int32_t&s,std::uint32_t&f,float&h,float&v,sadx::X87Context&p)
   :input(i),out(o),contexts(c),queue(q),saved(s),fov(f),horizontal(h),vertical(v),fp(p){}
 void event(std::uint32_t tag,std::initializer_list<std::uint32_t> args={}){
   if(count>=120)throw std::runtime_error("probe event storage exhausted");
   auto *record=out+64+count++*8;record[0]=tag;std::copy(args.begin(),args.end(),record+1);out[0]=count;
 }
 sadx::menu_caption::Context &resolve(const sadx::cleanup::Message *entry)override{
   for(auto &context:contexts)if(entry==&context){current=&context;return context;}
   throw std::runtime_error("unknown context");
 }
 std::uint32_t current_fov()override{event(1);return fov;}
 void set_fov(std::uint32_t v)override{event(2,{v});fov=v;if(input[10]==4)saved=input[7];}
 void set_depth_comparison(std::uint32_t v)override{event(3,{v});}
 void set_depth_write(bool v)override{event(4,{v});}
 void default_blending()override{event(5);}
 void set_blend(std::uint32_t a,std::uint32_t b)override{event(6,{a,b});}
 void set_panel_color(std::uint32_t color)override{event(7,{color,color,color,color});}
 void draw_panel(float x,float y,float z,float w,float h)override{event(8,{bits(x),bits(y),bits(z),bits(w),bits(h)});}
 void draw_box(float l,float t,float r,float b,float p,std::uint32_t color)override{event(9,{bits(l),bits(t),bits(r),bits(b),bits(p),color});}
 void upload_texture(sadx::menu_caption::Context &context)override{
   event(10,{static_cast<std::uint32_t>(&context-contexts.data())});
   context.texture_names_attached=input[8]!=0;context.flags&=~2U;
 }
 sadx::sprite::AnimationFrame &allocate_animation(std::uint32_t bytes)override{
   event(11,{bytes});if(input[10]==1){current->buffer_width=257;current->buffer_height=129;}
   if(allocated>=animations.size())throw std::runtime_error("probe animation storage exhausted");
   return animations[allocated++];
 }
 void set_material(const std::array<float,4> &color)override{
   event(12,{bits(color[0]),bits(color[1]),bits(color[2]),bits(color[3])});
   if(input[10]==2){current->kind=3;current->x=777;current->y=-777;current->scale=2;horizontal=1.5f;vertical=2.5f;queue.count=0;}
 }
 void publish_centered_sprite(const sadx::message_frame::SpriteInput &s)override{
   sprite=s;using sadx::fidelity::ExactFloat;
   sprite.x=(ExactFloat(fp,horizontal)*320.0f+(ExactFloat(fp,s.x)-320.0f)).rounded();
   sprite.y=(ExactFloat(fp,vertical)*240.0f+(ExactFloat(fp,s.y)-240.0f)).rounded();
   event(13,{bits(sprite.x),bits(sprite.y),bits(sprite.scale_x),bits(sprite.scale_y)});
 }
 void set_sprite_y(float y)override{sprite.y=y;}
 void submit_sprite(std::int32_t frame,float priority,std::uint32_t attr,std::uint32_t flags)override{
   event(14,{bits(sprite.x),bits(sprite.y),bits(sprite.scale_x),bits(sprite.scale_y),bits(priority),attr,flags});
   std::uint32_t words[5];std::memcpy(words,sprite.animation,20);
   event(15,{words[0],words[1],words[2],words[3],words[4],static_cast<std::uint32_t>(frame)});
 }
 void complete_caption_frame()override{event(16,{queue.count});if(input[10]==3)queue.count=1;}
};
}
extern "C" void sadx_native_message_frame_probe(const std::uint32_t *i,std::uint32_t *out){
 std::fill_n(out,1024,0);std::array<sadx::menu_caption::Context,4> contexts{};
 sadx::cleanup::MessageState queue;queue.count=i[0];
 for(unsigned n=0;n<4;++n){auto &c=contexts[n];const auto *v=i+16+n*16;
  c.kind=static_cast<std::uint8_t>(v[0]);c.x=static_cast<std::int16_t>(v[1]);c.y=static_cast<std::int16_t>(v[2]);
  c.width=static_cast<std::uint16_t>(v[3]);c.height=static_cast<std::uint16_t>(v[4]);
  c.buffer_width=static_cast<std::uint16_t>(v[5]);c.buffer_height=static_cast<std::uint16_t>(v[6]);
  c.scale=value(v[7]);c.foreground=v[8];c.background=v[9];c.flags=v[10];c.texture_names_attached=v[11]!=0;
  queue.entries[n]=(i[6]&(1U<<n))?nullptr:&c;
 }
 sadx::view::SavedFovState saved_fov; sadx::view::set_saved_fov_bits(saved_fov,i[1]); auto &saved=saved_fov.value; auto fov=i[2];auto horizontal=value(i[3]),vertical=value(i[4]);sadx::X87Context fp(static_cast<std::uint16_t>(i[5]));
 Calls calls(i,out,contexts,queue,saved,fov,horizontal,vertical,fp);
 try {
  sadx::message_frame::render({queue,saved,horizontal,vertical},calls,fp);
 } catch (const std::exception &) {
  out[1023]=1;
 }
 out[1]=saved;out[2]=fov;out[3]=queue.count;out[4]=fp.exception_flags();
 for(unsigned n=0;n<4;++n){out[16+n*3]=contexts[n].kind;out[17+n*3]=contexts[n].flags;out[18+n*3]=contexts[n].texture_names_attached;}
}
