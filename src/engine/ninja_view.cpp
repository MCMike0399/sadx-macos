#include "engine/ninja_view.hpp"
#include "cpu/x87_transcendental.hpp"
#include <cstring>
namespace sadx {
namespace {
struct Stop{};
float bits_float(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
std::uint32_t float_bits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
constexpr float threshold_at_7dce54=9.99999997e-07F;    // FCOMP m32 at 7812cd
constexpr double one_at_7e0b40=1.0;                     // FDIVR m64 at 7812b2 / 7812e0
constexpr float one_at_7dcb10=1.0F,zero_at_7dcb2c=0.0F; // the vertical-direction yaw (7812f8)
constexpr double angle_to_radians_at_7e5d60=9.5873779296875e-05; // 2 pi / 65536 as stored
constexpr float half_at_7dcc98=0.5F;
X87Value unsigned_load(X87Context& fp,std::uint32_t v){ // FILD m32, FADD 2^32 (7dcb7c) when negative
    auto x=fp.from_int64(static_cast<std::int32_t>(v));
    if(static_cast<std::int32_t>(v)<0)x=fp.add_float_memory(x,4294967296.0F);
    return x;}
struct View {
    NinjaViewRuntime& rt;NinjaViewResult r;
    explicit View(NinjaViewRuntime& r_):rt(r_){}
    [[noreturn]]void fail(NinjaViewFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(NinjaViewFailure::unknown_state,at,fact);return *v;}
    D3D8FrameCommand device(std::uint32_t load,std::uint32_t dereference,std::uint32_t site){
        if(!rt.render.device_at_3d128b0)fail(NinjaViewFailure::unknown_device,load,0x3d128b0);
        if(!*rt.render.device_at_3d128b0)fail(NinjaViewFailure::null_device,dereference);
        return {*rt.render.device_at_3d128b0,site};}
    void transform(bool multiply,std::uint32_t state,const D3dxMatrix& m,std::uint32_t load,std::uint32_t dereference,std::uint32_t site){
        const auto c=device(load,dereference,site);
        const auto& f=multiply?rt.device.multiply_transform:rt.device.set_transform;
        if(!f||!f(c,state,m))fail(NinjaViewFailure::device_unavailable,site);}
    float camera(std::size_t word,std::uint32_t at){return bits_float(need(rt.state.camera_at_3aad0a0[word],at,0x3aad0a0+4*static_cast<std::uint32_t>(word)));}
    std::uint32_t width(std::uint32_t at){return need(rt.device_state.viewport_at_3d12780[2],at,0x3d12788);}

    void projection(float fov,float far_plane){
        auto& fp=rt.fp;
        auto aspect=unsigned_load(fp,width(0x791170));
        const auto height=unsigned_load(fp,need(rt.device_state.viewport_at_3d12780[3],0x79118a,0x3d1278c));
        const float ratio=fp.to_float(fp.divide(aspect,height));                       // FDIVP; FSTP [esp+0x10]
        const auto half=fp.multiply_float_memory(fp.from_float(fov),half_at_7dcc98);
        const auto tangent=X87Transcendental::fptan_bochs(fp,half);                    // 791240 (then FSTP ST0 drops the 1.0)
        if(!tangent)fail(NinjaViewFailure::fp_out_of_range,0x791240);
        const auto scaled=fp.divide_float_memory(*tangent,ratio);
        const auto angle=X87Transcendental::fpatan_bochs_p5_p6(fp,scaled,fp.from_float(1.0F)); // FLD1; FPATAN
        const float fovy=fp.to_float(fp.add(angle,angle));
        const auto perspective=d3dx_perspective_7716df(fovy,ratio,1.0F,far_plane,fp);     // 791256
        if(!perspective)fail(NinjaViewFailure::fp_out_of_range,0x771706);
        for(std::size_t i=0;i<16;++i)rt.state.perspective_at_3d12960[i]=(*perspective)[i];
        D3dxMatrix flip{};flip[0]=float_bits(1.0F);flip[5]=float_bits(-1.0F);flip[10]=float_bits(1.0F);flip[15]=float_bits(1.0F);
        const auto product=d3dx_matrix_multiply_774310(flip,*perspective,rt.sse);         // 79126a (770804, slot 3)
        for(std::size_t i=0;i<16;++i)rt.screen.projection_at_3d129c0[i]=product[i];
        transform(false,3,product,0x79126f,0x791274,0x79127e);
        D3dxMatrix screen{};for(std::size_t i=0;i<16;++i)screen[i]=need(rt.device_state.matrix_at_3d0fd80[i],0x791293,0x3d0fd80+4*static_cast<std::uint32_t>(i));
        transform(true,3,screen,0x791284,0x791289,0x791293);
        D3dxMatrix current{};std::memcpy(current.data(),rt.matrix_stack.current().data(),64);
        transform(false,2,current,0x79129f,0x7912a4,0x7912aa);
        for(std::size_t i=0;i<16;++i)rt.state.view_at_389d398[i]=current[i];          // REP MOVSD at 7912c9
        const auto inverse=d3dx_matrix_inverse_7757e0(current,rt.sse,rt.reciprocal);    // 7912cb (770d10, slot 18)
        if(inverse.matrix){r.inverse_stored=true;for(std::size_t i=0;i<16;++i)rt.state.view_inverse_at_389d358[i]=(*inverse.matrix)[i];}
    }
    void view(){
        auto& fp=rt.fp;NinjaMath math(rt.sine_table,fp);
        const auto roll=static_cast<std::int32_t>(0U-need(rt.state.camera_at_3aad0a0[6],0x781258,0x3aad0b8));
        const float s=math.sin(roll);                       // 78125f, FSTP [arg slot]
        const float c=math.cos(roll);                       // 78126d, FSTP [esp+0xc]
        const auto dz=fp.from_float(camera(5,0x781276)),dx=fp.from_float(camera(3,0x781279));
        const float horizontal_squared=fp.to_float(fp.add(fp.multiply(dx,dx),fp.multiply(dz,dz)));
        const float horizontal=fp.to_float(signed_sqrt_extended(horizontal_squared,fp)); // 781293
        const float dy_bits=camera(4,0x78129d);const auto dy=fp.from_float(dy_bits);
        const float length_squared=fp.to_float(fp.add_float_memory(fp.multiply(dy,dy),horizontal_squared));
        const auto inverse=fp.reverse_divide_double_memory(signed_sqrt_extended(length_squared,fp),one_at_7e0b40);
        const float cos_pitch=fp.to_float(fp.multiply(fp.from_float(horizontal),inverse));
        const float sin_pitch=fp.to_float(fp.multiply_float_memory(inverse,dy_bits));
        X87Value y0=fp.from_float(zero_at_7dcb2c),y1=y0;
        if(fp.compare_float_memory(fp.from_float(horizontal_squared),threshold_at_7dce54)==X87Comparison::greater){
            const auto inverse_horizontal=fp.reverse_divide_double_memory(signed_sqrt_extended(horizontal_squared,fp),one_at_7e0b40);
            const float inverse_horizontal_float=fp.to_float(inverse_horizontal);   // FST [esp+0x18]
            y1=fp.multiply_float_memory(inverse_horizontal,camera(5,0x7812ea));
            y0=fp.multiply_float_memory(fp.from_float(inverse_horizontal_float),camera(3,0x7812f1)).negated();
        }else{y1=fp.from_float(one_at_7dcb10);y0=fp.from_float(zero_at_7dcb2c);}
        auto& v=rt.state.view_at_3d0f760;std::array<float,16> m{};
        const auto store=[&](std::size_t i,float value){m[i]=value;v[i]=float_bits(value);};
        const auto sp_y0=fp.multiply(fp.from_float(sin_pitch),y0);
        v[3]=0;m[3]=0;v[6]=float_bits(sin_pitch);m[6]=sin_pitch;v[7]=0;m[7]=0;v[11]=0;m[11]=0;
        store(0,fp.to_float(fp.subtract(fp.multiply_float_memory(y1,c),fp.multiply_float_memory(sp_y0,s))));
        store(1,fp.to_float(fp.add(fp.multiply_float_memory(sp_y0,c),fp.multiply_float_memory(y1,s))));
        store(2,fp.to_float(fp.multiply(fp.from_float(cos_pitch),y0).negated()));
        store(4,fp.to_float(fp.multiply_float_memory(fp.from_float(cos_pitch),s).negated()));
        store(5,fp.to_float(fp.multiply_float_memory(fp.from_float(cos_pitch),c)));
        const auto y1_sp=fp.multiply_float_memory(y1,sin_pitch);
        store(8,fp.to_float(fp.add(fp.multiply(fp.from_float(s),y1_sp),fp.multiply_float_memory(y0,c))));
        store(9,fp.to_float(fp.subtract(fp.multiply_float_memory(y0,s),fp.multiply_float_memory(y1_sp,c))));
        store(10,fp.to_float(fp.multiply_float_memory(y1,cos_pitch)));
        const float px=camera(0,0x7813cc),py=camera(1,0x7813d8),pz=camera(2,0x7813c1);
        const auto translation=[&](float a,float b,float c2){ // -(a*pz) + -(b*px) + -(c2*py), FADDP order
            const auto first=fp.multiply_float_memory(fp.from_float(a),pz).negated();
            const auto second=fp.multiply_float_memory(fp.from_float(b),px).negated();
            return fp.to_float(fp.add(fp.add(first,second),fp.multiply_float_memory(fp.from_float(c2),py).negated()));};
        store(12,translation(m[8],m[0],m[4]));
        store(13,translation(m[9],m[1],m[5]));
        v[15]=float_bits(1.0F);m[15]=1.0F;
        rt.matrix_stack.reset_to_first();
        store(14,translation(m[10],m[2],sin_pitch));
        // Slot 0: columns 0 and 1 negated through FLD/FCHS/FSTP, column 2 copied as raw words, column 3 = 0,0,0,1.
        auto& slot=rt.matrix_stack.current();
        const auto negate=[&](float x){return fp.to_float(fp.from_float(x).negated());};
        for(std::size_t row=0;row<4;++row){
            slot[row*4]=negate(m[row*4]);slot[row*4+1]=negate(m[row*4+1]);
            std::uint32_t raw=*v[row*4+2];std::memcpy(&slot[row*4+2],&raw,4);
            slot[row*4+3]=row==3?1.0F:0.0F;}
        for(std::size_t i=0;i<16;++i){std::uint32_t w;std::memcpy(&w,&slot[i],4);rt.state.view_copy_at_3d0f980[i]=w;}
        // 7814e7..781523: the sprite projection scale from the viewport width and half the angle (787e30).
        const auto angle_word=need(rt.state.camera_at_3aad0a0[7],0x78150a,0x3aad0bc);
        const float width_float=fp.to_float(unsigned_load(fp,width(0x7814e7)));
        const auto signed_angle=static_cast<std::int32_t>(angle_word);
        const std::int32_t half_angle=(signed_angle+(signed_angle<0?1:0))>>1;          // CDQ; SUB; SAR 1
        const float half_sine=math.sin(half_angle);
        const auto tangent=fp.reverse_divide_float_memory(math.cos_extended(half_angle),half_sine);
        rt.projection.scale_at_3d0f9e0=fp.to_float(fp.reverse_divide_float_memory(fp.add(tangent,tangent),width_float));
        rt.range.near_at_3d0f808=fp.to_float(fp.from_float(need(rt.range.cache_x_at_3aad0c0,0x781529,0x3aad0c0)).negated());
        rt.range.far_at_3d0f968=fp.to_float(fp.from_float(need(rt.range.cache_y_at_3aad0c4,0x781534,0x3aad0c4)).negated());
        const float fov=fp.to_float(fp.multiply_double_memory(fp.from_int64(signed_angle),angle_to_radians_at_7e5d60));
        projection(fov,*rt.range.far_at_3d0f968);
    }
};
} // namespace
void ninja_view_init_781570(NinjaViewState& s,NinjaProjectionRangeState& range){
    s.camera_at_3aad0a0={0,0,0,0,0,0xbf800000U,0,0x2aaaU};
    range.cache_x_at_3aad0c0=bits_float(0xbf800000U);range.cache_y_at_3aad0c4=bits_float(0xc76a6000U);
    s.camera_tail_at_3aad0c8={0x3f800000U,0,0,0,0x3f800000U,0};
}
NinjaViewResult ninja_set_view_781250(NinjaViewRuntime& rt){
    View v(rt);try{v.view();}catch(const Stop&){}return v.r;
}
NinjaViewResult ninja_set_projection_791170(float fov,float near_plane,float far_plane,NinjaViewRuntime& rt){
    (void)near_plane;View v(rt);try{v.projection(fov,far_plane);}catch(const Stop&){}return v.r;
}
} // namespace sadx
