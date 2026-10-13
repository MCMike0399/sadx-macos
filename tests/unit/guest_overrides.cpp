// M1 4d: override-equivalence. For every entry of the frame-loop override table, two guests with the same
// sonic.exe image and the same randomised globals run the same call: one executes the original bytes, the other
// the native port through the override bridge. The ranges the original may write must be byte-identical.
// Cases are generated here with the input classes of each function's oracle (thresholds, signs, control words);
// the oracles' own case files are per-oracle formats and are not decoded.
//   guest-overrides-test SONIC.EXE
#include "guest/override_table.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
std::uint32_t fb(float f){std::uint32_t b;std::memcpy(&b,&f,4);return b;}
struct Case {std::vector<std::pair<std::uint32_t,std::uint32_t>> memory;std::vector<std::uint32_t> arguments;std::optional<std::uint32_t> ecx;std::uint16_t cw;};
}
int main(int argc,char** argv){try{
    require(argc==2,"usage: guest-overrides-test SONIC.EXE");
    std::ifstream f(argv[1],std::ios::binary);std::string error;
    const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    const auto table=frame_loop_overrides(*image);
    std::mt19937 rng(0x0d1d);const auto u32=[&](){return static_cast<std::uint32_t>(rng());};
    const auto pick=[&](std::initializer_list<std::uint32_t> v){return *(v.begin()+rng()%v.size());};
    const auto cw=[&]()->std::uint16_t{return static_cast<std::uint16_t>(pick({0x7f,0x27f,0x37f,0x47f,0x87f,0xc7f}));};
    std::size_t total=0;
    for(const auto& entry:table){
        auto original=X86Guest::create(&error);auto native=X86Guest::create(&error);require(original&&native,error);
        require(original->map_image(*image,&error)&&native->map_image(*image,&error),error);
        require(native->add_override(entry.address,entry.bridge,&error),error);
        std::vector<Case> cases;
        for(int n=0;n<2000;++n){
            Case c;c.cw=cw();
            if(entry.address==0x7899a0){
                c.memory={{0x3d08538,n%7?u32():10000000U},{0x3d0853c,n%3?0U:u32()%16},{0x389d7dc,u32()},{0x3d084a8,u32()},{0x3d084ac,u32()}};
                c.arguments={n%5==0?u32():pick({0,1,2,3,4,5})};}
            else if(entry.address==0x7815e0){
                const auto value=[&](){switch(rng()%8){case 0:return fb(-1.0F);case 1:return fb(-65535.0F);case 2:return fb(-0.5F);case 3:return fb(-70000.0F);
                    case 4:return fb(std::uniform_real_distribution<float>(-100000,100)(rng));case 5:return fb(0.0F);case 6:return 0x80000000U;default:return fb(std::uniform_real_distribution<float>(-10,0)(rng));}};
                c.memory={{0x3d0f808,u32()},{0x3d0f968,u32()}};c.arguments={value(),value()};}
            else if(entry.address==0x7815c0){
                c.memory={{0x3d0f9e4,n%4?fb(std::uniform_real_distribution<float>(1,4096)(rng)):fb(640.0F)},{0x3d0f9e0,u32()}};
                c.arguments={n%6==0?u32():static_cast<std::uint32_t>(static_cast<std::int32_t>(rng()%0x10000)-0x8000)};}
            else if(entry.address==0x781570){
                for(std::uint32_t k=0;k<16;++k)c.memory.push_back({0x3aad0a0+4*k,u32()});c.ecx=0x3aad0a0;if(n>=10)break;}
            cases.push_back(c);}
        std::size_t compared=0;
        for(const auto& c:cases){
            for(auto* g:{original.get(),native.get()})for(const auto& [at,v]:c.memory)g->set_u32(at,v);
            // the argument lists are spelled out (an initializer_list cannot be built at run time)
            GuestCallResult ra,rb;
            switch(c.arguments.size()){
            case 0:ra=original->call(entry.address,{},c.ecx,std::nullopt,c.cw);rb=native->call(entry.address,{},c.ecx,std::nullopt,c.cw);break;
            case 1:ra=original->call(entry.address,{c.arguments[0]},c.ecx,std::nullopt,c.cw);rb=native->call(entry.address,{c.arguments[0]},c.ecx,std::nullopt,c.cw);break;
            default:ra=original->call(entry.address,{c.arguments[0],c.arguments[1]},c.ecx,std::nullopt,c.cw);rb=native->call(entry.address,{c.arguments[0],c.arguments[1]},c.ecx,std::nullopt,c.cw);break;}
            require(bool(ra),entry.name+": original failed: "+ra.error);require(bool(rb),entry.name+": override failed: "+rb.error);
            for(const auto& [at,size]:entry.writes){
                std::vector<std::uint8_t> x(size),y(size);original->read(at,x.data(),size);native->read(at,y.data(),size);
                require(x==y,entry.name+": the override's stores differ from the original's at "+std::to_string(at));}
            ++compared;}
        total+=compared;std::cout<<"  "<<entry.name<<": "<<compared<<" cases equivalent\n";
    }
    std::cout<<"Guest overrides: "<<table.size()<<" frame-loop overrides equivalent to the original bytes on "<<total<<" cases\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
