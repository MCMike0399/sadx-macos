#include "engine/d3d8_caps.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
}
int main(){try{
    const auto caps=sadx::assumed_d3d8_caps();
    const auto word=[&](std::size_t at){std::uint32_t v=0;for(unsigned k=0;k<4;++k)v|=std::uint32_t(caps.at(at+k))<<(8*k);return v;};
    require(caps.size()==0xd4,"D3DCAPS8 wire size");
    require(word(0)==1&&word(0x24)==0x100000&&word(0x3c)==0x1ec05,"HAL, WFOG and texture capability fields");
    for(std::size_t at=0;at<caps.size();at+=4)
        if(at!=0&&at!=0x24&&at!=0x3c)require(word(at)==0,"other fields use the reported zero-cap assumption");
    require(std::string(sadx::d3d8_caps_assumption()).find("were not captured")!=std::string::npos,"reference GPU remains unknown");
    std::cout<<"D3D8 caps: 0xd4 wire bytes, HAL, WFOG and mipmap/cube-map texture capabilities; reference GPU explicitly unknown\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
