#include "player/player_projection.hpp"
#include <cstring>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace{void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}}
int main(int argc,char** argv){try{
    require(argc==2,"usage: player-projection-test SOURCE_PROJECTION_CONSTANTS");
    std::array<std::uint8_t,28> bytes{};std::ifstream file(argv[1],std::ios::binary);file.read(reinterpret_cast<char*>(bytes.data()),28);
    require(file.gcount()==28&&file.peek()==std::char_traits<char>::eof(),"requires exact28-byte source constants");
    std::uint64_t digest=0xcbf29ce484222325ULL;for(auto b:bytes)digest=(digest^b)*0x100000001b3ULL;
    require(digest==0x6584ca27c26f97beULL,"source angle-constant digest mismatch");
    sadx::CoordinateAxisConstants axis;std::memcpy(&axis.zero,bytes.data(),4);std::memcpy(&axis.one,bytes.data()+4,4);
    sadx::ProjectionAngleConstants c;std::memcpy(&c.negative_one,bytes.data()+8,4);std::memcpy(&c.first_factor,bytes.data()+12,8);std::memcpy(&c.second_factor,bytes.data()+20,8);
    require(axis.zero==0&&axis.one==1&&c.negative_one==-1,"source comparison constants retained");
    const sadx::NinjaVector x{1,0,0},y{0,1,0},z{0,0,1},negative_z{0,0,-1};sadx::CrtAsinRuntime runtime;runtime.matherr_control=1;
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        const auto cw=static_cast<std::uint16_t>(pc|rc|0x7f);sadx::X87Context fp(cw);
        const auto positive=sadx::projection_vector_angle(x,y,&z,axis,c,runtime,fp);
        const auto negative=sadx::projection_vector_angle(x,y,&negative_z,axis,c,runtime,fp);
        require(positive&&negative&&positive.cross&&positive.cross->x==0&&positive.cross->y==0&&positive.cross->z==1&&
            static_cast<std::uint32_t>(*positive.angle)+static_cast<std::uint32_t>(*negative.angle)==65536&&fp.control_word()==cw,
            "source canonical cross orientation, complement sign and CRT control restoration");
        const auto no_side=sadx::projection_vector_angle(x,y,nullptr,axis,c,runtime,fp);
        require(no_side&&no_side.angle==positive.angle,"actual null sign direction skips only final orientation dot");
        sadx::CrtAsinRuntime unknown;
        const auto lower=sadx::projection_vector_angle(x,{-1,0,0},&z,axis,c,unknown,fp);
        const auto upper=sadx::projection_vector_angle(x,x,&z,axis,c,unknown,fp);
        require(lower&&lower.angle==0x8000&&!lower.cross&&upper&&upper.angle==0&&!upper.cross,
            "dot endpoint branches return before unknown CRT owner or cross calculation");
        const auto failed=sadx::projection_vector_angle(x,y,&z,axis,c,unknown,fp);
        require(!failed&&failed.failure==sadx::ProjectionAngleFailure::acos_unavailable&&
            failed.crt_failure==sadx::CrtAsinFailure::unknown_matherr_policy&&failed.source_boundary==0x438460&&
            failed.nested_source_boundary==0x645138&&failed.stored_dot==0&&!failed.cross&&
            failed.pending_value&&!failed.pending_stored_result&&!failed.angle,
            "missing CRT policy retains actual extended result/dot before a binary64 dispatcher store");
    }
    sadx::X87Context fp(0x027f);auto aliased=x;
    const auto length=sadx::cross_product_length_extended(aliased,y,aliased,fp);
    require(aliased.x==0&&aliased.y==0&&aliased.z==1&&fp.to_float(length)==1,
            "canonical cross reads every input before stores and supports output/input alias");
    aliased=y;const auto second_length=sadx::cross_product_length_extended(x,aliased,aliased,fp);
    require(aliased.x==0&&aliased.y==0&&aliased.z==1&&fp.to_float(second_length)==1,
            "canonical cross also preserves second-input alias before source stores");
    sadx::CrtAsinRuntime dispatcher_missing;dispatcher_missing.matherr_control=0;
    const auto pending=sadx::projection_vector_angle({NAN,0,0},x,&z,axis,c,dispatcher_missing,fp);
    require(!pending&&pending.crt_failure==sadx::CrtAsinFailure::missing_matherr_handler&&
        pending.source_boundary==0x438460&&pending.nested_source_boundary==0x648b6b&&
        pending.pending_stored_result&&std::isnan(*pending.pending_stored_result)&&!pending.pending_value&&!pending.cross,
        "unavailable dispatcher preserves actual binary64 FSTP result, distinct from unstored extended value");
    std::cout<<"Signed vector-angle: real CRT owner, clamps, optional direction, canonical cross/alias and partial CRT failures passed.\n"
             <<"Original hardware exceptions and complete gameplay remain unverified.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
