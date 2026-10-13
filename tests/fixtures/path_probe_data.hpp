#pragma once
#include "player/player_path.hpp"
#include <cstring>
namespace sadx_probe {
inline float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
inline std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
inline std::int32_t integer(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
inline sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
inline sadx::PlayerPathResource resource(std::uint32_t type,std::uint32_t logical,float total,
    const std::uint32_t* points,std::size_t count){
    sadx::PlayerPathResource p{static_cast<std::uint8_t>(type),static_cast<std::uint16_t>(logical),total,{}};
    for(std::size_t i=0;i<count;++i){const auto w=points+i*5;std::uint16_t a=static_cast<std::uint16_t>(w[0]),b=static_cast<std::uint16_t>(w[0]>>16);std::int16_t x,y;std::memcpy(&x,&a,2);std::memcpy(&y,&b,2);p.points.push_back({x,y,number(w[1]),vector(w+2)});}return p;
}
inline sadx::PlayerPathStatus status(const std::uint32_t* w){
    sadx::PlayerPathStatus s{number(w[4])};s.angle0=integer(w[0]);s.angle4=integer(w[1]);s.angle8=integer(w[2]);s.anglec=integer(w[3]);
    sadx::PathVectorFacts* fields[]{&s.position_at14,&s.position_at20,&s.direction_at2c,&s.direction_at38,&s.tangent_at44};
    for(std::size_t k=0;k<5;++k)for(std::size_t i=0;i<3;++i)(*fields[k])[i]=number(w[5+k*3+i]);return s;
}
inline std::uint32_t store_status(const sadx::PlayerPathStatus& s,std::uint32_t* w){
    std::uint32_t mask=1U<<4;w[4]=bits(s.distance);
    const std::optional<std::int32_t>* angles[]{&s.angle0,&s.angle4,&s.angle8,&s.anglec};
    for(std::size_t i=0;i<4;++i)if(*angles[i]){mask|=1U<<i;w[i]=static_cast<std::uint32_t>(**angles[i]);}
    const sadx::PathVectorFacts* fields[]{&s.position_at14,&s.position_at20,&s.direction_at2c,&s.direction_at38,&s.tangent_at44};
    for(std::size_t k=0;k<5;++k)for(std::size_t i=0;i<3;++i)if((*fields[k])[i]){const auto index=5+k*3+i;mask|=1U<<index;w[index]=bits(*(*fields[k])[i]);}return mask;
}
}
