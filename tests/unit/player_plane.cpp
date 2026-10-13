#include "player/player_plane.hpp"
#include <iostream>
#include <stdexcept>

namespace { void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);} }
int main(){
    try {
        const sadx::PlayerPlaneConstants c{{0,1},0.9750000238418579F,1};
        for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
            sadx::X87Context fp(pc|rc|0x7f);
            sadx::NinjaPlane a{{2,0,0},{1,0,0}},b{{0,3,0},{0,1,0}},out{};
            require(!sadx::source_planes_parallel(a,b,c,fp),"perpendicular planes take intersection body");
            const auto value=sadx::intersect_source_planes(a,b,&out,c,fp);
            require(fp.to_float(value)==0 && out.point.x==2 && out.point.y==3 && out.point.z==0 &&
                    out.normal.x==0 && out.normal.y==0 && out.normal.z==1,"source intersection point and direction");
            const sadx::NinjaPlane parallel{{5,0,0},{1,0,0}};
            out={{9,8,7},{6,5,4}};
            require(sadx::source_planes_parallel(a,parallel,c,fp),"source threshold selects parallel planes");
            const auto distance=sadx::intersect_source_planes(a,parallel,&out,c,fp);
            require(fp.to_float(distance)==3 && out.point.x==0 && out.point.y==0 && out.point.z==0 &&
                    out.normal.x==0 && out.normal.y==0 && out.normal.z==0,"parallel path clears six fields then returns real distance");
            require(fp.to_float(sadx::intersect_source_planes(a,b,nullptr,c,fp))==0,
                    "source null output skips intersection math and returns zero");
        }
        std::cout<<"Source plane intersection: perpendicular/parallel/null paths and 12 control words passed.\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
