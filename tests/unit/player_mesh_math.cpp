#include "player/player_mesh_math.hpp"
#include "player/player_mesh_matrix.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool v,const char* message){if(!v)throw std::runtime_error(message);}
bool same(const sadx::NinjaVector& a,const sadx::NinjaVector& b){return std::memcmp(&a,&b,sizeof(a))==0;}
}
int main(){
    try {
        sadx::X87Context fp(0x027f);sadx::NinjaVector a{0,0,0},b{1,0,0},c{0,0,1},out{123,456,789};
        sadx::mesh_triangle_normal(a,b,c,out,fp);
        require(out.x==0 && out.y==-1 && out.z==0,"Source winding must preserve the downward normal");
        const auto expected=out;
        sadx::mesh_triangle_normal(a,b,c,a,fp);require(same(a,expected),"First input/output alias must not affect later components");
        a={0,0,0};sadx::mesh_triangle_normal(a,b,c,b,fp);require(same(b,expected),"Second input/output alias is valid");
        b={1,0,0};sadx::mesh_triangle_normal(a,b,c,c,fp);require(same(c,expected),"Third input/output alias is valid");
        c={0,0,1};sadx::mesh_triangle_normal(a,c,b,out,fp);require(out.y==1,"Reversed winding flips the source normal");
        const sadx::CoordinateAxisConstants axis{0,1};const sadx::CrtAsinRuntime bypass{1,{}};
        sadx::NinjaVector zero{0,0,0};auto normalized=sadx::normalize_mesh_vector_zero(zero,axis,bypass,fp);
        require(static_cast<bool>(normalized)&&zero.x==0&&zero.y==0&&zero.z==0,"450920 zero fallback is the source zero vector");
        normalized=sadx::normalize_mesh_vector_up(zero,axis,bypass,fp);
        require(static_cast<bool>(normalized)&&zero.x==0&&zero.y==1&&zero.z==0,"4509d0 zero fallback is source unitY");
        sadx::NinjaVector pending{1,2,3};const auto prior=pending;
        normalized=sadx::normalize_mesh_vector_zero(pending,axis,{{},{}},fp);
        require(!normalized && normalized.sqrt && normalized.sqrt->failure==sadx::CrtAsinFailure::unknown_matherr_policy &&
            normalized.sqrt_argument && normalized.squared_spill && same(pending,prior),"Missing CRT policy retains argument/pending facts and does not invent vector stores");
        sadx::NinjaMatrix singular{};require(!sadx::invert_mesh_matrix(singular,axis,fp),"Source singular inverse returnsfalse");
        for(const auto value:singular){std::uint32_t bits;std::memcpy(&bits,&value,4);require(bits==0x7f7fc99eU,"Singular source stores finite sentinel, not identity/zero");}
        sadx::NinjaMatrix identity{};identity[0]=identity[5]=identity[10]=identity[15]=1;
        const auto determinant=sadx::mesh_matrix_determinant(identity,fp);require(fp.to_float(determinant)==1,"Actual determinant keeps unit value");
        require(sadx::invert_mesh_matrix(identity,axis,fp)&&identity[0]==1&&identity[5]==1&&identity[10]==1&&identity[15]==1,"Nonsingular source inverse returns1");
        identity[12]=21;identity[13]=22;identity[14]=23;identity[15]=24;
        sadx::scale_mesh_matrix_vector(identity,{2,3,4},fp);
        require(identity[0]==2&&identity[5]==3&&identity[10]==4&&identity[12]==21&&identity[13]==22&&identity[14]==23&&identity[15]==24,
            "Mesh vector scaling preserves source translation row");
        std::cout<<"Mesh math: source winding/aliases, CRT partial state, singular sentinel and matrix scaling passed.\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
