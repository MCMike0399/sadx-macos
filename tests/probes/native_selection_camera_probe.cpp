#include "game/selection_camera.hpp"
#include <cstring>
namespace {
float number(std::uint32_t w) {float f;std::memcpy(&f,&w,4);return f;}
std::uint32_t bits(float f) {std::uint32_t w;std::memcpy(&w,&f,4);return w;}
std::int32_t integer(std::uint32_t w) {std::int32_t i;std::memcpy(&i,&w,4);return i;}
std::array<std::uint32_t,8> words(const sadx::view::Camera &c) {return {bits(c.position[0]),bits(c.position[1]),bits(c.position[2]),bits(c.direction[0]),bits(c.direction[1]),bits(c.direction[2]),static_cast<std::uint32_t>(c.roll),static_cast<std::uint32_t>(c.angle)};}
struct Calls : sadx::selection_camera::Calls {
 unsigned count=0;std::array<std::uint32_t,8> received{};
 void set_camera(sadx::view::Camera &camera) override {++count;received=words(camera);}
};
}
extern "C" int sadx_native_selection_camera(const std::uint8_t *table_bytes,const std::uint8_t *unit_bytes,
 const std::uint32_t *input,std::uint16_t cw,std::uint32_t *out) {
 try {
 sadx::NinjaMath::SineTable table;std::memcpy(table.data(),table_bytes,4100);
 sadx::NinjaMatrix unit;std::memcpy(unit.data(),unit_bytes,64);
 std::vector<sadx::NinjaMatrix> slots(6);
 for(unsigned i=0;i<96;++i)slots[i/16][i%16]=number(input[16+i]);
 sadx::NinjaMatrixStack stack(std::move(slots),input[6],input[7]);
 sadx::NinjaMatrixPrimitives matrices(table,unit,stack);sadx::X87Context fp(cw);
 std::array<std::int32_t,3> angles{integer(input[0]),integer(input[1]),integer(input[2])};
 sadx::NinjaVector pos{number(input[3]),number(input[4]),number(input[5])};
 sadx::view::Camera camera;for(unsigned i=0;i<3;++i){camera.position[i]=number(input[8+i]);camera.direction[i]=number(input[11+i]);}camera.roll=integer(input[14]);camera.angle=integer(input[15]);
 Calls calls;sadx::selection_camera::update({angles,pos},camera,matrices,calls,fp);
 auto w=words(camera);for(unsigned i=0;i<8;++i)out[i]=w[i];out[8]=stack.depth();out[9]=fp.exception_flags();out[10]=calls.count;
 for(unsigned i=0;i<8;++i)out[11+i]=calls.received[i];
 for(unsigned i=0;i<96;++i)out[19+i]=bits(stack.slots()[i/16][i%16]);
 return 0;
 }catch(...){return 1;}
}
