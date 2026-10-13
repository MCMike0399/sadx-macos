#include "game/win32_profile.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
// The native GetPrivateProfileIntA/StringA re-implementation: section/key
// case folding, trimming, sign and digit parsing, fallbacks, truncation and the
// forms it flags as not interpreted like Win32.
namespace {
using namespace sadx;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::vector<std::uint8_t> bytes(const std::string& s){return {s.begin(),s.end()};}
}
int main(){try{
    const Win32Profile p(bytes("; comment\r\n[sonicDX]\r\nframerate=1\r\n  FogEmulation = 2 \r\nvoicev=-7x\r\nempty=\r\nname= pad one \r\n[other]\r\nframerate=3\r\n"));
    require(p.get_int("SONICDX","FRAMERATE",9)==1,"case-insensitive lookup in the first section");
    require(p.get_int("sonicDX","fogemulation",0)==2,"trimmed key and value");
    require(p.get_int("sonicDX","voicev",0)==-7,"sign then digits up to the first non-digit");
    require(p.get_int("sonicDX","empty",5)==0,"present but empty parses as 0");
    require(p.get_int("sonicDX","missing",-2)==-2,"absent key returns the fallback");
    require(p.get_int("other","framerate",0)==3,"other section");
    require(p.get_string("sonicDX","name","x",0x20)=="pad one","string value");
    require(p.get_string("sonicDX","name","x",4)=="pad","truncated to size-1");
    require(p.get_string("sonicDX","nothing","pad_default",0x20)=="pad_default","string fallback");
    require(p.unsupported.empty(),"plain values are interpreted");
    const Win32Profile q(bytes("[s]\na=\"1\"\nb=0x10\nc=12345678901\n"));
    require(q.unsupported.size()==3,"quoted, hex and overflowing values are flagged");
    std::cout<<"Win32 profile: lookup, trimming, integer parsing, fallbacks, truncation and flagged forms passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
