#include "player/collision_shapes.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
void check(bool c,const char* message){if(!c)throw std::runtime_error(message);}
template<typename T>T read(const char* path){T v{};std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(&v),sizeof(v));check(f.gcount()==sizeof(v)&&f.peek()==std::char_traits<char>::eof(),"source artifact size");return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
}
int main(int argc,char** argv){try{
    check(argc==3,"usage: collision-shapes-test SINE UNIT");const auto table=read<NinjaMath::SineTable>(argv[1]);const auto unit=read<NinjaMatrix>(argv[2]);
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}) {
        X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));NinjaMatrixStack stack({unit,unit,unit},1,0);PlayerCoordinates c(table,unit,stack);
        PlayerTaskState task{{0,{0,0,0}},0,0,{},NinjaVector{10,20,30}};const auto pose=collision_task_view(&task);
        PositionCollisionInfo shape;shape.attributes=0x20;shape.center=NinjaVector{number(0x7f812345),-0.0F,3};NinjaVector output{41,42,43};
        auto result=collision_shape_center(collision_task_view(nullptr),shape,output,nullptr,fp);
        check(result&&bits(output.x)==0x7f812345&&bits(output.y)==0x80000000&&output.z==3&&fp.exception_flags()==0,
              "absolute centers are raw MOV without task/matrix/default FP semantics");
        shape.attributes=0x8000;shape.center=NinjaVector{1,2,3};
        check(collision_shape_center(pose,shape,output,nullptr,fp)&&output.x==11&&output.y==22&&output.z==33,
              "translation-only path uses owned position and no rotations/matrix");
        shape.attributes=0;check(collision_shape_center(pose,shape,output,&c,fp)&&output.x==11&&output.y==22&&output.z==33&&stack.depth()==1,
                                "regular center uses actual unit/translation/point/popEx");
        task.position.reset();output={41,42,43};result=collision_shape_center(pose,shape,output,&c,fp);
        check(!result&&result.failure==CollisionShapeFailure::unavailable_task_position&&stack.depth()==2&&output.x==41,
              "unknown position preserves prior frame push without invented pose or output");
        c.end_ex_frame();task.position=NinjaVector{10,20,30};
        CollisionTaskView partial=pose;partial.angle=[](unsigned i)->std::optional<std::int32_t>{if(i==0)return {};return 0;};shape.attributes=0x200;
        result=collision_shape_center(partial,shape,output,&c,fp);check(result.failure==CollisionShapeFailure::unavailable_task_angle&&result.source_boundary==0x4192e0&&stack.depth()==2,
              "alternate source rotation order consumes fields sequentially after translation");c.end_ex_frame();
        NinjaMatrix scaled=unit;scaled[12]=50;scaled[13]=60;scaled[14]=70;c.scale_matrix(scaled,2,3,4,fp);
        check(scaled[0]==2&&scaled[5]==3&&scaled[10]==4&&scaled[12]==50&&scaled[13]==60&&scaled[14]==70&&scaled[15]==unit[15],
              "source Scale touches all first-three-row components but not translation");
        shape.attributes=0;
        NinjaMatrixStack full_box({unit,scaled},2,0);PlayerCoordinates box(table,unit,full_box);
        check(collision_shape_box_center(pose,shape,output,&box,fp)&&output.x==72&&output.y==126&&output.z==202&&full_box.depth()==1,
              "supplied-unit push failure preserves current matrix before translation/point/pop");
        NinjaMatrixStack full_regular({unit,scaled},2,0);PlayerCoordinates regular(table,unit,full_regular);
        check(collision_shape_center(pose,shape,output,&regular,fp)&&output.x==11&&output.y==22&&output.z==33&&full_regular.depth()==1,
              "copy-current push failure still gets source unit copy; do not merge frame policies");
        shape.attributes=0x8000;CollisionTaskView no_angles=pose;no_angles.angle={};
        check(collision_shape_box_center(no_angles,shape,output,&c,fp)&&output.x==11&&output.y==22&&output.z==33,
              "box translation-only flag suppresses rotation reads but still owns matrix translation");
        NinjaVector first{5,6,7},second{1,2,3};check(collision_relative_yaw(first,second,0,first,&c,fp)&&first.x==4&&first.y==4&&first.z==4,
                                                "relative-yaw owns f32 spills and permits output/input alias");
        NinjaMatrix arbitrary=scaled;c.translate_matrix(arbitrary,{1,2,3},fp);check(arbitrary[12]==52&&arbitrary[13]==66&&arbitrary[14]==82,
                                                                              "translate applies actual matrix basis");
        NinjaVector point{1,2,3};c.calculate_point(arbitrary,point,point,fp);check(point.x==54&&point.y==72&&point.z==94,
                                                                              "CalcPoint loads inputs before aliased stores and includes translation");
    }
    std::cout<<"collision shapes and source point/translate/scale passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
