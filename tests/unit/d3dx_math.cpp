#include "d3dx/d3dx_math.hpp"
#include <cstring>
#include <iostream>
// D3DX8 774310/774150/772615: identity products, the w divide, NULL-argument switch arms and the viewport map.
namespace {using namespace sadx;void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::uint32_t b(float f){std::uint32_t v;std::memcpy(&v,&f,4);return v;}}
int main(){try{
    SseContext sse(0x1f80);X87Context fp(0x27f);
    const D3dxMatrix identity{b(1),0,0,0, 0,b(1),0,0, 0,0,b(1),0, 0,0,0,b(1)};
    const D3dxMatrix a{b(1),b(2),b(3),b(4), b(5),b(6),b(7),b(8), b(9),b(10),b(11),b(12), b(13),b(14),b(15),b(16)};
    require(d3dx_matrix_multiply_774310(identity,a,sse)==a&&d3dx_matrix_multiply_774310(a,identity,sse)==a,"identity products");
    require(d3dx_matrix_multiply_774310(a,a,sse)[0]==b(90.0F),"row 0 x column 0 of a*a");
    const D3dxMatrix scale_w{b(1),0,0,0, 0,b(1),0,0, 0,0,b(1),0, 0,0,0,b(2)};
    for(const bool aligned:{true,false}){
        const auto v=d3dx_vec3_transform_coord_774150({b(4),b(6),b(8)},scale_w,aligned,sse,sse_reciprocal_by_division);
        require(v[0]==b(2)&&v[1]==b(3)&&v[2]==b(4),"w = 2 halves every lane");}
    require(sse_reciprocal_by_division(0,0x1f80)==0x7f800000U&&sse_reciprocal_by_division(b(4),0x1f80)==b(0.25F),"division model");
    const D3dxViewport vp{0,0,640,480,0,b(1)};
    const auto none=d3dx_vec3_project_772615({b(0.5F),b(-0.5F),b(0.25F)},vp,std::nullopt,std::nullopt,std::nullopt,sse,fp,sse_reciprocal_by_division);
    require(none[0]==b(480)&&none[1]==b(360)&&none[2]==b(0.25F),"no matrices: the identity on the stack, then the viewport map");
    const auto projected=d3dx_vec3_project_772615({b(1),b(1),b(1)},std::nullopt,D3dxMatrixArgument{scale_w,true},std::nullopt,std::nullopt,sse,fp,sse_reciprocal_by_division);
    require(projected[0]==b(0.5F)&&projected[2]==b(0.5F),"projection alone, no viewport");
    std::cout<<"D3DX 774310/774150/772615: products, w divide, NULL arms and viewport map passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
