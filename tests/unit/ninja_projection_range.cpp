#include "engine/ninja_projection_range.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}}
int main(){try{
    sadx::NinjaProjectionRangeState s;s.cache_x_at_3aad0c0=0;s.cache_y_at_3aad0c4=0;s.negative_one_at_7dcb0c=-1.0F;s.limit_at_7dce50=-65535.0F;
    sadx::NinjaMatrix m{};std::optional<sadx::NinjaMatrix> written;sadx::NinjaTransformOwner t{[&](std::uint32_t){return std::optional<sadx::NinjaMatrix>{m};},[&](std::uint32_t,const sadx::NinjaMatrix& x){written=x;return true;}};
    sadx::X87Context fp(0x27f);auto r=sadx::ninja_set_projection_range_403180(-1.0F,-3000.0F,s,t,fp);
    require(r&&r.transform_updated&&s.near_at_3d0f808==1.0F&&s.far_at_3d0f968==3000.0F&&s.cache_x_at_3aad0c0==-1.0F&&s.cache_y_at_3aad0c4==-3000.0F,"near -1 / far -3000 stored negated and cached");
    require(written&&std::fabs((*written)[10]-3000.0F/2999.0F)<1e-6F,"Q = (-Y)/(X-Y) with X=-1, Y=-3000");
    written.reset();r=sadx::ninja_set_projection_range_403180(-1.0F,-3000.0F,s,t,fp);require(r&&!r.transform_updated&&!written,"unchanged pair keeps the transform");
    r=sadx::ninja_set_projection_range_403180(5.0F,-1e9F,s,t,fp);require(r&&s.near_at_3d0f808==1.0F&&s.far_at_3d0f968==65535.0F,"clamp near to -1 and far to -65535");
    s.cache_x_at_3aad0c0.reset();require(sadx::ninja_set_projection_range_403180(-2.0F,-3.0F,s,t,fp).source_boundary==0x4031b9,"unknown cache");
    std::cout<<"Projection range: 7815e0 clamps, 403180 cache/transform update and boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
