#include "player/player_heading.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool condition,const char* message) {
    if(!condition) throw std::runtime_error(message);
}
template<class T> static T read(const char* path) {
    T value{};std::ifstream file(path,std::ios::binary);
    file.read(reinterpret_cast<char*>(&value),sizeof(value));
    require(file.gcount()==static_cast<std::streamsize>(sizeof(value)) &&
        file.peek()==std::char_traits<char>::eof(),"source table/matrix missing or truncated");
    return value;
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: player-heading-test SOURCE_QUARTER_TABLE SOURCE_UNIT_MATRIX");
        require(sadx::adjust_angle(65530,5,10)==4,"positive turn across 16-bit wrap");
        require(sadx::adjust_angle(5,65530,10)==65531,"negative turn across 16-bit wrap");
        require(sadx::adjust_angle(0,32768,8192)==57344,"half-turn source tie chooses negative");
        require(sadx::adjust_angle(65536,-65536,0)==0,"source masks incoming angles");
        require(sadx::adjust_angle(12,20,8)==20,"exact step reaches target");
        require(sadx::adjust_angle(12,20,7)==19,"over-step moves only selected bound");
        require(sadx::adjust_angle(0,0,std::numeric_limits<std::int32_t>::min())==0,
            "original signed comparison/NEG wrapping preserved without C++ overflow");
        const auto table=read<sadx::NinjaMath::SineTable>(argv[1]);
        const auto unit=read<sadx::NinjaMatrix>(argv[2]);
        sadx::NinjaMatrixStack stack({unit,unit,unit},1,0);
        sadx::PlayerCoordinates coordinates(table,unit,stack);
        sadx::X87Context fp(0x027f); // Explicit experimental control, no startup claim.
        sadx::NinjaMath math(table,fp);
        sadx::PlayerOrientation task{0,{0,0,0}};
        sadx::PlayerMotion motion{{11,12,13},{1,0,0}};
        const auto turned=sadx::turn_player_heading(task,motion,16384,coordinates,fp);
        require(turned && task.angles[1]==8192 && motion.speed.x==math.cos(8192) &&
            motion.speed.y==0 && motion.speed.z==-math.sin(8192) && stack.depth()==1,
            "real matrix transforms preserve world velocity while changing local yaw");
        require(motion.acceleration.x==11 && motion.acceleration.y==12 && motion.acceleration.z==13,
            "heading source does not overwrite acceleration");
        sadx::NinjaMatrixStack unknown({unit,unit,unit},1,{});
        sadx::PlayerCoordinates unknown_coordinates(table,unit,unknown);
        const auto old_speed=motion.speed;const auto old_yaw=task.angles[1];
        const auto failed=sadx::turn_player_heading(task,motion,0,unknown_coordinates,fp);
        require(!failed && failed.source_boundary==0x443e15 && failed.temporary_vector &&
            failed.coordinate.failure==sadx::CoordinateFailure::unknown_normalization_flag &&
            task.angles[1]==old_yaw && motion.speed.x==old_speed.x && unknown.depth()==2,
            "first transform failure preserves task/speed and real partial matrix state");
        std::cout<<"Source heading: wrapped signed angle policy, real coordinate owners and partial stores passed.\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
