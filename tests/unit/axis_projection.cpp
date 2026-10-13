#include "player/player_coordinates.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class T>T read(const char* p){T value{};std::ifstream file(p,std::ios::binary);file.read(reinterpret_cast<char*>(&value),sizeof(value));require(file.gcount()==static_cast<std::streamsize>(sizeof(value))&&file.peek()==std::char_traits<char>::eof(),"source artifact size mismatch");return value;}
}
int main(int argc,char** argv){try{
    require(argc==4,"usage: axis-projection-test SOURCE_TABLE SOURCE_UNIT SOURCE_AXIS_CONSTANTS");
    const auto table=read<sadx::NinjaMath::SineTable>(argv[1]);const auto unit=read<sadx::NinjaMatrix>(argv[2]);
    const auto words=read<std::array<std::uint32_t,2>>(argv[3]);require(words[0]==0&&words[1]==0x3f800000,"requires exact source zero/one words");
    sadx::CoordinateAxisConstants c;std::memcpy(&c.zero,&words[0],4);std::memcpy(&c.one,&words[1],4);
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}){
        sadx::X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));
        sadx::NinjaMatrixStack stack({unit,unit,unit},1,0);sadx::PlayerCoordinates coordinates(table,unit,stack);
        auto matrix=unit;coordinates.rotate_axis(matrix,{0,0,1},16384,c,fp);
        require(matrix[1]==1&&matrix[4]==-1&&matrix[10]==1,"source positive-Z quarter turn has established row/angle convention");
        require(matrix[12]==unit[12]&&matrix[13]==unit[13]&&matrix[14]==unit[14]&&matrix[15]==unit[15],"axis body does not overwrite fourth row");
        matrix=unit;coordinates.rotate_axis(matrix,{2,3,4},0,c,fp);
        require(std::fabs(matrix[0])>2,"non-unit axis source second alignment keeps raw X and length, not guessed normalization");
        const sadx::PlayerOrientation task{0,{0,0,0}};
        const auto projected=coordinates.project_input_to_floor(task,{0,1,0},0,0,0,fp.from_float(0.25F),c,fp);
        require(projected&&projected.camera_direction&&projected.projected_direction&&projected.forward_direction&&
            projected.camera_direction->x==0.25F&&projected.camera_direction->y==0.25F&&projected.camera_direction->z==-1&&
            projected.forward_direction->x==1&&projected.forward_direction->y==0&&projected.forward_direction->z==0&&stack.depth()==1,
            "projection stores the caller's real retained ST0 twice and computes actual forward vector");
        sadx::NinjaMatrixStack unknown({unit,unit,unit},1,{});sadx::PlayerCoordinates unknown_coordinates(table,unit,unknown);
        const auto partial=unknown_coordinates.project_input_to_floor(task,{0,1,0},0,0,0,fp.from_float(0),c,fp);
        require(!partial&&partial.source_boundary==0x44b4a8&&partial.coordinate.failure==sadx::CoordinateFailure::unknown_normalization_flag&&
            partial.coordinate.source_boundary==0x784db9&&partial.camera_direction&&!partial.projected_direction&&!partial.forward_direction&&unknown.depth()==2,
            "first projection failure retains camera stores and actual pushed matrix state without cleanup fabrication");
        sadx::NinjaMatrixStack full({unit,unit},2,0);sadx::PlayerCoordinates full_coordinates(table,unit,full);
        const auto capacity=full_coordinates.project_input_to_floor(task,{0,1,0},0,0,0,fp.from_float(0),c,fp);
        require(capacity&&full.depth()==1,"source capacity-no-op push followed by real pop changes prior depth");
    }
    std::cout<<"Axis/projection: source row policy, non-unit axis, retained ST0, shared stack ownership and partial failure passed.\n"
             <<"Complete alternate acceleration/vector-angle and original hardware state remain separate verification scopes.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
