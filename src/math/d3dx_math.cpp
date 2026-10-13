#include "math/d3dx_math.hpp"
#include <cstring>
namespace sadx {
namespace {
using Lanes=std::array<Sse32,4>;
Lanes broadcast(std::uint32_t v){return {Sse32{v},Sse32{v},Sse32{v},Sse32{v}};}
Lanes row(const D3dxMatrix& m,std::size_t r){return {Sse32{m[4*r]},Sse32{m[4*r+1]},Sse32{m[4*r+2]},Sse32{m[4*r+3]}};}
Lanes mulps(SseContext& c,const Lanes& d,const Lanes& s){Lanes r;for(std::size_t i=0;i<4;++i)r[i]=c.multiply(d[i],s[i]);return r;}
Lanes addps(SseContext& c,const Lanes& d,const Lanes& s){Lanes r;for(std::size_t i=0;i<4;++i)r[i]=c.add(d[i],s[i]);return r;}
Lanes subps(SseContext& c,const Lanes& d,const Lanes& s){Lanes r;for(std::size_t i=0;i<4;++i)r[i]=c.subtract(d[i],s[i]);return r;}
Lanes rcpps(SseContext& c,const Lanes& s,const SseReciprocalEstimate& estimate){Lanes r;for(std::size_t i=0;i<4;++i)r[i]=Sse32{estimate(s[i].bits,c.mxcsr())};return r;}
// The Newton step and the lane-3 scale shared by both 774150 paths.
D3dxVector3 divide_by_w(SseContext& c,const Lanes& v,const SseReciprocalEstimate& estimate){
    const auto r=rcpps(c,v,estimate);
    const auto rv=mulps(c,mulps(c,r,v),r);
    const auto scale=broadcast(subps(c,addps(c,r,r),rv)[3].bits);
    const auto out=mulps(c,v,scale);
    return {out[0].bits,out[1].bits,out[2].bits};}
// FILD m32 then FADD 2^32 (0x4f800000) when negative: an unsigned load.
X87Value unsigned_load(X87Context& fp,std::uint32_t v){
    auto x=fp.from_int64(static_cast<std::int32_t>(v));
    if(static_cast<std::int32_t>(v)<0)x=fp.add_float_memory(x,4294967296.0F);
    return x;}
float bits_float(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
std::uint32_t float_bits(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
}
std::uint32_t sse_reciprocal_by_division(std::uint32_t bits,std::uint32_t mxcsr){
    SseContext scratch((mxcsr&0x6000U)|0x1f80U);
    return scratch.divide(Sse32{0x3f800000U},Sse32{bits}).bits;
}
D3dxMatrix d3dx_matrix_multiply_774310(const D3dxMatrix& a,const D3dxMatrix& b,SseContext& c){
    const Lanes b0=row(b,0),b1=row(b,1),b2=row(b,2),b3=row(b,3);
    D3dxMatrix out{};
    for(std::size_t i=0;i<4;++i){
        auto acc=mulps(c,broadcast(a[4*i]),b0);
        acc=addps(c,acc,mulps(c,broadcast(a[4*i+1]),b1));
        acc=addps(c,acc,mulps(c,broadcast(a[4*i+2]),b2));
        acc=addps(c,acc,mulps(c,broadcast(a[4*i+3]),b3));
        for(std::size_t j=0;j<4;++j)out[4*i+j]=acc[j].bits;
    }
    return out;
}
D3dxVector3 d3dx_vec3_transform_coord_774150(const D3dxVector3& v,const D3dxMatrix& m,bool aligned,SseContext& c,const SseReciprocalEstimate& estimate){
    if(aligned){ // 774171
        const auto zw=addps(c,mulps(c,broadcast(v[2]),row(m,2)),row(m,3));
        const auto xy=addps(c,mulps(c,broadcast(v[0]),row(m,0)),mulps(c,broadcast(v[1]),row(m,1)));
        return divide_by_w(c,addps(c,xy,zw),estimate);
    }
    // 7741cc: the matrix rows are the destinations.
    const auto xy=addps(c,mulps(c,row(m,0),broadcast(v[0])),mulps(c,row(m,1),broadcast(v[1])));
    const auto zw=addps(c,mulps(c,row(m,2),broadcast(v[2])),row(m,3));
    return divide_by_w(c,addps(c,xy,zw),estimate);
}
D3dxVector3 d3dx_vec3_project_772615(const D3dxVector3& v,const std::optional<D3dxViewport>& viewport,
    const std::optional<D3dxMatrixArgument>& projection,const std::optional<D3dxMatrixArgument>& view,const std::optional<D3dxMatrixArgument>& world,
    SseContext& c,X87Context& fp,const SseReciprocalEstimate& estimate){
    // 77261e..772654: switch on (world != 0) << 2 | (view != 0) << 1 | (projection != 0).
    D3dxMatrixArgument m{{},true};
    const auto product=[&](const D3dxMatrixArgument& a,const D3dxMatrixArgument& b){return D3dxMatrixArgument{d3dx_matrix_multiply_774310(a.matrix,b.matrix,c),true};};
    switch((world?4:0)|(view?2:0)|(projection?1:0)){
    case 0:m.matrix={0x3f800000U,0,0,0, 0,0x3f800000U,0,0, 0,0,0x3f800000U,0, 0,0,0,0x3f800000U};break; // FLDZ/FLD1 stores
    case 1:m=*projection;break;
    case 2:m=*view;break;
    case 3:m=product(*view,*projection);break;
    case 4:m=*world;break;
    case 5:m=product(*world,*projection);break;
    case 6:m=product(*world,*view);break;
    default:m=product(product(*world,*view),*projection);break;
    }
    auto out=d3dx_vec3_transform_coord_774150(v,m.matrix,m.aligned,c,estimate);
    if(!viewport)return out;
    const auto& vp=*viewport;
    // 772704: x = (x + 1) * Width * 0.5 + X; y = (1 - y) * Height * 0.5 + Y; z = (MaxZ - MinZ) * z + MinZ.
    auto x=fp.add_float_memory(fp.from_float(bits_float(out[0])),1.0F);
    x=fp.multiply(unsigned_load(fp,vp.width),x);
    x=fp.multiply_float_memory(x,0.5F);
    out[0]=float_bits(fp.to_float(fp.add(unsigned_load(fp,vp.x),x)));
    auto y=fp.subtract_float_memory(fp.from_float(1.0F),bits_float(out[1]));
    y=fp.multiply(unsigned_load(fp,vp.height),y);
    y=fp.multiply_float_memory(y,0.5F);
    out[1]=float_bits(fp.to_float(fp.add(unsigned_load(fp,vp.y),y)));
    auto z=fp.subtract_float_memory(fp.from_float(bits_float(vp.max_z)),bits_float(vp.min_z));
    z=fp.multiply_float_memory(z,bits_float(out[2]));
    out[2]=float_bits(fp.to_float(fp.add_float_memory(z,bits_float(vp.min_z))));
    return out;
}
namespace {
struct Xmm {std::array<Sse32,4> l{};};
struct Sse {
    SseContext& c;const SseReciprocalEstimate& estimate;
    static Xmm zero(){return {};}
    void movlps(Xmm& d,const D3dxMatrix& m,std::size_t at){d.l[0]=Sse32{m[at/4]};d.l[1]=Sse32{m[at/4+1]};}
    void movhps(Xmm& d,const D3dxMatrix& m,std::size_t at){d.l[2]=Sse32{m[at/4]};d.l[3]=Sse32{m[at/4+1]};}
    static void shufps(Xmm& d,const Xmm& s,unsigned imm){const Xmm a=d,b=s;d.l[0]=a.l[imm&3];d.l[1]=a.l[(imm>>2)&3];d.l[2]=b.l[(imm>>4)&3];d.l[3]=b.l[(imm>>6)&3];}
    void mulps(Xmm& d,const Xmm& s){for(std::size_t i=0;i<4;++i)d.l[i]=c.multiply(d.l[i],s.l[i]);}
    void addps(Xmm& d,const Xmm& s){for(std::size_t i=0;i<4;++i)d.l[i]=c.add(d.l[i],s.l[i]);}
    void subps(Xmm& d,const Xmm& s){for(std::size_t i=0;i<4;++i)d.l[i]=c.subtract(d.l[i],s.l[i]);}
    void addss(Xmm& d,const Xmm& s){d.l[0]=c.add(d.l[0],s.l[0]);}
    void mulss(Xmm& d,const Xmm& s){d.l[0]=c.multiply(d.l[0],s.l[0]);}
    void subss(Xmm& d,const Xmm& s){d.l[0]=c.subtract(d.l[0],s.l[0]);}
    void rcpss(Xmm& d,const Xmm& s){d.l[0]=Sse32{estimate(s.l[0].bits,c.mxcsr())};}
};
}
D3dxInverseResult d3dx_matrix_inverse_7757e0(const D3dxMatrix& m,SseContext& c,const SseReciprocalEstimate& estimate){
    Sse x{c,estimate};Xmm x0,x1,x2,x3,x4,x5,x6,x7;
    Xmm e0,d0,c0,b0,a0,f0,v100,v90,v80,v70,v60,v50,v40,v30;  // the stack slots var_E0 .. var_30
    x.movlps(x0,m,8);x.movlps(x4,m,0x28);x.movhps(x0,m,0x18);x.movhps(x4,m,0x38);
    x.movlps(x3,m,0x20);x.movlps(x1,m,0);x.movhps(x3,m,0x30);x.movhps(x1,m,0x10);
    x5=x0;Sse::shufps(x0,x4,0x88);Sse::shufps(x4,x5,0xdd);
    x5=x4;x.mulps(x5,x0);Sse::shufps(x5,x5,0xb1);x6=x5;Sse::shufps(x5,x5,0x4e);
    x7=x3;Sse::shufps(x3,x1,0xdd);Sse::shufps(x1,x7,0x88);
    x7=x3;x.mulps(x3,x5);x2=Sse::zero();x.mulps(x5,x1);e0=x5;
    x5=x7;x.mulps(x7,x6);x.mulps(x6,x1);x.subps(x3,x7);
    x7=x5;x.mulps(x5,x0);Sse::shufps(x0,x0,0x4e);Sse::shufps(x5,x5,0xb1);d0=x6;
    x6=x4;x.mulps(x6,x5);x.addps(x6,x3);
    x3=x5;Sse::shufps(x5,x5,0x4e);x.mulps(x3,x1);c0=x3;
    x3=x4;x.mulps(x3,x5);x.mulps(x5,x1);x.subps(x6,x3);
    x3=x7;Sse::shufps(x7,x7,0x4e);x.mulps(x7,x4);Sse::shufps(x7,x7,0xb1);b0=x5;
    x5=x0;x.mulps(x0,x7);x.addps(x0,x6);
    x6=x7;Sse::shufps(x7,x7,0x4e);x.mulps(x6,x1);a0=x4;
    x4=x5;x.mulps(x5,x7);x.mulps(x7,x1);x.subps(x0,x5);x.subps(x7,x6);
    x5=x1;x.mulps(x1,x0);Sse::shufps(x7,x7,0x4e);
    x6=x1;Sse::shufps(x1,x1,0x4e);f0=x0;x.addps(x1,x6);
    x0=x1;Sse::shufps(x1,x1,0xb1);x.addss(x1,x0);v100=x1;
    x0=x5;x.mulps(x5,x3);Sse::shufps(x5,x5,0xb1);x6=x5;Sse::shufps(x5,x5,0x4e);
    v90=x4;x4=a0;v80=x3;
    x3=x4;x.mulps(x3,x6);x.addps(x3,x7);
    x7=x4;x.mulps(x7,x5);x.subps(x7,x3);
    x3=x4;x.mulps(x3,x0);Sse::shufps(x3,x3,0xb1);v70=x5;
    x5=v80;v60=x6;x6=x5;x.mulps(x5,x3);x.addps(x5,x7);
    x7=x3;Sse::shufps(x3,x3,0x4e);v50=x4;
    x4=x6;x.mulps(x4,x3);v40=x6;x6=x5;x.subps(x6,x4);
    x4=e0;x5=d0;x.subps(x4,x5);Sse::shufps(x4,x4,0x4e);
    x5=v90;x.mulps(x7,x5);x.mulps(x3,x5);x.subps(x4,x7);x.mulps(x0,x5);x.addps(x4,x3);
    Sse::shufps(x0,x0,0xb1);x3=x0;Sse::shufps(x0,x0,0x4e);
    x7=v50;v30=x6;x6=x7;x.mulps(x7,x3);x.mulps(x6,x0);x.addps(x4,x7);x.subps(x4,x6);
    x6=b0;x7=c0;x.subps(x6,x7);Sse::shufps(x6,x6,0x4e);
    x7=v60;x.mulps(x7,x5);x.subps(x7,x6);
    x6=v70;x.mulps(x5,x6);x.subps(x7,x5);
    x5=v40;x.mulps(x3,x5);x.mulps(x5,x0);x.subps(x7,x3);
    x0=x2;x2=x5;x.addps(x2,x7);
    const auto compared=c.comiss(x1.l[0],x0.l[0]);
    x6=v30;
    if(compared==SseComparison::equal||compared==SseComparison::unordered)return {};
    D3dxInverseResult r;r.determinant=x1.l[0].bits;
    x0=x1;x1=f0;
    x3=x0;x.rcpss(x0,x0);x5=x0;x.mulss(x0,x0);x.mulss(x3,x0);x.addss(x5,x5);x.subss(x5,x3);Sse::shufps(x5,x5,0x0);
    x.mulps(x1,x5);x.mulps(x4,x5);x.mulps(x6,x5);x.mulps(x5,x2);
    D3dxMatrix out{};for(std::size_t i=0;i<4;++i){out[i]=x1.l[i].bits;out[4+i]=x4.l[i].bits;out[8+i]=x6.l[i].bits;out[12+i]=x5.l[i].bits;}
    r.matrix=out;(void)v100;
    return r;
}
std::optional<D3dxMatrix> d3dx_perspective_7716df(float fovy,float aspect,float near_plane,float far_plane,X87Context& fp){
    const float half=fp.to_float(fp.multiply_float_memory(fp.from_float(fovy),0.5F));
    const auto sc=X87Transcendental::fsincos_bochs(fp,fp.from_float(half));
    if(!sc)return std::nullopt;
    const float cosine=fp.to_float(sc->cosine),sine=fp.to_float(sc->sine); // FSTP [fovy slot] = cos, FSTP [ebp-4] = sin
    const auto cotangent=fp.divide_float_memory(fp.from_float(cosine),sine);
    D3dxMatrix m{};
    m[0]=float_bits(fp.to_float(fp.divide(cotangent,fp.from_float(aspect)))); // FLD aspect ; FDIVR ST0,ST1: ST1 / ST0
    m[5]=float_bits(fp.to_float(cotangent));
    const auto depth=fp.reverse_divide_float_memory(fp.subtract_float_memory(fp.from_float(far_plane),near_plane),far_plane);
    m[10]=float_bits(fp.to_float(depth));
    m[11]=0x3f800000U;
    m[14]=float_bits(fp.to_float(fp.multiply_float_memory(depth,near_plane).negated()));
    return m;
}
} // namespace sadx
