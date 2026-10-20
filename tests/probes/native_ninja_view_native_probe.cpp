#include "engine/ninja_view_native.hpp"
#include "cpu/x87_transcendental.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
float fl(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
std::uint32_t bits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
}
extern "C" int sadx_native_ninja_view(const std::uint32_t* in,const float* table,std::uint32_t* out){
using namespace sadx;
std::fill_n(out,200,0xa5a5a5a5U);
view::State state;
for(int i=0;i<3;++i){state.camera.position[i]=fl(in[3+i]);state.camera.direction[i]=fl(in[6+i]);}
state.camera.roll=static_cast<std::int32_t>(in[9]);state.camera.angle=static_cast<std::int32_t>(in[10]);state.camera.near_distance=fl(in[11]);state.camera.far_distance=fl(in[12]);
state.camera.tail.fill(fl(0xa5a5a5a5U));
if(in[2]==0)view::initialize_camera(state.camera);
std::memcpy(&state.depth.viewport,in+13,24);
D3dxMatrix screen;std::copy_n(in+19,16,screen.begin());
state.inverse.fill(0xa5a5a5a5U);state.perspective.fill(0xa5a5a5a5U);state.depth.projection.fill(0xa5a5a5a5U);state.device_view.fill(0xa5a5a5a5U);
NinjaMatrix filler;filler.fill(fl(0xa5a5a5a5U));
NinjaMatrixStack stack(std::vector<NinjaMatrix>(in[35],filler),in[36],0);
NinjaMath::SineTable sine;std::copy_n(table,1025,sine.begin());
struct Call{std::uint32_t offset,kind;D3dxMatrix matrix;};std::vector<Call> calls;
view::Services services{stack,sine,screen,[&](bool multiply,std::uint32_t kind,const D3dxMatrix& m){calls.push_back({multiply?0x9cu:0x94u,kind,m});},sse_reciprocal_by_division};
X87Context fp(static_cast<std::uint16_t>(in[0]));SseContext sse(in[1]);
view::Result r;
try{r=view::set_camera(state,services,fp,sse);}catch(const X87Error&){return 0;}
out[0]=r?0:5;out[1]=r?0:r.failure==view::Failure::TangentOutOfRange?0x791240:0x771706;out[2]=r.inverse_stored;
for(int i=0;i<16;++i){out[3+i]=bits(state.view[i]);out[19+i]=bits(stack.slots()[0][i]);out[35+i]=bits(state.view_copy[i]);out[54+i]=state.perspective[i];out[70+i]=state.depth.projection[i];out[86+i]=state.device_view[i];out[102+i]=state.inverse[i];}
out[51]=bits(state.sprite_scale);out[52]=bits(state.near_distance);out[53]=bits(state.far_distance);out[118]=stack.depth();out[119]=static_cast<std::uint32_t>(calls.size());
for(std::size_t i=0;i<calls.size();++i){out[120+i*18]=calls[i].offset;out[121+i*18]=calls[i].kind;std::copy(calls[i].matrix.begin(),calls[i].matrix.end(),out+122+i*18);}
out[174]=fp.control_word();for(int i=0;i<3;++i){out[175+i]=bits(state.camera.position[i]);out[178+i]=bits(state.camera.direction[i]);}out[181]=static_cast<std::uint32_t>(state.camera.roll);out[182]=static_cast<std::uint32_t>(state.camera.angle);
for(int i=0;i<6;++i)out[183+i]=bits(state.camera.tail[i]);out[189]=bits(state.camera.near_distance);out[190]=bits(state.camera.far_distance);return 1;
}
// Bochs models for the seeded profile. op 0 FPTAN(a), 1 FPATAN(y=a, x=b), 2 FSINCOS(a). Ext80 as (sign/exponent, significand).
// out: [0] ok [1] first se [2] second se ; sig [0] first [1] second (FSINCOS: sine, cosine)
extern "C" int sadx_native_ninja_view_x87(int op,std::uint32_t cw,std::uint32_t ase,std::uint64_t asig,std::uint32_t bse,std::uint64_t bsig,std::uint32_t* out,std::uint64_t* sig){
    using namespace sadx;X87Context fp(static_cast<std::uint16_t>(cw));
    out[0]=out[1]=out[2]=0;sig[0]=sig[1]=0;
    try{
    const auto a=fp.from_canonical_ext80(static_cast<std::uint16_t>(ase),asig);
    if(op==0){const auto t=X87Transcendental::fptan_bochs(fp,a);if(t){out[0]=1;out[1]=t->sign_exponent();sig[0]=t->significand();}}
    else if(op==1){const auto t=X87Transcendental::fpatan_bochs_p5_p6(fp,a,fp.from_canonical_ext80(static_cast<std::uint16_t>(bse),bsig));out[0]=1;out[1]=t.sign_exponent();sig[0]=t.significand();}
    else{const auto t=X87Transcendental::fsincos_bochs(fp,a);if(t){out[0]=1;out[1]=t->sine.sign_exponent();sig[0]=t->sine.significand();out[2]=t->cosine.sign_exponent();sig[1]=t->cosine.significand();}}
    }catch(const X87Error&){return 0;}
    return 1;
}
