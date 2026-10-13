#include "player/player_contact_response.hpp"
#include "player/player_rotation.hpp"
#include "player/player_acceleration_alternate_math.hpp"
#include <cstring>
namespace sadx {
std::optional<PlayerContactResponseConstants> decode_contact_response_constants(const std::uint8_t* b,std::size_t n){
    if(!b||n!=52)return {};float f[7];double d[3];std::memcpy(f,b,28);std::memcpy(d,b+28,24);
    const CoordinateAxisConstants axis{f[0],f[1]};
    return PlayerContactResponseConstants{{f[5],d[0],d[1]},{axis,f[6],d[2]},f[2],f[3],f[4]};
}
namespace {
struct Stopped{};
struct Sites {std::uint32_t count,point_z,center_z,point_y,center_y,point_x,center_x,
    angle,gravity,displacement,normalized_dot,loop_end,final_length,final_distance;};
constexpr Sites sites[]{
    {0x43cce9,0x43cd20,0x43cd24,0x43cd27,0x43cd2a,0x43cd2d,0x43cd30,0x43cd6b,0x43cd85,0x43cda9,0x43cdeb,0x43ceed,0x43cf14,0x43cf19},
    {0x43cf99,0x43cfc0,0x43cfc4,0x43cfc7,0x43cfca,0x43cfcd,0x43cfd0,0x43d008,0x43d026,0x43d04a,0x43d094,0x43d0dc,0x43d103,0x43d108},
    {0x43d189,0x43d1b1,0x43d1b5,0x43d1b8,0x43d1bb,0x43d1be,0x43d1c1,0x43d1f6,0x43d210,0x43d230,0x43d272,0x43d2ba,0x43d2e1,0x43d2e6},
    {0x43d8b9,0x43d8e0,0x43d8e4,0x43d8e7,0x43d8e9,0x43d8ec,0x43d8ef,0x43d930,0,0x43d958,0x43d9a8,0x43da30,0x43da4b,0x43da54},
    {0x43d370,0x43d398,0x43d39c,0x43d39f,0x43d3a2,0x43d3a5,0x43d3a8,0x43d3e6,0x43d401,0x43d435,0x43d477,0x43d508,0x43d523,0x43d528}};
float from_bits(std::uint32_t u){float f;std::memcpy(&f,&u,4);return f;}
std::int32_t signed_bits(std::uint32_t u){std::int32_t x;std::memcpy(&x,&u,4);return x;}
}
PlayerContactResponseResult respond_player_contacts(PlayerContactResponseEntry entry,
        PlayerContactWork& contact,const PlayerMeshQueryState& state,const PlayerContactResponseActors& actors,
        const PlayerContactResponseConstants& c,const PlayerContactResponseRuntime& runtime,X87Context& fp){
    PlayerContactResponseResult r;const auto kind=static_cast<unsigned>(entry);
    if(kind>=5){r.failure=PlayerContactResponseFailure::invalid_entry;return r;}
    const auto& s=sites[kind];
    const bool ceiling=entry==PlayerContactResponseEntry::at_43d180;
    const bool reflection=entry==PlayerContactResponseEntry::at_43d8b0;
    const bool movement=entry==PlayerContactResponseEntry::at_43d360;
    const bool ordinary=entry==PlayerContactResponseEntry::at_43cce0;
    auto stop=[&](PlayerContactResponseFailure f,std::uint32_t at)->void{r.failure=f;r.source_boundary=at;throw Stopped{};};
    auto field=[&](const auto& value,std::uint32_t at)->const auto&{
        if(!value)stop(PlayerContactResponseFailure::unavailable_contact_field,at);return *value;};
    auto payload=[&](std::size_t i,std::uint32_t at)->const MeshContactPayload&{
        if(i>=state.responses.size()||!state.responses[i])stop(PlayerContactResponseFailure::unavailable_response,at);
        return *state.responses[i];};
    auto motion=[&](std::uint32_t at)->PlayerMotionState&{if(!actors.world)stop(PlayerContactResponseFailure::null_motion,at);return *actors.world;};
    auto player=[&](std::uint32_t at)->PlayerPhysicsState&{if(!actors.player)stop(PlayerContactResponseFailure::null_player,at);return *actors.player;};
    auto params=[&](std::uint32_t at)->const PlayerParameters&{if(!actors.parameters)stop(PlayerContactResponseFailure::missing_parameters,at);return *actors.parameters;};
    auto attributes=[&](std::uint32_t at){if(!actors.attributes)stop(PlayerContactResponseFailure::unavailable_attributes,at);return *actors.attributes;};
    auto gravity=[&](std::uint32_t at)->const NinjaVector&{if(!runtime.world_gravity)stop(PlayerContactResponseFailure::unavailable_gravity,at);return *runtime.world_gravity;};
    auto cmp=[&](X87Value v,float m){return fp.compare_float_memory(v,m);};
    auto cf=[&](float a,float b){return cmp(fp.from_float(a),b);};
    auto angle=[&](const NinjaVector& a,const NinjaVector& b,std::uint32_t at){
        auto result=projection_vector_angle(a,b,nullptr,c.plane.axis,c.angle,runtime.inverse_math,fp);
        if(!result){r.projection=result;r.nested_source_boundary=result.source_boundary;stop(PlayerContactResponseFailure::projection_failed,at);}
        return *result.angle;};
    auto negative_cos=[&](std::uint32_t at){if(!runtime.sine_table)stop(PlayerContactResponseFailure::missing_sine_table,at);
        NinjaMath math(*runtime.sine_table,fp);return math.cos_extended(0x600).negated();};
    auto nearest=[&](float distance,std::size_t i){++r.candidates;r.classifications[i]=ceiling?2U:8U;
        if(cf(r.closest,distance)==X87Comparison::greater){r.closest=distance;r.selected=static_cast<std::int32_t>(i);}};
    auto selected_distance=[&](std::uint32_t at){
        if(r.selected>=0)return *r.distances[static_cast<std::size_t>(r.selected)];
        if(!movement && r.classifications[15])return from_bits(*r.classifications[15]);
        if(!runtime.unselected_distance_word)stop(PlayerContactResponseFailure::unavailable_stack_word,at);
        return from_bits(*runtime.unselected_distance_word);};
    auto clip=[&](float distance){
        auto& displacement=contact.displacement_at_10.value();
        if(cmp(rotation_vector_length(displacement,fp),c.plane.axis.zero)==X87Comparison::equal){displacement={0,0,0};return;}
        (void)normalize_vector_extended(displacement,c.plane.axis,fp);
        // Actual tails alternate FLD/m32 order, including X of43d180.
        displacement.x=fp.to_float(ceiling?fp.multiply_float_memory(fp.from_float(distance),displacement.x):fp.multiply_float_memory(fp.from_float(displacement.x),distance));
        displacement.y=fp.to_float(fp.multiply_float_memory(fp.from_float(distance),displacement.y));
        displacement.z=fp.to_float(ceiling?fp.multiply_float_memory(fp.from_float(distance),displacement.z):fp.multiply_float_memory(fp.from_float(displacement.z),distance));
    };
    try{
        const auto initial_count=field(contact.contact_count_at_28,s.count);
        r.selection_scratch_initialized=true;
        if(ordinary||movement)r.normal_sum=NinjaVector{0,0,0};
        if(initial_count==0){r.outcome=0;return r;}
        for(std::size_t i=0;;++i){
            const auto z=fp.from_float(payload(i,s.point_z).point_at_2c.z);
            const auto dz=fp.subtract_float_memory(z,field(contact.center_at_4,s.center_z).z);
            const auto y=fp.from_float(payload(i,s.point_y).point_at_2c.y);
            const auto dy=fp.subtract_float_memory(y,field(contact.center_at_4,s.center_y).y);
            const auto x=fp.from_float(payload(i,s.point_x).point_at_2c.x);
            const auto dx=fp.subtract_float_memory(x,field(contact.center_at_4,s.center_x).x);
            auto square=fp.multiply(dx,dx);
            if(reflection){
                const auto zz=fp.multiply(dz,dz);square=fp.add(square,zz);
                const auto yy=fp.multiply(dy,dy);square=fp.add(square,yy);
            }else{
                const auto yy=fp.multiply(dy,dy);square=fp.add(square,yy);
                const auto zz=fp.multiply(dz,dz);square=fp.add(square,zz);
            }
            const float stored_square=fp.to_float(square);
            const auto root=signed_sqrt_extended(stored_square,fp);
            const float distance=fp.to_float(root); // FST, then FSTP again
            r.distances[i]=fp.to_float(root);r.classifications[i]=0;
            const auto a=angle(field(contact.direction_at_1c,s.angle),payload(i,s.angle).oriented_normal,s.angle);
            bool floor=a<=0x2c00;
            if(!floor){
                if(reflection){
                    floor=cf(field(contact.direction_at_1c,0x43d947).y,payload(i,0x43d94a).oriented_normal.y)==X87Comparison::less;
                }else{
                    const auto first=fp.to_float(inner_product_extended(gravity(s.gravity),field(contact.direction_at_1c,s.gravity),fp));
                    const auto second=inner_product_extended(gravity(s.gravity),payload(i,s.gravity).oriented_normal,fp);
                    floor=cmp(second,first)==X87Comparison::less;
                }
            }
            if(!floor && movement && (attributes(0x43d425)&0x4000U))floor=true;
            if(floor){r.classifications[i]=2;if(ceiling)nearest(distance,i);}
            else{
                NinjaVector displacement=field(contact.displacement_at_10,s.displacement);
                if(cmp(rotation_vector_length(displacement,fp),c.plane.axis.zero)!=X87Comparison::equal)
                    (void)normalize_vector_extended(displacement,c.plane.axis,fp);
                if((entry==PlayerContactResponseEntry::at_43cf90 || reflection)&&a>=0x7000){}
                else{
                    const auto dot=inner_product_extended(displacement,payload(i,s.normalized_dot).oriented_normal,fp);
                    const float stored_dot=(ordinary||reflection||movement)?fp.to_float(dot):0;
                    const auto comparison=cmp(dot,ceiling?c.floor_dot:c.plane.axis.zero);
                    if(comparison==X87Comparison::less){
                        bool eligible=true;
                        if(reflection||movement){
                            const auto raw=fp.to_float(inner_product_extended(field(contact.displacement_at_10,reflection?0x43d9c3:0x43d492),payload(i,s.normalized_dot).oriented_normal,fp));
                            const auto threshold=negative_cos(reflection?0x43d9d1:0x43d4a0);
                            if(cmp(threshold,stored_dot)!=X87Comparison::greater)
                                eligible=cf(raw,c.hard_impact)==X87Comparison::less;
                        }
                        if(eligible){if(!ceiling)nearest(distance,i);else r.classifications[i]=8;}
                        if(ordinary){
                            if(!(attributes(0x43ce31)&0x800000U)){
                                const auto threshold=negative_cos(0x43ce47);
                                if(cmp(threshold,stored_dot)==X87Comparison::greater)motion(0x43ce62).motion.speed={0,0,0};
                                if(cmp(inner_product_extended(field(contact.displacement_at_10,0x43ce70),payload(i,0x43ce70).oriented_normal,fp),c.hard_impact)==X87Comparison::less){
                                    (void)player(0x43ce89);const auto limit=params(0x43ce89).run_speed;
                                    auto& speed=motion(0x43ce98).motion.speed;
                                    if(cmp(rotation_vector_length(speed,fp),limit)==X87Comparison::greater){
                                        (void)normalize_vector_extended(speed,c.plane.axis,fp);
                                        speed.x=fp.to_float(fp.multiply_float_memory(fp.from_float(limit),speed.x));
                                        speed.y=fp.to_float(fp.multiply_float_memory(fp.from_float(limit),speed.y));
                                        speed.z=fp.to_float(fp.multiply_float_memory(fp.from_float(limit),speed.z));
                                    }
                                }
                            }
                            event_vector_add(*r.normal_sum,payload(i,0x43cede).oriented_normal,fp);
                        }
                        if(movement)event_vector_add(*r.normal_sum,payload(i,0x43d4f6).oriented_normal,fp);
                    }
                }
            }
            if(i+1>=field(contact.contact_count_at_28,s.loop_end))break;
        }
        if(r.candidates<=0){r.outcome=0;return r;}
        const auto length=rotation_vector_length(field(contact.displacement_at_10,s.final_length),fp);
        const auto distance=selected_distance(s.final_distance);
        if(cmp(length,distance)!=X87Comparison::greater){r.outcome=0;return r;}
        if(!reflection&&!movement){clip(distance);r.outcome=1;return r;}
        auto& p=player(reflection?0x43da69:0x43d53d);
        if(!p.prior_attributes)stop(PlayerContactResponseFailure::unavailable_prior_attributes,reflection?0x43da69:0x43d53d);
        if(*p.prior_attributes&0x4000U){r.outcome=0;return r;}
        auto& v=contact.vector_at_6c;
        const auto v_read=reflection?0x43da76:0x43d54a;
        if(cmp(rotation_vector_length(field(v,v_read),fp),c.plane.axis.zero)==X87Comparison::equal){v=NinjaVector{0,0,0};r.outcome=0;return r;}
        if(reflection){
            const auto add=field(contact.vector_at_78,0x43da9e);
            v->x=fp.to_float(fp.add_float_memory(fp.from_float(add.x),v->x));
            v->y=fp.to_float(fp.add_float_memory(fp.from_float(add.y),v->y));
            v->z=fp.to_float(fp.add_float_memory(fp.from_float(add.z),v->z));
        }else{
            (void)normalize_vector_extended(*v,c.plane.axis,fp);
            v->x=fp.to_float(fp.multiply_float_memory(fp.from_float(v->x),distance));
            v->y=fp.to_float(fp.multiply_float_memory(fp.from_float(v->y),distance));
            v->z=fp.to_float(fp.multiply_float_memory(fp.from_float(distance),v->z));
        }
        if(r.selected<0)stop(PlayerContactResponseFailure::unavailable_response,reflection?0x43dacd:0x43d5b2);
        const auto index=static_cast<std::size_t>(r.selected);
        p.source_vector_at_44=payload(index,reflection?0x43dacd:0x43d5b2).oriented_normal;
        auto& world=motion(reflection?0x43dae4:0x43d5d0);
        NinjaVector negative;
        negative.x=fp.to_float(fp.from_float(world.motion.speed.x).negated());
        negative.y=fp.to_float(fp.from_float(world.motion.speed.y).negated());
        negative.z=fp.to_float(fp.from_float(world.motion.speed.z).negated());
        if(cmp(rotation_vector_length(negative,fp),c.plane.axis.zero)==X87Comparison::equal){r.outcome=0;return r;}
        if(movement){
            (void)normalize_vector_extended(negative,c.plane.axis,fp);
            const auto a=angle(negative,*p.source_vector_at_44,0x43d61e);
            if(a<=0x1000){
                const auto& live=params(0x43d63b);
                const auto comparison=cf(p.motion.speed.x,live.crash_speed);
                if(comparison!=X87Comparison::less&&comparison!=X87Comparison::unordered){*v={0,0,0};r.outcome=2;return r;}
                if(cf(p.motion.speed.x,live.jog_speed)==X87Comparison::greater){
                    const auto scale=fp.to_float(fp.multiply_float_memory(fp.from_float(live.jog_speed),c.slow_factor));
                    (void)normalize_vector_extended(world.motion.speed,c.plane.axis,fp);
                    world.motion.speed.x=fp.to_float(fp.multiply_float_memory(fp.from_float(scale),world.motion.speed.x));
                    world.motion.speed.y=fp.to_float(fp.multiply_float_memory(fp.from_float(scale),world.motion.speed.y));
                    world.motion.speed.z=fp.to_float(fp.multiply_float_memory(fp.from_float(scale),world.motion.speed.z));
                }
                r.outcome=1;return r;
            }
        }
        NinjaVector cross;(void)cross_product_length_extended(*v,*p.source_vector_at_44,cross,fp);
        if(cmp(rotation_vector_length(cross,fp),c.plane.axis.zero)==X87Comparison::equal)cross=field(contact.direction_at_1c,reflection?0x43db44:0x43d6ee);
        else(void)normalize_vector_extended(cross,c.plane.axis,fp);
        const NinjaPlane first{payload(index,reflection?0x43db6a:0x43d714).point_at_20,*p.source_vector_at_44};
        const NinjaPlane second{field(contact.vector_at_60,reflection?0x43db8c:0x43d746),cross};
        NinjaPlane intersection;(void)intersect_source_planes(first,second,&intersection,c.plane,fp);
        NinjaVector direction=*v;
        if(cmp(rotation_vector_length(direction,fp),c.plane.axis.zero)!=X87Comparison::equal)(void)normalize_vector_extended(direction,c.plane.axis,fp);
        if(cmp(inner_product_extended(intersection.normal,direction,fp),c.plane.axis.zero)==X87Comparison::less){
            intersection.normal.x=fp.to_float(fp.from_float(intersection.normal.x).negated());
            intersection.normal.y=fp.to_float(fp.from_float(intersection.normal.y).negated());
            intersection.normal.z=fp.to_float(fp.from_float(intersection.normal.z).negated());
        }
        const auto magnitude=rotation_vector_length(world.motion.speed,fp);
        world.motion.speed.x=fp.to_float(fp.multiply(fp.from_float(intersection.normal.x),magnitude));
        world.motion.speed.y=fp.to_float(fp.multiply(fp.from_float(intersection.normal.y),magnitude));
        world.motion.speed.z=fp.to_float(movement?fp.multiply(fp.from_float(intersection.normal.z),magnitude):fp.multiply_float_memory(magnitude,intersection.normal.z));
        *v=world.motion.speed;
        if(movement){
            const auto heading=event_horizontal_heading(intersection.normal,c.angle.first_factor,c.angle.second_factor,fp);
            if(!actors.task)stop(PlayerContactResponseFailure::null_task,0x43d87c);
            auto yaw=heading;
            actors.task->orientation.angles[1]=signed_bits(yaw);
            if(angle_distance(world.target_yaw,actors.task->orientation.angles[1])>0x6000U){yaw+=0x8000U;actors.task->orientation.angles[1]=signed_bits(yaw);}
            world.target_yaw=actors.task->orientation.angles[1];
        }
        r.outcome=-1;return r;
    }catch(const Stopped&){return r;}
}
} // namespace sadx
