// engine/ninja_matrix: the matrix stack and primitives the engine's sprite/projection code uses
// directly (the player layer's PlayerCoordinates derives from it). Exactness of each primitive
// against the original bytes is the player-coordinates / projection-angle / scene-mesh-bounds oracles.
#include "engine/ninja_matrix.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

namespace {

void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
template<class T> T read_exact(const char* path) {
    std::ifstream file(path,std::ios::binary);T result;
    file.read(reinterpret_cast<char*>(result.data()),sizeof(result));
    require(file.gcount()==static_cast<std::streamsize>(sizeof(result)) && file.peek()==std::char_traits<char>::eof(),
            "source data byte count differs");
    return result;
}
bool bits(float a,float b) { return std::memcmp(&a,&b,4)==0; }

} // namespace

int main(int argc,char** argv) {
    try {
        require(argc==3,"usage: ninja-matrix-test ninja-trig.f32 ninja-unit.f32");
        const auto table=read_exact<sadx::NinjaMath::SineTable>(argv[1]);
        const auto unit=read_exact<sadx::NinjaMatrix>(argv[2]);
        require(unit[0]==1 && unit[5]==1 && unit[10]==1 && unit[15]==1 && unit[1]==0,"source unit matrix");

        sadx::NinjaMatrixStack stack({unit,unit,unit},1,1);
        sadx::NinjaMatrixPrimitives matrices(table,unit,stack);
        sadx::X87Context fp(0x27f);

        // begin_unit_frame = push-ex + unit copy; end_frame pops back.
        stack.current()[12]=5;
        require(matrices.begin_unit_frame() && stack.depth()==2 && matrices.current_matrix()[12]==0,"unit frame");
        matrices.end_frame();
        require(stack.depth()==1 && matrices.current_matrix()[12]==5,"end frame restores");
        matrices.set_current_unit_matrix();
        require(std::memcmp(matrices.current_matrix().data(),unit.data(),64)==0,"0x7875f0 unit copy");

        // 0x781c10 translate then 0x781670 point: translation row then point = input + translation.
        sadx::NinjaMatrix m=unit;
        matrices.translate_matrix(m,{1,2,3},fp);
        require(m[12]==1 && m[13]==2 && m[14]==3 && m[15]==1,"translate");
        sadx::NinjaVector point{};
        matrices.calculate_point(m,{10,20,30},point,fp);
        require(point.x==11 && point.y==22 && point.z==33,"calculate point");

        // 0x784c70 scales rows 0..2, translation untouched.
        matrices.scale_matrix(m,2,3,4,fp);
        require(m[0]==2 && m[5]==3 && m[10]==4 && m[12]==1,"scale");

        // Quarter-turn about Y uses the source table: sin(0x4000)=table[1024], cos=table[0].
        sadx::NinjaMatrix r=unit;
        matrices.rotate_y(r,0x4000,fp);
        require(r[0]==0 && r[10]==0 && bits(r[8],table[1024]) && r[2]==-table[1024] && r[5]==1,"rotate y"); // cos(0x4000) is the table's signed zero

        // 0x784d00: normalization flag 1 normalizes; an unknown flag is an explicit failure at 0x784db9.
        sadx::NinjaVector v{};
        require(matrices.calculate_vector(unit,{3,0,4},v,fp) && v.y==0 && std::abs(v.x-0.6F)<1e-6F && std::abs(v.z-0.8F)<1e-6F,"normalized vector");
        sadx::NinjaMatrixStack unknown({unit},1,std::nullopt);
        sadx::NinjaMatrixPrimitives unflagged(table,unit,unknown);
        const auto failed=unflagged.calculate_vector(unit,{3,0,4},v,fp);
        require(!failed && failed.failure==sadx::CoordinateFailure::unknown_normalization_flag && failed.source_boundary==0x784db9,"unknown flag");

        require(sadx::ninja_wrapped_negation(1)==0xffff && sadx::ninja_wrapped_negation(0)==0,"angle negation");
        std::cout<<"Ninja matrix stack, unit frames, translate/point/scale/rotate/vector primitives passed.\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
