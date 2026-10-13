#include "engine/ninja_sprite.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
using namespace sadx;
namespace {
template<class T>void check(const T& v){if(!static_cast<bool>(v))throw std::runtime_error("sprite assertion");}
template<class T>T read(const char* path){T values{};std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(values.data()),sizeof(values));check(f.gcount()==sizeof(values)&&f.peek()==EOF);return values;}
float value(const std::array<std::uint8_t,112>& bytes,unsigned word){float f;std::memcpy(&f,bytes.data()+word*4,4);return f;}
}
int main(int argc,char** argv){try{
    check(argc==4);const auto bytes=read<std::array<std::uint8_t,40>>(argv[1]);const auto frame=decode_ninja_sprite_frame(bytes.data(),20);const auto constants=decode_ninja_sprite_constants(bytes.data()+20,20);
    check(frame && constants && frame->texture_index==130 && frame->dimensions_center_uv[0]==2 && frame->dimensions_center_uv[2]==1);
    const auto table=read<NinjaMath::SineTable>(argv[2]);const auto unit=read<NinjaMatrix>(argv[3]);
    X87Context fp(0x7f);NinjaSpriteProjection runtime;NinjaSpriteInput input{{3,4,5},2,3,0x1234};
    auto world=ninja_sprite_geometry(input,*frame,0,*constants,runtime,fp);check(world && !world.partial.clipped);
    check(*world.partial.bounds[0]==1 && *world.partial.bounds[1]==1 && *world.partial.bounds[2]==5 && *world.partial.bounds[3]==7);
    check(*world.partial.uv[0]==0 && *world.partial.uv[2]==1 && *world.partial.diffuse_argb==0xffffffff);
    auto vertices=ninja_sprite_vertices(world.partial,{},fp);check(vertices && vertices.command && vertices.command->vertex_stride==24 && vertices.command->vertex_format==0x142 && vertices.written_words==0xffffff);
    check(value(vertices.command->vertex_bytes,0)==1 && value(vertices.command->vertex_bytes,1)==1 && value(vertices.command->vertex_bytes,2)==5);
    auto angle=input;angle.angle=-999;auto ignored=ninja_sprite_geometry(angle,*frame,0,*constants,runtime,fp);check(ignored.partial.bounds==world.partial.bounds); // source body never reads angle
    auto projected=ninja_sprite_geometry(input,*frame,0x31,*constants,runtime,fp);check(projected.failure==NinjaSpriteFailure::missing_coordinates);
    NinjaMatrixStack stack({unit},1,0);NinjaMatrixPrimitives coordinates(table,unit,stack);runtime.coordinates=&coordinates;
    projected=ninja_sprite_geometry(input,*frame,0x31,*constants,runtime,fp);check(projected.failure==NinjaSpriteFailure::missing_projection_scale && projected.partial.camera_position->z==5);
    runtime.scale_at_3d0f9e0=100;runtime.center_x_at_3d0fa0c=10;runtime.center_y_at_3d0fa10=20;
    projected=ninja_sprite_geometry(input,*frame,0x31,*constants,runtime,fp);check(projected && *projected.partial.bounds[0]==30 && *projected.partial.bounds[3]==160);
    check(ninja_sprite_vertices(projected.partial,{},fp).failure==NinjaSpriteFailure::missing_projected_depth);
    auto screen=ninja_sprite_vertices(projected.partial,fp.from_float(.25F),fp);check(screen && screen.command && screen.command->vertex_stride==28 && screen.command->vertex_format==0x144 && screen.written_words==0xfffffff);
    check(value(screen.command->vertex_bytes,0)==30 && value(screen.command->vertex_bytes,1)==40 && value(screen.command->vertex_bytes,2)==.25F && value(screen.command->vertex_bytes,3)==1);
    runtime.color_argb_at_3d0f7f0=std::array<float,4>{1,.5F,.25F,.125F};auto tinted=ninja_sprite_geometry(input,*frame,2|4|8,*constants,runtime,fp);
    check(tinted && tinted.partial.diffuse_argb==0xff7f3f1f && *tinted.partial.uv[0]==1 && *tinted.partial.uv[1]==1 && *tinted.partial.uv[2]==0 && *tinted.partial.uv[3]==0);
    input.position.z=0;auto clipped=ninja_sprite_geometry(input,*frame,0x31,*constants,runtime,fp);check(clipped && clipped.partial.clipped && !ninja_sprite_vertices(clipped.partial,{},fp).command);
    runtime.color_argb_at_3d0f7f0.reset();check(ninja_sprite_geometry(input,*frame,2,*constants,runtime,fp).failure==NinjaSpriteFailure::missing_color);
    NinjaSpriteLocalGeometry unknown;check(ninja_sprite_vertices(unknown,{},fp).failure==NinjaSpriteFailure::missing_geometry);
    std::cout<<"Sprite numeric/vertex slices: actual frame/constants, UV/color/source stores and explicit projection/depth boundaries passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
