#include "player/collision_normal.hpp"
#include <cstring>
namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::uint32_t word(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
std::int32_t integer(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
std::int16_t index(std::uint32_t v){v&=65535;return static_cast<std::int16_t>(v<32768?static_cast<int>(v):static_cast<int>(v)-65536);}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
void vector_out(const sadx::NinjaVector& v,std::uint32_t* w){w[0]=word(v.x);w[1]=word(v.y);w[2]=word(v.z);}
}
extern "C" int sadx_native_collision_shapes(const std::uint8_t* table,const std::uint8_t* unit,
    const std::uint8_t* constants,const std::uint32_t* w,std::uint16_t cw,std::uint32_t* out) {
    if(!table||!unit||!constants||!w||!out||w[0]>7||w[135]<1||w[135]>6)return -1;
    try {
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::CoordinateAxisConstants c;std::memcpy(&c.zero,constants,4);std::memcpy(&c.one,constants+4,4);
        std::vector<sadx::NinjaMatrix> frames(w[135]);for(unsigned row=0;row<w[135];++row)for(unsigned col=0;col<16;++col)frames[row][col]=number(w[34+row*16+col]);
        sadx::NinjaMatrixStack stack(std::move(frames),w[32],w[33]);sadx::PlayerCoordinates coordinates(sine,identity,stack);
        sadx::PlayerTaskState task{{static_cast<std::uint16_t>(w[1]),{integer(w[2]),integer(w[3]),integer(w[4])}},0,0,{},vector(w+5)};
        sadx::PlayerTaskState other{{0,{integer(w[133]),integer(w[19]),integer(w[134])}},0,0,{},vector(w+16)};
        sadx::PositionCollisionInfo shape;shape.attributes=w[8];shape.center=vector(w+9);shape.a=number(w[12]);shape.c=number(w[13]);shape.angle_y=integer(w[14]);shape.form=static_cast<std::uint8_t>(w[15]);
        sadx::NinjaVector output=vector(w+130);std::uint32_t failed=0,boundary=0,stores=0,contact=0xdeadbeef;
        sadx::X87Context fp(cw);
        if(w[0]<=3) {
            sadx::CollisionShapeResult r;
            if(w[0]==2)r=sadx::collision_shape_box_center(sadx::collision_task_view(w[26]?&task:nullptr),shape,output,&coordinates,fp);
            else if(w[0]==3)r=sadx::collision_relative_yaw(*task.position,*other.position,integer(w[19]),output,&coordinates,fp);
            else if(w[0]==0)r=sadx::collision_shape_center_419030(sadx::collision_task_view(w[26]?&task:nullptr),shape,output,&coordinates,fp);
            else r=sadx::collision_shape_center(sadx::collision_task_view(w[26]?&task:nullptr),shape,output,&coordinates,fp);
            failed=static_cast<std::uint32_t>(r.failure);boundary=r.source_boundary;
        } else if(w[0]==4) {
            sadx::PlayerPositionCollisionScene scene;scene.task_collision=sadx::PositionResourceRef{0};
            auto& own=scene.works[0];own.my_info_index=index(w[20]);own.info=sadx::PositionResourceRef{w[23]?std::optional<std::size_t>{0}:std::nullopt};own.hit_work=sadx::PositionResourceRef{w[22]?std::optional<std::size_t>{1}:std::nullopt};
            auto& hit=scene.works[1];hit.info=sadx::PositionResourceRef{w[24]?std::optional<std::size_t>{1}:std::nullopt};hit.info_count=static_cast<std::uint16_t>(w[21]);hit.owning_task=sadx::PositionResourceRef{w[25]?std::optional<std::size_t>{0}:std::nullopt};
            scene.tasks[0].work=sadx::PositionResourceRef{w[26]?std::optional<std::size_t>{0}:std::nullopt};
            auto& own_shape=scene.info_tables[0].records[0];own_shape.attributes=w[27];own_shape.center=vector(w+28);own_shape.a=number(w[31]);
            scene.info_tables[1].records[index(w[20])]=shape;
            sadx::CollisionTaskLookup lookup=[&](sadx::PositionResourceRef id)->std::optional<sadx::CollisionTaskView>{if(id.id==0)return sadx::collision_task_view(&other);return {};};
            sadx::CollisionNormalOutput normal{output.x,output.y,output.z};
            const auto r=sadx::collision_normal_query(task,scene,lookup,c,&coordinates,normal,fp);
            failed=static_cast<std::uint32_t>(r.failure);boundary=r.source_boundary;stores=r.output_stores;
            if(r.contact)contact=static_cast<std::uint32_t>(*r.contact);output=*normal.complete();
        } else if(w[0]==5)coordinates.translate_matrix(coordinates.current_matrix(),*task.position,fp);
        else if(w[0]==6)coordinates.calculate_point(coordinates.current_matrix(),*task.position,output,fp);
        else coordinates.scale_matrix(coordinates.current_matrix(),task.position->x,task.position->y,task.position->z,fp);
        vector_out(output,out);out[3]=contact;out[4]=failed;out[5]=boundary;out[6]=stores;out[7]=stack.depth();out[8]=fp.exception_flags();out[9]=fp.control_word();
        for(unsigned row=0;row<6;++row)for(unsigned col=0;col<16;++col)out[10+row*16+col]=row<stack.slots().size()?word(stack.slots()[row][col]):w[34+row*16+col];return 0;
    }catch(const sadx::X87Error&){return -2;}catch(const sadx::CoordinateError&){return -3;}
}
