#include "player/player_position.hpp"
#include "cpu/crt_integer.hpp"
#include "cpu/x87_transcendental.hpp"
#include <cstring>
namespace sadx {
namespace {
struct SourceFailure { PlayerPositionResult result; };
[[noreturn]] void fail(PlayerPositionFailure failure,std::uint32_t boundary,
                      std::uint32_t callee=0,std::uint32_t nested=0) {
    PlayerPositionResult r;r.failure=failure;r.source_boundary=boundary;
    r.callee_address=callee;r.nested_source_boundary=nested;throw SourceFailure{r};
}
template<typename T> T require(const std::optional<T>& value,PlayerPositionFailure failure,std::uint32_t boundary) {
    if(!value)fail(failure,boundary);return *value;
}
PositionResourceRef reference(const std::optional<PositionResourceRef>& value,std::uint32_t boundary) {
    return require(value,PlayerPositionFailure::unavailable_reference,boundary);
}
template<typename T> T& resource(std::unordered_map<std::size_t,T>& map,PositionResourceRef ref,std::uint32_t boundary) {
    if(!ref.id)fail(PlayerPositionFailure::null_required_reference,boundary);
    const auto found=map.find(*ref.id);if(found==map.end())fail(PlayerPositionFailure::unresolved_resource,boundary);
    return found->second;
}
template<typename T> const T& resource(const std::unordered_map<std::size_t,T>& map,PositionResourceRef ref,std::uint32_t boundary) {
    if(!ref.id)fail(PlayerPositionFailure::null_required_reference,boundary);
    const auto found=map.find(*ref.id);if(found==map.end())fail(PlayerPositionFailure::unresolved_resource,boundary);
    return found->second;
}
struct InfoLocation { PositionResourceRef table;std::int16_t index; };
const PositionCollisionInfo& info(const PlayerPositionCollisionScene& scene,InfoLocation location,std::uint32_t boundary) {
    const auto& table=resource(scene.info_tables,location.table,boundary);
    const auto found=table.records.find(location.index);
    if(found==table.records.end())fail(PlayerPositionFailure::unavailable_collision_info,boundary);
    return found->second;
}
PositionCollisionWork& collision(PlayerPositionCollisionScene& scene,std::uint32_t boundary) {
    return resource(scene.works,reference(scene.task_collision,boundary),boundary);
}
std::uint16_t flags(PlayerPositionCollisionScene& scene,std::uint32_t boundary) {
    return require(collision(scene,boundary).flags,PlayerPositionFailure::unavailable_collision_flags,boundary);
}
InfoLocation selected_info(PlayerPositionCollisionScene& scene,std::uint32_t boundary) {
    auto& own=collision(scene,boundary);
    const auto index=require(own.my_info_index,PlayerPositionFailure::unavailable_collision_index,boundary);
    const auto& hit=resource(scene.works,reference(own.hit_work,boundary),boundary);
    return {reference(hit.info,boundary),index};
}
std::int32_t signed32(std::uint32_t bits) { std::int32_t out;std::memcpy(&out,&bits,4);return out; }
std::int32_t signed16(std::uint32_t bits) { const auto v=bits&65535U;return v<32768U?static_cast<std::int32_t>(v):static_cast<std::int32_t>(v)-65536; }
X87Comparison compare(float a,float b,X87Context& fp) { return fp.compare_float_memory(fp.from_float(a),b); }
bool ordered_at_least(X87Comparison comparison) { return comparison==X87Comparison::equal || comparison==X87Comparison::greater; }
PlayerPositionResult form(const PlayerPositionCollisionScene& scene,std::int32_t index,bool task_present) {
    PlayerPositionResult r;
    if(index<0||index>=16||!task_present) {r.outcome=-1;return r;}
    const auto ref=reference(scene.task_collision,0x41c1d5);
    if(!ref.id){r.outcome=-1;return r;}
    const auto& work=resource(scene.works,ref,0x41c1d8);
    const auto locator=InfoLocation{reference(work.info,0x41c1e0),static_cast<std::int16_t>(index)};
    r.outcome=require(info(scene,locator,0x41c1e9).form,PlayerPositionFailure::unavailable_collision_field,0x41c1e9);
    return r;
}
std::int32_t selected_form(PlayerPositionCollisionScene& scene,std::uint32_t boundary) {
    auto index=require(collision(scene,boundary).my_info_index,PlayerPositionFailure::unavailable_collision_index,boundary);
    auto r=form(scene,index,true);return *r.outcome;
}
bool solid_blocker(PlayerTaskState& task,PlayerPositionCollisionScene& scene,std::uint32_t boundary) {
    if(!(flags(scene,boundary)&0x10)|| (task.orientation.flags&4))return false;
    const auto selected=selected_info(scene,boundary);
    const auto attr=require(info(scene,selected,boundary).attributes,PlayerPositionFailure::unavailable_collision_field,boundary);
    if(attr&0x2000U)return false; // TEST byte[CCL_INFO+5],0x20
    return (flags(scene,boundary)&4)!=0;
}
void coordinate_check(CoordinateResult r,std::uint32_t boundary) {
    if(!r)fail(PlayerPositionFailure::coordinate_failed,boundary,0,r.source_boundary);
}
PlayerCoordinates& matrix_owner(PlayerCoordinates* coordinates,std::uint32_t boundary) {
    if(!coordinates)fail(PlayerPositionFailure::missing_matrix_owner,boundary);
    return *coordinates;
}
void transform_y(PlayerCoordinates* owner,std::int32_t rotation,NinjaVector& vector,X87Context& fp) {
    auto& c=matrix_owner(owner,0x44d0d4);c.begin_unit_frame();
    const auto negative=signed32(0U-static_cast<std::uint32_t>(rotation));
    if(negative)c.rotate_y(c.current_matrix(),negative,fp);
    NinjaVector transformed;
    coordinate_check(c.calculate_vector(c.current_matrix(),vector,transformed,fp),0x44d10f);
    c.end_frame();vector=transformed;
}
void airborne_floor(PlayerTaskState& task,PlayerMotionState* world,PlayerPhysicsState& player,
                    PlayerCoordinates* owner,X87Context& fp) {
    auto& c=matrix_owner(owner,0x44d3c7);c.begin_unit_frame();
    if(task.orientation.angles[2])c.rotate_z(c.current_matrix(),task.orientation.angles[2],fp);
    if(task.orientation.angles[0])c.rotate_x(c.current_matrix(),task.orientation.angles[0],fp);
    const auto negative=signed32(0U-static_cast<std::uint32_t>(task.orientation.angles[1]));
    if(negative)c.rotate_y(c.current_matrix(),negative,fp);
    NinjaVector normal;
    coordinate_check(c.calculate_vector(c.current_matrix(),{0,1,0},normal,fp),0x44d430);
    player.floor_normal=normal;
    NinjaVector displacement;
    coordinate_check(c.calculate_vector(c.current_matrix(),{0,-0.5F,0},displacement,fp),0x44d46f);
    c.end_frame();
    if(!task.position)fail(PlayerPositionFailure::unavailable_task_position,0x44d482);
    task.position->y=fp.to_float(fp.add_float_memory(fp.from_float(displacement.y),task.position->y));
    if(!world)fail(PlayerPositionFailure::null_required_motion,0x44d48e);
    world->motion.speed.y=0;
}
}
std::optional<PlayerPositionConstants> decode_player_position_constants(const std::uint8_t* b,std::size_t n) {
    if(!b||n!=32)return std::nullopt;
    PlayerPositionConstants c;std::uint32_t single[2];std::uint64_t wide[3];
    for(unsigned i=0;i<2;++i){single[i]=0;for(unsigned k=0;k<4;++k)single[i]|=std::uint32_t(b[i*4+k])<<(8*k);}
    for(unsigned i=0;i<3;++i){wide[i]=0;for(unsigned k=0;k<8;++k)wide[i]|=std::uint64_t(b[8+i*8+k])<<(8*k);}
    std::memcpy(&c.speed_cap,&single[0],4);std::memcpy(&c.opposing_dot,&single[1],4);
    std::memcpy(&c.angle_units,&wide[0],8);std::memcpy(&c.positive_factor,&wide[1],8);std::memcpy(&c.negative_factor,&wide[2],8);return c;
}
PlayerPositionResult position_collision_form(const PlayerPositionCollisionScene& scene,std::int32_t index,bool present) {
    try{return form(scene,index,present);}catch(const SourceFailure& failure){return failure.result;}
}
PlayerPositionResult set_player_position(PlayerTaskState& task,PlayerMotionState* world,PlayerPhysicsState& player,
    PlayerParameters& p,PlayerPositionCollisionScene& scene,const PlayerPositionConstants& constants,
    const PlayerRotationConstants& rotation,const PlayerPositionRuntime& runtime,const PlayerPositionCallees& callees,
    PlayerCoordinates* coordinates,X87Context& fp) {
    std::int32_t outcome=0;
    std::shared_ptr<const PlayerReactionResult> reaction_diagnostics;
    std::shared_ptr<const PlayerHoldResult> hold_diagnostics;
    std::shared_ptr<const PlayerScenePositionResult> scene_diagnostics;
    try {
        player.source_vector_at_44=NinjaVector{0,0,0};player.prior_attributes=player.attributes;player.attributes=0;
        const auto zero=rotation.single(0x7dcb2c);
        const auto initial_flags=flags(scene,0x44ce1e);
        bool wall=(initial_flags&0x10)&&!(task.orientation.flags&4)&&(initial_flags&4);
        std::optional<InfoLocation> selected;
        if(wall) {
            selected=selected_info(scene,0x44ce3e);
            const auto push=require(info(scene,*selected,0x44ce56).push,PlayerPositionFailure::unavailable_collision_field,0x44ce56);
            wall=(push&7)!=0;
        }
        if(wall) {
            auto& own=collision(scene,0x44ce64);
            const InfoLocation original{reference(own.info,0x44ce64),0};
            if(!world)fail(PlayerPositionFailure::null_required_motion,0x44ce6c);
            NinjaVector velocity=world->motion.speed;
            if(!callees.normal)fail(PlayerPositionFailure::missing_normal_owner,0x44ce8a,0x41bed0);
            auto normal=callees.normal(task,scene,fp);
            if(normal.failure!=PlayerPositionFailure::none)fail(PlayerPositionFailure::normal_owner_failed,0x44ce8a,0x41bed0,normal.source_boundary);
            if(!normal.normal)fail(PlayerPositionFailure::unavailable_normal_output,0x44ce9b,0x41bed0);
            NinjaVector contact=*normal.normal;player.source_vector_at_44=contact;
            if(fp.compare_float_memory(rotation_vector_length(contact,fp),zero)==X87Comparison::equal)contact={0,0,0};
            else (void)normalize_vector_extended(contact,{rotation.single(0x7dcb2c),rotation.single(0x7dcb10)},fp);
            if(fp.compare_float_memory(rotation_vector_length(velocity,fp),zero)==X87Comparison::equal)velocity={0,0,0};
            else (void)normalize_vector_extended(velocity,{rotation.single(0x7dcb2c),rotation.single(0x7dcb10)},fp);
            float dot=fp.to_float(inner_product_extended(velocity,contact,fp));
            if(ordered_at_least(compare(player.motion.speed.x,constants.speed_cap,fp))) {
                outcome=2;clear_speed(world?&world->motion:nullptr,&player.motion);
            } else if(ordered_at_least(compare(player.motion.speed.x,p.crash_speed,fp)) &&
                      compare(dot,constants.opposing_dot,fp)==X87Comparison::less) {
                outcome=2;world->motion.speed.z=0;world->motion.speed.x=0;
            } else if(ordered_at_least(compare(player.motion.speed.x,p.run_speed,fp)) &&
                      compare(dot,zero,fp)==X87Comparison::less) {
                outcome=1;
                bool turn=selected_form(scene,0x44cfd5)==0;
                if(!turn)turn=selected_form(scene,0x44cfed)==1;
                if(turn) {
                    const auto selected_a=require(info(scene,*selected,0x44d002).a,PlayerPositionFailure::unavailable_collision_field,0x44d002);
                    const auto original_a=require(info(scene,original,0x44d009).a,PlayerPositionFailure::unavailable_collision_field,0x44d009);
                    turn=compare(selected_a,original_a,fp)==X87Comparison::greater;
                    if(turn) {
                        const auto original_damage=require(info(scene,original,0x44d017).damage,PlayerPositionFailure::unavailable_collision_field,0x44d017);
                        const auto selected_damage=require(info(scene,*selected,0x44d01a).damage,PlayerPositionFailure::unavailable_collision_field,0x44d01a);
                        turn=(selected_damage&3)<=((original_damage>>2)&3);
                    }
                }
                if(turn) {
                    dot=fp.to_float(fp.add_float_memory(fp.from_float(require(info(scene,original,0x44d02d).a,PlayerPositionFailure::unavailable_collision_field,0x44d02d)),
                        require(info(scene,*selected,0x44d034).a,PlayerPositionFailure::unavailable_collision_field,0x44d034)));
                    const auto zz=fp.multiply_float_memory(fp.from_float(velocity.z),velocity.z);
                    const auto xx=fp.multiply_float_memory(fp.from_float(velocity.x),velocity.x);
                    const float squared=fp.to_float(fp.add(zz,xx));
                    const auto ratio=fp.divide_float_memory(signed_sqrt_extended(squared,fp),dot);
                    const auto angle=crt_acos(ratio,runtime.crt_math,fp);
                    if(angle.failure!=CrtAsinFailure::none||!angle.value)fail(PlayerPositionFailure::acos_unavailable,0x44d05a,0x6450f0,angle.source_boundary);
                    const auto scaled=fp.multiply_double_memory(*angle.value,constants.angle_units);
                    auto change=crt_ftol_low32(fp.multiply_double_memory(scaled,constants.positive_factor),fp);
                    const auto contact_z=fp.from_float(contact.z),contact_x=fp.from_float(contact.x);
                    const auto contact_angle=X87Transcendental::fpatan_bochs_p5_p6(fp,contact_z,contact_x);
                    const auto c_units=crt_ftol_low32(fp.multiply_double_memory(fp.multiply_double_memory(contact_angle,constants.angle_units),constants.negative_factor),fp);
                    const auto speed_z=fp.from_float(velocity.z),speed_x=fp.from_float(velocity.x);
                    const auto speed_angle=X87Transcendental::fpatan_bochs_p5_p6(fp,speed_z,speed_x);
                    const auto v_units=crt_ftol_low32(fp.multiply_double_memory(fp.multiply_double_memory(speed_angle,constants.angle_units),constants.positive_factor),fp);
                    if(signed16(v_units-(0x8000U-c_units))<0)change=0x10000U-change;
                    auto transformed=world->motion.speed;
                    transform_y(coordinates,signed32(change),transformed,fp);
                    task.orientation.angles[1]=signed32(static_cast<std::uint32_t>(task.orientation.angles[1])+change);
                    world->motion.speed.x=transformed.x;world->motion.speed.z=transformed.z;
                } else if(selected_form(scene,0x44d148)==3 && selected_form(scene,0x44d165)==4) {
                    if(solid_blocker(task,scene,0x44d176)) {world->motion.speed.z=0;world->motion.speed.x=0;}
                    else {
                        if(!callees.reaction)fail(PlayerPositionFailure::missing_reaction_owner,0x44d1bf,0x43c580);
                        const auto r=callees.reaction(task,world,player,scene,fp);
                        reaction_diagnostics=r.reaction_diagnostics;
                        if(r.failure!=PlayerPositionFailure::none)fail(PlayerPositionFailure::reaction_owner_failed,0x44d1bf,0x43c580,r.source_boundary);
                    }
                }
            } else {
                outcome=1;
                if(compare(dot,zero,fp)==X87Comparison::less) {
                    if(!callees.hold)fail(PlayerPositionFailure::missing_hold_owner,0x44d1ea,0x43b9c0);
                    const auto held=callees.hold(task,player,scene,fp);
                    hold_diagnostics=held.hold_diagnostics;
                    if(held.failure!=PlayerPositionFailure::none)fail(PlayerPositionFailure::hold_owner_failed,0x44d1ea,0x43b9c0,held.source_boundary);
                    const auto hold_flags=require(held.flags,PlayerPositionFailure::unavailable_hold_result,0x44d1f2);
                    if(hold_flags&0x400) {
                        world->motion.speed.x=fp.to_float(fp.multiply_float_memory(fp.from_float(contact.x),player.motion.speed.x).negated());
                        world->motion.speed.z=fp.to_float(fp.multiply_float_memory(fp.from_float(contact.z),player.motion.speed.x).negated());
                    } else if(solid_blocker(task,scene,0x44d213)) {
                        velocity.x=fp.to_float(fp.add_float_memory(fp.from_float(velocity.x),contact.x));velocity.y=0;
                        velocity.z=fp.to_float(fp.add_float_memory(fp.from_float(velocity.z),contact.z));
                        if(fp.compare_float_memory(rotation_vector_length(velocity,fp),zero)!=X87Comparison::equal) {
                            (void)normalize_vector_extended(velocity,{rotation.single(0x7dcb2c),rotation.single(0x7dcb10)},fp);
                            // At0x44d291 ESP still includes the pushed vector
                            // argument; [ESP+1c] is normalized X, not local Y.
                            world->motion.speed.x=fp.to_float(fp.multiply_float_memory(fp.from_float(velocity.x),player.motion.speed.x));
                            world->motion.speed.z=fp.to_float(fp.multiply_float_memory(fp.from_float(velocity.z),player.motion.speed.x));
                        } else {world->motion.speed.x=0;world->motion.speed.z=0;}
                    } else {
                        if(!callees.reaction)fail(PlayerPositionFailure::missing_reaction_owner,0x44d1bf,0x43c580);
                        const auto r=callees.reaction(task,world,player,scene,fp);
                        reaction_diagnostics=r.reaction_diagnostics;
                        if(r.failure!=PlayerPositionFailure::none)fail(PlayerPositionFailure::reaction_owner_failed,0x44d1bf,0x43c580,r.source_boundary);
                    }
                }
            }
        } else if(initial_flags&1) {
            auto& own=collision(scene,0x44d2d1);const auto hit_ref=reference(own.hit_work,0x44d2d1);
            if(hit_ref.id) {
                auto& hit=resource(scene.works,hit_ref,0x44d2df);const auto task_ref=reference(hit.owning_task,0x44d2df);
                if(task_ref.id && require(hit.hit_count,PlayerPositionFailure::unavailable_hit_count,0x44d2ed)!=0) {
                    auto& hit_task=resource(scene.tasks,task_ref,0x44d2f3);const auto work_ref=reference(hit_task.work,0x44d2f3);
                    if(work_ref.id) {
                        auto& actor=resource(scene.actors,work_ref,0x44d2fa);
                        if(require(position_actor_task_id(actor),PlayerPositionFailure::unavailable_actor_field,0x44d2fa)==12 && (task.orientation.flags&1) &&
                            compare(player.motion.speed.y,zero,fp)!=X87Comparison::greater) {
                            const auto speed_compare=compare(player.motion.speed.x,p.run_speed,fp);
                            if(speed_compare!=X87Comparison::less || (task.orientation.flags&0x100)) {
                                const auto yaw=require(position_actor_yaw(actor),PlayerPositionFailure::unavailable_actor_field,0x44d334);
                                if(angle_distance(signed32(0x4000U-static_cast<std::uint32_t>(task.orientation.angles[1])),yaw)<0x3000) {
                                    outcome=3;player.motion.speed={0,0,0};if(world)world->motion.speed={0,0,0};
                                }
                            }
                        }
                    }
                }
            }
        }
        const auto act=require(runtime.act,PlayerPositionFailure::unavailable_act,0x44d373);
        const auto stage=require(runtime.stage,PlayerPositionFailure::unavailable_stage,0x44d37c);
        const auto packed=(static_cast<std::uint32_t>(static_cast<std::int32_t>(stage))<<8)|static_cast<std::uint32_t>(static_cast<std::int32_t>(act));
        const bool alternate=packed==0x902||packed==0x903;
        const auto& callback=alternate?callees.alternate_collision:callees.ordinary_collision;
        const auto address=alternate?0x445d10U:0x449500U;
        const bool airborne=(task.orientation.flags&2)!=0;
        const auto boundary=airborne?(alternate?0x44d3a9U:0x44d39fU):(alternate?0x44d4bbU:0x44d4b1U);
        if(!callback)fail(PlayerPositionFailure::missing_collision_owner,boundary,address);
        const auto collision_result=callback(task,world,player,p,scene,coordinates,fp);
        scene_diagnostics=collision_result.scene_diagnostics;
        if(collision_result.failure!=PlayerPositionFailure::none)fail(PlayerPositionFailure::collision_owner_failed,boundary,address,collision_result.source_boundary);
        const auto returned=require(collision_result.outcome,PlayerPositionFailure::unavailable_collision_result,boundary);
        if(!outcome)outcome=returned;
        // Source chooses flag2 branch before collision, then reads flag1 after
        // that mandatory collision owner has had an opportunity to change it.
        if(airborne && !(task.orientation.flags&1))airborne_floor(task,world,player,coordinates,fp);
        PlayerPositionResult result;result.outcome=outcome;result.provisional_outcome=outcome;
        result.hold_diagnostics=std::move(hold_diagnostics);
        result.scene_diagnostics=std::move(scene_diagnostics);
        result.reaction_diagnostics=std::move(reaction_diagnostics);return result;
    }catch(const SourceFailure& failure) {
        auto result=failure.result;result.provisional_outcome=outcome;
        result.hold_diagnostics=std::move(hold_diagnostics);
        result.scene_diagnostics=std::move(scene_diagnostics);
        result.reaction_diagnostics=std::move(reaction_diagnostics);return result;
    }
}
} // namespace sadx
