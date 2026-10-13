#include "player/collision_normal.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace sadx;
namespace {
void check(bool c,const char* message){if(!c)throw std::runtime_error(message);}
template<typename T>T read(const char* path){T v{};std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(&v),sizeof(v));check(f.gcount()==sizeof(v)&&f.peek()==std::char_traits<char>::eof(),"source artifact size");return v;}
PlayerPositionCollisionScene scene(){PlayerPositionCollisionScene s;s.task_collision=PositionResourceRef{0};auto& own=s.works[0];own.my_info_index=0;own.info=PositionResourceRef{0};own.hit_work=PositionResourceRef{1};auto& hit=s.works[1];hit.info_count=0;hit.info=PositionResourceRef{1};hit.owning_task=PositionResourceRef{0};s.tasks[0].work=PositionResourceRef{};
    auto& a=s.info_tables[0].records[0];a.attributes=0x20;a.center=NinjaVector{1,2,3};a.a=1;
    auto& b=s.info_tables[1].records[0];b.attributes=0x20;b.center=NinjaVector{0,0,0};b.a=1;b.c=1;b.form=0;b.angle_y=0;return s;}
}
int main(int argc,char** argv){try{
    check(argc==4,"usage: collision-normal-test SINE UNIT AXIS_CONSTANTS");const auto table=read<NinjaMath::SineTable>(argv[1]);const auto unit=read<NinjaMatrix>(argv[2]);
    const auto bytes=read<std::array<std::uint8_t,8>>(argv[3]);CoordinateAxisConstants constants;std::memcpy(&constants.zero,bytes.data(),4);std::memcpy(&constants.one,bytes.data()+4,4);
    for(std::uint16_t pc:{0,0x200,0x300})for(std::uint16_t rc:{0,0x400,0x800,0xc00}) {
        X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));NinjaMatrixStack stack({unit,unit,unit},1,0);PlayerCoordinates c(table,unit,stack);
        PlayerTaskState task{{0,{0,0,0}},0,0,{},NinjaVector{5,0,0}};auto s=scene();CollisionNormalOutput output;CollisionTaskLookup lookup;
        auto r=collision_normal_query(task,s,lookup,constants,nullptr,output,fp);
        check(r&&r.contact==1&&r.output_stores==7&&output.complete()&&*output.x>.26F&&*output.y>.53F&&*output.z>.80F,
              "actual full-vector normal supports index==nbInfo and absolute centers without pose/matrix");
        for(std::uint8_t form:{1,2,4,6}) {
            s=scene();s.info_tables[1].records[0].form=form;s.info_tables[0].records[0].center=NinjaVector{1,200,3};
            r=collision_normal_query(task,s,lookup,constants,nullptr,output,fp);
            check(r&&r.contact==1&&*output.y==0&&*output.x>.31F&&*output.z>.94F,"actual selected forms use XZ difference/normalize, not full-vector guess");
        }
        s=scene();s.info_tables[0].records[0].center=NinjaVector{0,200,0};s.info_tables[1].records[0].form=1;
        r=collision_normal_query(task,s,lookup,constants,nullptr,output,fp);
        check(r&&*output.x==0&&*output.y==0&&*output.z==1,"source zero planar difference selects literal +Z normal");
        s=scene();s.info_tables[1].records[0].form=3;
        r=collision_normal_query(task,s,lookup,constants,&c,output,fp);
        check(r&&r.contact==1&&*output.x==1&&*output.y==0&&*output.z==0&&stack.depth()==1,
              "actual shape3 applies source weighted quadrant/relative yaw/matrix normal");
        s.works[0].hit_work=PositionResourceRef{};s.works[0].my_info_index.reset();output={17,18,19};
        r=collision_normal_query(task,s,lookup,constants,nullptr,output,fp);
        check(r&&r.contact==0&&r.output_stores==0&&*output.x==17&&*output.y==18&&*output.z==19,
              "AL0 no-hit return preserves output and does not require unused index/shape defaults");
        auto binding=bind_collision_normal(lookup,constants,&c);
        auto parent=binding(task,s,fp);check(parent.failure==PlayerPositionFailure::none&&!parent.normal,
                                          "PSetPosition binding must keep untouched unknown caller scratch unknown");
        s=scene();s.info_tables[1].records[0].form=5;output={17,18,19};r=collision_normal_query(task,s,lookup,constants,nullptr,output,fp);
        check(r&&r.contact==0&&r.output_stores==0&&*output.x==17,"unhandled known form returns false after actual own-center calculation");
        s=scene();s.works[0].my_info_index=1;output={17,18,19};r=collision_normal_query(task,s,lookup,constants,nullptr,output,fp);
        check(r&&r.contact==0&&*output.x==17,"source strict greater-than count guard precedes shape resource reads");
        s=scene();s.tasks[0].work=PositionResourceRef{1};s.info_tables[1].records[0].attributes=0;s.info_tables[1].records[0].center=NinjaVector{0,0,0};
        r=collision_normal_query(task,s,lookup,constants,&c,output,fp);
        check(!r&&r.failure==CollisionNormalFailure::shape_unavailable&&r.shape&&r.shape->failure==CollisionShapeFailure::unavailable_task_position&&stack.depth()==2,
              "missing live actor pose leaves actual frame push pending rather than guessed identity");c.end_ex_frame();
        PlayerTaskState other{{0,{0,0,0}},0,0,{},NinjaVector{0,0,0}};
        lookup=[&](PositionResourceRef id)->std::optional<CollisionTaskView>{if(id.id==1)return collision_task_view(&other);return {};};
        r=collision_normal_query(task,s,lookup,constants,&c,output,fp);check(r&&r.contact==1,"real borrowed pose reader binds normal backend");
        other.position=NinjaVector{10,0,0};r=collision_normal_query(task,s,lookup,constants,&c,output,fp);check(r&&*output.x<0,
                                                                                            "pose mutation must be observed live without duplicated cached position");
    }
    std::cout<<"collision normal real dispatcher and borrowed resource boundaries passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
