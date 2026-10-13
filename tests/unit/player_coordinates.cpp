#include "player/player_coordinates.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

namespace {

void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
bool same(sadx::NinjaVector a,sadx::NinjaVector b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
bool matrix_bits(const sadx::NinjaMatrix& a,const sadx::NinjaMatrix& b) { return std::memcmp(a.data(),b.data(),64)==0; }

template<class T> T read_exact(const char* path) {
    std::ifstream file(path,std::ios::binary);T result;
    file.read(reinterpret_cast<char*>(result.data()),sizeof(result));
    require(file.gcount()==static_cast<std::streamsize>(sizeof(result)) && file.peek()==std::char_traits<char>::eof(),
            "source data byte count differs");
    return result;
}

void test_stack_owner(const sadx::NinjaMatrix& unit) {
    sadx::NinjaMatrixStack stack({unit,unit,unit},1,0);
    require(stack.push_ex() && stack.depth()==2 && matrix_bits(stack.current(),unit),"push copies current to next");
    stack.current()[3]=123;stack.pop_one();
    require(stack.depth()==1 && matrix_bits(stack.current(),unit) && stack.slots()[1][3]==123,
            "pop restores prior current while retaining popped bytes");
    stack.pop_one();require(stack.depth()==1,"pop targetzero is source no-op");
    require(stack.push_ex() && stack.push_ex() && !stack.push_ex() && stack.depth()==3,"source capacity no-op");
    bool rejected=false;
    try { sadx::NinjaMatrixStack invalid({unit},0,0); }
    catch(const sadx::CoordinateError& error) { rejected=error.failure()==sadx::CoordinateFailure::invalid_matrix_stack; }
    require(rejected,"invalid source depth must not become a guest pointer");
    // Collision push(unit) copies only when capacity permits; unlike the
    // transform push-ex/unit-copy sequence it does not overwrite a full slot.
    auto prior=unit;prior[3]=987;
    sadx::NinjaMatrixStack full({prior},1,0);
    require(!full.push_matrix(unit) && matrix_bits(full.current(),prior),"full push supplied matrix retains bytes");
    full.pop_ex();require(full.depth()==0,"pop-ex permits zero depth");
    full.pop_ex();require(full.depth()==0,"pop-ex zero guard preserves state");
    rejected=false;
    try { (void)full.current(); }
    catch(const sadx::CoordinateError& error) { rejected=error.failure()==sadx::CoordinateFailure::invalid_matrix_stack; }
    require(rejected,"zero-depth pointer precedes owned slots; cannot silently select slot0");
    require(full.push_matrix(unit) && full.depth()==1 && matrix_bits(full.current(),unit),
            "explicit supplied push recovers zero-depth without reading before owned storage");
}

void test_task_coordinates(const sadx::NinjaMath::SineTable& table,const sadx::NinjaMatrix& unit) {
    sadx::X87Context fp(0x037f);
    auto prior=unit;prior[0]=7;prior[6]=-3;prior[15]=9;
    sadx::NinjaMatrixStack stack({prior,unit,unit},1,0);
    sadx::PlayerCoordinates coordinates(table,unit,stack);
    sadx::NinjaVector vector{1,0,0};const sadx::PlayerOrientation task{0,{0,16384,0}};
    require(static_cast<bool>(coordinates.task_to_world(&task,vector,fp)) && same(vector,{0,0,1}),"source yaw axis/direction");
    require(stack.depth()==1 && matrix_bits(stack.current(),prior),"normal transform preserves caller matrix owner");
    vector={1,0,0};
    require(static_cast<bool>(coordinates.world_to_player(task,vector,fp)) && same(vector,{0,0,-1}),"inverse source yaw direction");
    auto wrapped=task;wrapped.angles[1]+=65536;vector={1,0,0};
    require(static_cast<bool>(coordinates.task_to_world(&wrapped,vector,fp)) && same(vector,{0,0,1}),"source16-bit angle wrap");
    const auto before=stack.slots();vector={9,8,7};
    require(static_cast<bool>(coordinates.task_to_world(nullptr,vector,fp)) && same(vector,{9,8,7}),"null task is source no-op");
    require(stack.slots()==before && stack.depth()==1,"null task must not push/reset/pop matrix");

    sadx::NinjaMatrixStack full({prior,unit},2,0);sadx::PlayerCoordinates exhausted(table,unit,full);
    vector={1,0,0};require(static_cast<bool>(exhausted.task_to_world(&task,vector,fp)),"capacity source path still computes");
    require(full.depth()==1 && matrix_bits(full.current(),prior),"overflow push no-op followed by real source pop");
    require(!matrix_bits(full.slots()[1],unit),"overflow source path retains overwritten current slot");
}

void test_vector_normalization(const sadx::NinjaMath::SineTable& table,const sadx::NinjaMatrix& unit) {
    sadx::X87Context fp(0x037f);
    sadx::NinjaMatrixStack stack({unit,unit},1,1);sadx::PlayerCoordinates coordinates(table,unit,stack);
    sadx::NinjaVector vector{3,0,4};
    require(static_cast<bool>(coordinates.calculate_vector(unit,vector,vector,fp)),"normalization in-place aliases input safely");
    require(vector.x==0.6F && vector.y==0 && vector.z==0.8F,"real normalization branch uses stored magnitude and reciprocal");
    fp.clear_exceptions();vector={0,0,0};
    require(static_cast<bool>(coordinates.calculate_vector(unit,vector,vector,fp)) &&
            std::isnan(vector.x) && std::isnan(vector.y) && std::isnan(vector.z),
            "zero normalization preserves source invalid arithmetic, no fallback vector");
    require((fp.exception_flags()&5U)==5U,"zero normalization flags divide-zero and invalid");
    sadx::NinjaMatrixStack unknown({unit,unit},1,{});sadx::PlayerCoordinates unresolved(table,unit,unknown);
    vector={1,2,3};auto altered=unit;altered[0]=2;
    const auto result=unresolved.calculate_vector(altered,vector,vector,fp);
    require(result.failure==sadx::CoordinateFailure::unknown_normalization_flag && result.source_boundary==0x784db9 &&
            same(vector,{2,2,3}),"unknown flag is explicit after retaining known matrix stores");
}

void test_floor_boundary(const sadx::NinjaMath::SineTable& table,const sadx::NinjaMatrix& unit) {
    sadx::X87Context fp(0x037f);sadx::NinjaMatrixStack stack({unit,unit},1,0);
    sadx::PlayerCoordinates coordinates(table,unit,stack);sadx::NinjaVector vector{1,2,3};
    require(coordinates.floor_to_world({0,1,0},vector,0,{},fp).failure==sadx::CoordinateFailure::missing_normal_angles &&
            same(vector,{1,2,3}) && stack.depth()==1,"missing normal angles cannot become identity/zero rotation");
    const sadx::NormalAnglesProvider unknown=[](const auto&,auto&) {
        return sadx::NormalAnglesResult{sadx::CoordinateFailure::none,{}};
    };
    require(coordinates.floor_to_world({0,1,0},vector,0,unknown,fp).failure==sadx::CoordinateFailure::normal_angles_unavailable,
            "unknown angle values remain unknown");
    // Explicit test input for the matrix path ONLY, not claimed normal recovery.
    const sadx::NormalAnglesProvider seeded=[](const auto& normal,auto&) {
        require(same(normal,{0,0,1}),"normal source value passes unchanged to owner");
        return sadx::NormalAnglesResult{sadx::CoordinateFailure::none,sadx::NormalAngles{16384,0}};
    };
    vector={0,1,0};require(static_cast<bool>(coordinates.floor_to_world({0,0,1},vector,0,seeded,fp)) &&
                         same(vector,{0,0,1}),"floor matrix path uses recovered X inclination axis");
    // A borrowed binding can observe/mutate the same live vector. The caller's
    // source MOV snapshot already exists before the helper is dispatched.
    const sadx::NormalAnglesProvider observes_live=[&](const auto&,auto&) {
        require(stack.depth()==1,"source input snapshot precedes angle call and stack push");
        vector={9,8,7};
        return sadx::NormalAnglesResult{sadx::CoordinateFailure::none,sadx::NormalAngles{0,0}};
    };
    vector={1,2,3};require(static_cast<bool>(coordinates.floor_to_world({0,1,0},vector,0,observes_live,fp)) &&
                         same(vector,{1,2,3}),"floor conversion uses pre-helper input snapshot");
}

} // namespace

int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: player_coordinates_test SOURCE_QUARTER_TABLE SOURCE_UNIT_MATRIX");
        const auto table=read_exact<sadx::NinjaMath::SineTable>(argv[1]);
        const auto unit=read_exact<sadx::NinjaMatrix>(argv[2]);
        test_stack_owner(unit);test_task_coordinates(table,unit);test_vector_normalization(table,unit);test_floor_boundary(table,unit);
        std::cout<<"Source coordinate/matrix owner: directions, wrap, null task, capacity, prior frames, normalization and explicit floor boundary passed.\n"
                     "General floor angles and actual runtime/hardware state remain unverified.\n";return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
