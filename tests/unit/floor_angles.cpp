#include "player/floor_angles.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
float source_float(std::uint32_t word) { float value;std::memcpy(&value,&word,4);return value; }
double source_double(std::uint64_t word) { double value;std::memcpy(&value,&word,8);return value; }
sadx::FloorAngleConstants constants() {
    // Exact source bits at0x7e66d8/0x7e66d4/0x7e00e0 and
    //0x7dccd0/0x7dccc8/0x7dcce8, not recomputed pi/trig constants.
    return {source_float(0x3f7ffd61),source_float(0xbf7ffd61),source_float(0x3b03126f),
            source_double(0x40f0000000000000ULL),source_double(0x3fc45f30b4e4e30aULL),
            source_double(0xbfc45f30b4e4e30aULL)};
}
template<class T>T read_source(const char* path) {
    T result;std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(result.data()),sizeof(result));
    require(file.gcount()==static_cast<std::streamsize>(sizeof(result)) && file.peek()==std::char_traits<char>::eof(),"source table/matrix byte count");
    return result;
}

void test_exact_compare_branches() {
    sadx::X87Context fp(0x037f);const auto c=constants();const sadx::CrtAsinRuntime unknown{{},{}};
    for(const auto normal:{sadx::NinjaVector{0,1,1},{0,1,-1},{0,0,0.5F},{0,0,-0.5F}}) {
        const auto result=sadx::floor_normal_angles(normal,c,unknown,fp);
        require(result.failure==sadx::CrtAsinFailure::none && result.angles && result.angles->roll==0 &&
                result.angles->inclination==(normal.z>0 ? 0x4000 : 0xc000),
                "source pole/near-axis branches bypass CRT without normalizing input");
    }
    for(float z:{c.upper_cut,c.lower_cut}) {
        const auto result=sadx::floor_normal_angles({0,1,z},c,unknown,fp);
        require(result.failure==sadx::CrtAsinFailure::unknown_matherr_policy && !result.angles,
                "exact cutoff equality remains general source path");
    }
    const auto above=sadx::floor_normal_angles({0,1,std::nextafter(c.upper_cut,1.0F)},c,unknown,fp);
    require(above.angles && above.angles->inclination==0x4000,"strict greater cutoff next sourcefloat");
    const auto epsilon=sadx::floor_normal_angles({0,c.axis_epsilon,0.5F},c,unknown,fp);
    require(epsilon.failure==sadx::CrtAsinFailure::unknown_matherr_policy,"exact epsilon equality must not use near-axis shortcut");
    const auto inside=sadx::floor_normal_angles({0,std::nextafter(c.axis_epsilon,0.0F),0.5F},c,unknown,fp);
    require(inside.angles && inside.angles->inclination==0x4000,"strict epsilon below boundary");
}

void test_general_owner_and_partial_store() {
    const auto c=constants();const sadx::CrtAsinRuntime bypass{1,{}};
    for(std::uint16_t pc:{0x0000,0x0200,0x0300}) for(std::uint16_t rc:{0,0x400,0x800,0xc00}) {
        const auto word=static_cast<std::uint16_t>(pc|rc|0x7f);sadx::X87Context fp(word);
        const auto up=sadx::floor_normal_angles({0,1,0},c,bypass,fp);
        require(up.angles && up.angles->inclination==0 && up.angles->roll==0 && fp.control_word()==word,
                "actual general angle owner closes ordinary up-floor path");
        const auto slope=sadx::floor_normal_angles({0,1,0.5F},c,bypass,fp);
        require(slope.angles && slope.angles->inclination==5461 && slope.angles->roll==0 && fp.control_word()==word,
                "source-scaled/FIST-corrected inclination under all selected controls");
    }
    sadx::X87Context unmasked(0x037e);bool retained=false;
    try { (void)sadx::floor_normal_angles({std::numeric_limits<float>::quiet_NaN(),1,0},c,bypass,unmasked); }
    catch(const sadx::FloorAngleArithmeticError& error) {
        retained=error.failure()==sadx::X87Failure::unmasked_exception && error.stored_inclination()==0 &&
                 error.source_boundary()==0x4b8f48 && error.exceptions()==1;
    }
    require(retained,"roll FISTP failure retains prior source inclination store without inventing roll");
}

void test_real_floor_coordinate_integration(const sadx::NinjaMath::SineTable& table,const sadx::NinjaMatrix& unit) {
    const auto c=constants();const sadx::CrtAsinRuntime bypass{1,{}};
    sadx::X87Context fp(0x037f);auto prior=unit;prior[0]=7;prior[15]=9;
    sadx::NinjaMatrixStack stack({prior,unit,unit},1,0);sadx::PlayerCoordinates coordinates(table,unit,stack);
    const sadx::NormalAnglesProvider real_owner=[&](const auto& normal,auto& context) {
        const auto result=sadx::floor_normal_angles(normal,c,bypass,context);
        return sadx::NormalAnglesResult{result.failure==sadx::CrtAsinFailure::none ?
            sadx::CoordinateFailure::none : sadx::CoordinateFailure::normal_angles_unavailable,result.angles};
    };
    sadx::NinjaVector vector{1,0,0};
    require(static_cast<bool>(coordinates.floor_to_world({0,1,0},vector,16384,real_owner,fp)) &&
            vector.x==0 && vector.y==0 && vector.z==1,"full real floor-angle to matrix path, yaw direction");
    vector={0,1,0};
    require(static_cast<bool>(coordinates.floor_to_world({0,1,0.5F},vector,0,real_owner,fp)),"full real slope path");
    sadx::NinjaMath math(table,fp);
    require(vector.x==0 && vector.y==math.cos(5461) && vector.z==math.sin(5461),
            "real angle owner feeds actual Ninja table/matrix rotation");
    require(stack.depth()==1 && std::memcmp(stack.current().data(),prior.data(),64)==0,
            "full floor integration restores original active matrix frame");
    const sadx::NormalAnglesProvider missing_dispatch=[&](const auto& normal,auto& context) {
        const auto result=sadx::floor_normal_angles(normal,c,{0,{}},context);
        return sadx::NormalAnglesResult{result.failure==sadx::CrtAsinFailure::none ?
            sadx::CoordinateFailure::none : sadx::CoordinateFailure::normal_angles_unavailable,result.angles};
    };
    vector={1,2,3};fp.clear_exceptions();
    const auto failed=coordinates.floor_to_world({0,1,0.5F},vector,0,missing_dispatch,fp);
    require(failed.failure==sadx::CoordinateFailure::normal_angles_unavailable && stack.depth()==1 &&
            vector.x==1 && vector.y==2 && vector.z==3,"unresolved real dispatcher does not become identity transform success");
}
}

int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: floor_angles_test SOURCE_QUARTER_TABLE SOURCE_UNIT_MATRIX");
        test_exact_compare_branches();test_general_owner_and_partial_store();
        test_real_floor_coordinate_integration(read_source<sadx::NinjaMath::SineTable>(argv[1]),read_source<sadx::NinjaMatrix>(argv[2]));
        std::cout<<"Floor angles: exact source thresholds, actual numeric/CRT owner, partial stores and real floor-coordinate integration passed.\n"
                     "Runtime policy is explicitly selected; hardware FPATAN/real error dispatcher remain unverified.\n";return 0;
    }catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
