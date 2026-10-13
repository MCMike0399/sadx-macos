#include "player/player_path.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}template<class T>T read(const char* p){T v{};std::ifstream f(p,std::ios::binary);f.read(reinterpret_cast<char*>(&v),sizeof(v));require(f.gcount()==static_cast<std::streamsize>(sizeof(v))&&f.peek()==std::char_traits<char>::eof(),"source artifact size mismatch");return v;}}
int main(int argc,char** argv){try{
    require(argc==3,"usage: player-path-test SOURCE_TABLE SOURCE_PATH_CONSTANTS");
    const auto table=read<sadx::NinjaMath::SineTable>(argv[1]);const auto raw=read<std::array<std::uint8_t,92>>(argv[2]);
    const auto c=sadx::decode_player_path_constants(raw.data(),raw.size());require(c.has_value(),"source path constants invalid");
    sadx::PlayerPathResource path{0,3,12,{{0,0,4,{0,0,0}},{2000,1000,4,{4,0,0}},{4000,2000,4,{8,0,0}},{6000,3000,4,{12,0,0}}}};
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        sadx::X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));sadx::NinjaMath math(table,fp);
        sadx::SseContext sse(0x1f80);sadx::CrtFloorRuntime runtime;runtime.cpu_sse2=1;runtime.sse=&sse;
        sadx::PlayerPathStatus status{2};path.type=0;
        const auto sampled=sadx::query_player_path(path,status,c->query,c->floor,runtime,math,fp);
        require(sampled&&sampled.source_return==true&&sampled.selected_index==0&&status.distance==2&&
            status.position_at20[0]&&*status.position_at20[0]>1.99F&&*status.position_at20[0]<2.01F&&
            status.tangent_at44[0]==1&&status.tangent_at44[1]==0&&status.tangent_at44[2]==0,
            "real path interpolation/source tangent stores without substituted callback");
        status.distance=-1;status.angle0=123;status.position_at20[0]=456;
        sadx::CrtFloorRuntime unknown;
        const auto outside=sadx::query_player_path(path,status,c->query,c->floor,unknown,math,fp);
        require(outside&&outside.source_return==false&&status.angle0==123&&status.position_at20[0]==456,
            "source open-range false retains preexisting outputs and never consults unavailable floor");
        path.type=1;status.distance=-1;
        const auto looped=sadx::query_player_path(path,status,c->query,c->floor,runtime,math,fp);
        require(looped&&looped.source_return==true&&status.distance==11&&looped.selected_index==2,
            "real CRT floor wraps negative loop distance before source point selection");
        status.distance=-1;const auto failed=sadx::query_player_path(path,status,c->query,c->floor,unknown,math,fp);
        require(!failed&&failed.failure==sadx::PathQueryFailure::floor_unavailable&&failed.source_boundary==0x4393e6&&
            failed.floor&&failed.floor->failure==sadx::CrtFloorFailure::unknown_cpu_policy&&failed.floor_argument&&status.distance==-1,
            "unknown floor policy retains exact stored argument and prior query distance");
    }
    const auto segment=sadx::path_segment_endpoints(path,0);require(segment.endpoints&&segment.endpoints->begin_angles[0]==2000,
        "loop midpoint uses final logical point, not physical sentinel");
    auto one=path;one.point_count=1;one.points.resize(1);
    const auto neighbor=sadx::path_segment_endpoints(one,0);require(neighbor.failure==sadx::PathQueryFailure::point_outside_storage&&neighbor.unavailable_index==1,
        "source first-index branch skips next clamp; native ownership rejects missing physical neighbor");
    std::cout<<"Path query: real source interpolation/CRT wrap, optional facts, sentinel ownership and failure preservation passed.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
