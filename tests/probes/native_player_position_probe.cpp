#include "player/player_position.hpp"
#include <cstring>
#include <utility>
namespace {
float number(std::uint32_t bits){float out;std::memcpy(&out,&bits,4);return out;}
std::int32_t integer(std::uint32_t bits){std::int32_t out;std::memcpy(&out,&bits,4);return out;}
std::int16_t short_value(std::uint32_t bits){const auto v=bits&65535;return static_cast<std::int16_t>(v<32768?static_cast<int>(v):static_cast<int>(v)-65536);}
std::uint32_t word(float value){std::uint32_t out;std::memcpy(&out,&value,4);return out;}
sadx::NinjaVector vector(const std::uint32_t* w){return {number(w[0]),number(w[1]),number(w[2])};}
void store(const sadx::NinjaVector& v,std::uint32_t* w){w[0]=word(v.x);w[1]=word(v.y);w[2]=word(v.z);}
}
extern "C" int sadx_native_player_position(const std::uint8_t* table,const std::uint8_t* unit,
    const std::uint8_t* rotation,const std::uint8_t* position,const std::uint32_t* w,
    std::uint16_t cw,std::uint32_t* out) {
    if(!table||!unit||!rotation||!position||!w||!out)return -1;
    try {
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);
        sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::PlayerRotationConstants rc;std::memcpy(rc.float_bits.data(),rotation,64);std::memcpy(rc.double_bits.data(),rotation+64,24);
        const auto constants=sadx::decode_player_position_constants(position,32);if(!constants)return -2;
        auto parameters=sadx::decode_player_parameters(reinterpret_cast<const std::uint8_t*>(w+39),132);if(!parameters)return -3;
        std::vector<sadx::NinjaMatrix> frames(6);for(unsigned r=0;r<6;++r)for(unsigned c=0;c<16;++c)frames[r][c]=number(w[74+r*16+c]);
        sadx::NinjaMatrixStack stack(std::move(frames),w[72],w[73]);sadx::PlayerCoordinates coordinates(sine,identity,stack);
        sadx::PlayerTaskState task{{static_cast<std::uint16_t>(w[0]),{integer(w[1]),integer(w[2]),integer(w[3])}},0,0,{},vector(w+4)};
        sadx::PlayerMotionState motion{{vector(w+10),{31,32,33}},1234,41,42,.5F};
        sadx::PlayerPhysicsState player{{{11,12,13},vector(w+7)},1,{21,22,23},sadx::NinjaVector{.1F,.2F,.3F},w[170],0,{}};
        sadx::PlayerPositionCollisionScene scene;scene.task_collision=sadx::PositionResourceRef{0};auto& own=scene.works[0];
        own.flags=static_cast<std::uint16_t>(w[13]);own.my_info_index=short_value(w[14]);own.info=sadx::PositionResourceRef{0};
        own.hit_work=sadx::PositionResourceRef{w[33]?std::optional<std::size_t>{1}:std::nullopt};
        auto& hit=scene.works[1];hit.info=sadx::PositionResourceRef{1};hit.hit_count=short_value(w[38]);
        hit.owning_task=sadx::PositionResourceRef{w[34]?std::optional<std::size_t>{0}:std::nullopt};
        scene.tasks[0].work=sadx::PositionResourceRef{w[35]?std::optional<std::size_t>{0}:std::nullopt};
        scene.actors[0]={static_cast<std::uint8_t>(w[36]),integer(w[37])};
        scene.info_tables[0].records[short_value(w[14])].form=static_cast<std::uint8_t>(w[15]);
        scene.info_tables[0].records[0].damage=static_cast<std::uint8_t>(w[17]);scene.info_tables[0].records[0].a=number(w[20]);
        auto& info=scene.info_tables[1].records[short_value(w[14])];info.push=static_cast<std::uint8_t>(w[16]);
        info.damage=static_cast<std::uint8_t>(w[18]);info.attributes=w[19];info.a=number(w[21]);
        sadx::PlayerPositionRuntime runtime;runtime.stage=short_value(w[31]);runtime.act=short_value(w[32]);runtime.crt_math.matherr_control=1;
        sadx::PlayerPositionCallees callees;
        if(w[26])callees.normal=[&](auto&,auto&,auto&){return sadx::PositionNormalResult{sadx::PlayerPositionFailure::none,vector(w+22),0};};
        if(w[27])callees.hold=[&](auto&,auto&,auto&,auto&){return sadx::PositionHoldResult{sadx::PlayerPositionFailure::none,w[25],0};};
        if(w[28]) {
            auto collision=[&](auto& t,auto*,auto& p,auto&,auto&,auto*,auto&){
                if(w[30]==1)t.orientation.flags|=1;else if(w[30]==2)t.orientation.flags&=static_cast<std::uint16_t>(~1U);
                else if(w[30]==3)t.orientation.flags&=static_cast<std::uint16_t>(~2U);
                p.attributes=0xabcdef;
                return sadx::PositionCollisionResult{sadx::PlayerPositionFailure::none,integer(w[29]),0};};
            callees.ordinary_collision=collision;callees.alternate_collision=collision;
        }
        sadx::X87Context fp(cw);const auto result=sadx::set_player_position(task,&motion,player,*parameters.parameters,scene,*constants,rc,runtime,callees,&coordinates,fp);
        out[0]=task.orientation.flags;for(unsigned i=0;i<3;++i)out[1+i]=static_cast<std::uint32_t>(task.orientation.angles[i]);
        store(*task.position,out+4);store(player.motion.speed,out+7);store(motion.motion.speed,out+10);
        store(*player.source_vector_at_44,out+13);store(*player.floor_normal,out+16);
        out[19]=player.attributes;out[20]=*player.prior_attributes;
        out[21]=result.outcome?static_cast<std::uint32_t>(*result.outcome):0xdeadbeef;
        out[22]=static_cast<std::uint32_t>(result.provisional_outcome);out[23]=result.source_boundary;
        out[24]=stack.depth();out[25]=fp.exception_flags();out[26]=fp.control_word();out[27]=static_cast<std::uint32_t>(result.failure);
        for(unsigned r=0;r<6;++r)for(unsigned c=0;c<16;++c)out[28+r*16+c]=word(stack.slots()[r][c]);return 0;
    }catch(const sadx::X87Error&){return -4;}catch(const sadx::CoordinateError&){return -5;}
}
extern "C" int sadx_native_position_form(std::int32_t index,std::uint32_t task_present,
    std::uint32_t collision_present,std::uint32_t raw_form,std::int32_t* out) {
    sadx::PlayerPositionCollisionScene scene;
    scene.task_collision=sadx::PositionResourceRef{collision_present?std::optional<std::size_t>{0}:std::nullopt};
    scene.works[0].info=sadx::PositionResourceRef{0};
    if(index>=0&&index<16)scene.info_tables[0].records[static_cast<std::int16_t>(index)].form=static_cast<std::uint8_t>(raw_form);
    const auto result=sadx::position_collision_form(scene,index,task_present!=0);
    if(!result||!result.outcome)return -1;*out=*result.outcome;return 0;
}
