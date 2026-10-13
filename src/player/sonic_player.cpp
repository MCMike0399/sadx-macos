#include "player/sonic_player.hpp"
#include "cpu/crt_integer.hpp"
#include "player/player_hold.hpp"
#include <cstring>
#include <stdexcept>
namespace sadx  {
    namespace  {
        SonicPlayerResult fail(SonicPlayerFailure f,std::uint32_t b,std::uint32_t c=0)  {
            SonicPlayerResult r;
            r.failure=f;
            r.source_boundary=b;
            r.callee_address=c;
            return r;
        }
        SonicPlayerResult done(std::optional<std::uint32_t> v={})  {
            SonicPlayerResult r;
            r.completed=true;
            r.value=v;
            return r;
        }
        PlayerStartupPlayer* player(SonicPlayerFrame& f)  {
            return f.player&&*f.player?f.player->get():nullptr;
        }
        SonicPlayerResult require_player(SonicPlayerFrame& f,std::uint32_t b)  {
            if(!f.player)return fail(SonicPlayerFailure::unknown_player_binding,b);
            auto p=player(f);
            if(!p||!p->player)return fail(SonicPlayerFailure::null_required_resource,b);
            return done();
        }
        SonicPlayerResult call(SonicPlayerRuntime& rt,SonicPlayerFrame& f,X87Context& fp,std::uint32_t a,std::uint32_t b,std::array<std::optional<std::uint32_t>,5> args={},std::optional<std::uint8_t> task_work_argument={})  {
            if(a==0x44a800) {
                if(!rt.animation_runtime)return fail(SonicPlayerFailure::missing_native_owner,b,a);
                auto ready=require_player(f,b);if(!ready)return ready;
                auto* p=player(f);PlayerAnimationView view{*p->player,p->player->animation_tail,p->player_work};
                auto animation=update_player_animation(view,*rt.animation_runtime,fp);
                auto result=animation?done():fail(SonicPlayerFailure::animation_failed,b,a);
                result.animation=animation;
                if(!animation)result.nested_source_boundary=animation.source_boundary;
                return result;
            }
            if(!rt.external)return fail(SonicPlayerFailure::missing_native_owner,b,a);
            auto r=rt.external({a,b,args,task_work_argument},f,fp);
            if(!r)  {
                r.nested_source_boundary=r.source_boundary;
                r.source_boundary=b;
                r.callee_address=a;
                if(r.failure==SonicPlayerFailure::none)r.failure=SonicPlayerFailure::native_owner_failed;
            }
            return r;
        }
        PositionCollisionWork* work(SonicPlayerFrame& f,SonicPlayerResult& r,std::uint32_t b)  {
            if(!f.collision||!f.collision->task_collision)  {
                r=fail(SonicPlayerFailure::unknown_field,b);
                return nullptr;
            }
            auto ref=*f.collision->task_collision;
            if(!ref.id)  {
                r=fail(SonicPlayerFailure::null_required_resource,b);
                return nullptr;
            }
            auto it=f.collision->works.find(*ref.id);
            if(it==f.collision->works.end())  {
                r=fail(SonicPlayerFailure::unresolved_resource,b);
                return nullptr;
            }
            return &it->second;
        }
        PositionInfoTable* infos(SonicPlayerFrame& f,PositionCollisionWork& w,SonicPlayerResult& r,std::uint32_t b)  {
            if(!w.info)  {
                r=fail(SonicPlayerFailure::unknown_field,b);
                return nullptr;
            }
            if(!w.info->id)  {
                r=fail(SonicPlayerFailure::null_required_resource,b);
                return nullptr;
            }
            auto it=f.collision->info_tables.find(*w.info->id);
            if(it==f.collision->info_tables.end())  {
                r=fail(SonicPlayerFailure::unresolved_resource,b);
                return nullptr;
            }
            return &it->second;
        }
        bool gt_unordered(X87Comparison c)  {
            return c==X87Comparison::greater||c==X87Comparison::unordered;
        }
        SonicPlayerResult floor_inverse(SonicPlayerFrame& f,const SonicPlayerNativeMotion& m,NinjaVector& v,X87Context& fp)  {
            auto& p=*player(f)->player;
            if(!p.floor_normal)return fail(SonicPlayerFailure::unknown_field,0x43ed45);
            auto a=floor_normal_angles(*p.floor_normal,m.floor_constants,m.inverse_math,fp);
            if(!a.angles||a.failure!=CrtAsinFailure::none)  {
                auto r=fail(SonicPlayerFailure::coordinate_failed,0x43ed55,0x4b8e80);
                r.floor_angles=a;
                return r;
            }
            auto input=v;
            m.coordinates.begin_unit_frame();
            auto& mat=m.coordinates.current_matrix();
            auto yaw=f.task->orientation.angles[1];
            if(yaw)m.coordinates.rotate_y(mat,static_cast<std::uint16_t>(yaw),fp);
            auto x=0u-static_cast<std::uint32_t>(a.angles->inclination);
            auto z=0u-static_cast<std::uint32_t>(a.angles->roll);
            if(x)m.coordinates.rotate_x(mat,x&0xffff,fp);
            if(z)m.coordinates.rotate_z(mat,z&0xffff,fp);
            auto c=m.coordinates.calculate_vector(mat,input,v,fp);
            if(!c)  {
                auto r=fail(SonicPlayerFailure::coordinate_failed,0x43edae,0x784d60);
                r.coordinate=c;
                return r;
            }
            m.coordinates.end_frame();
            return done();
        }
        SonicPlayerResult movement(SonicPlayerFrame& f,SonicPlayerRuntime& rt,X87Context& fp,bool ground,bool air,bool gravity,std::uint32_t acceleration_site,std::uint32_t speedsite,std::uint32_t positionsite)  {
            auto ready=require_player(f,speedsite);
            if(!ready)return ready;
            if(!rt.motion)return fail(SonicPlayerFailure::missing_native_owner,speedsite,0x443f50);
            auto& m=*rt.motion;
            auto& p=*player(f)->player;
            if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,speedsite);
            auto& params=*player(f)->parameters;
            SonicPlayerResult r;
            if(ground)  {
                auto a=get_player_acceleration_ground(*f.task,*f.world,p,params,m.ground_constants,m.acceleration_runtime,m.ground_callees,fp);
                if(!a)  {
                    r=fail(SonicPlayerFailure::acceleration_failed,acceleration_site,0x44c270);
                    r.nested_source_boundary=a.source_boundary;
                    r.acceleration=a;
                    return r;
                }
            }
            if(air)  {
                auto a=get_player_acceleration_air(*f.task,*f.world,p,params,m.air_constants,m.acceleration_runtime,m.air_callees,fp);
                if(!a)  {
                    r=fail(SonicPlayerFailure::acceleration_failed,acceleration_site,0x44b9c0);
                    r.nested_source_boundary=a.acceleration.source_boundary;
                    r.air=a;
                    return r;
                }
            }
            if(gravity&&!(f.task->orientation.flags&0x4000))  {
                if(!m.acceleration_runtime.world_gravity)return fail(SonicPlayerFailure::unknown_field,0x443826);
                auto g=get_gravity(f.task->orientation,f.world->motion,p.motion,*m.acceleration_runtime.world_gravity,params.weight,[&](const PlayerOrientation& t,NinjaVector& v)  {
                    auto c=m.coordinates.world_to_player(t,v,fp);return PlayerMotionResult{c?PlayerMotionFailure::none:PlayerMotionFailure::world_to_player_transform_unavailable};
                }
                ,fp);
                if(!g)return fail(SonicPlayerFailure::acceleration_failed,acceleration_site,0x443800);
            }
            auto s=run_player_movement(*f.task,m.acceleration_runtime.input,f.world->motion,p,params,m.coordinates,m.floor_constants,m.inverse_math,fp);
            if(!s)  {
                r=fail(SonicPlayerFailure::speed_failed,speedsite,0x443f50);
                r.nested_source_boundary=s.speed.source_boundary;
                r.movement=s;
                return r;
            }
            if(!f.collision)return fail(SonicPlayerFailure::unknown_field,positionsite);
            auto pos=set_player_position(*f.task,f.world.get(),p,params,*f.collision,m.position_constants,m.rotation_constants,m.position_runtime,m.position_callees,&m.coordinates,fp);
            if(!pos||!pos.outcome)  {
                r=fail(SonicPlayerFailure::position_failed,positionsite,0x44cdf0);
                r.nested_source_boundary=pos.source_boundary;
                r.position=pos;
                return r;
            }
            r=done(static_cast<std::uint32_t>(*pos.outcome));
            r.nested_source_boundary=pos.source_boundary;
            r.position=pos;
            return r;
        }
    }
    SonicPlayerFrameBinding bind_sonic_player_frame(const TaskHandle& object,const TaskWorkArena& arena,
    std::shared_ptr<PlayerMotionState> world,const PlayerStartupRegistry& players,PlayerPositionCollisionScene* collision)  {
        SonicPlayerFrameBinding binding;
        auto stop=[&](SonicPlayerFailure f,std::uint32_t b) {
            binding.result=fail(f,b);
            return binding;
        }
        ;
        if(!object)return stop(SonicPlayerFailure::null_required_resource,0x49a9b8);
        if(!object->motion_work)return stop(SonicPlayerFailure::null_required_resource,0x49a9c0);
        const auto reference=arena.reference_view(object->motion_work,0);
        if(!reference) {
            binding.result=fail(SonicPlayerFailure::work_binding_failed,0x49a9c0);
            binding.result.work_failure=reference.failure;
            return binding;
        }
        if(reference.byte_offset) {
            binding.result=fail(SonicPlayerFailure::work_binding_failed,0x49a9c0);
            binding.result.work_failure=TaskWorkFailure::interior_reference_requires_view;
            return binding;
        }
        std::shared_ptr<PlayerStartupPlayer> player;
        if(reference.work) {
            player=players.lookup_player_work(reference.work);
            if(!player)return stop(SonicPlayerFailure::unresolved_resource,0x49a9c0);
        }
        if(!world)return stop(SonicPlayerFailure::unknown_motion_binding,0x49a9bd);
        if(player&&player->motion&&player->motion!=world)return stop(SonicPlayerFailure::conflicting_live_binding,0x49a9bd);
        if(!object->primary_state)return stop(SonicPlayerFailure::unknown_field,0x49a9d0);
        if(!object->primary_work)return stop(SonicPlayerFailure::null_required_resource,0x49a9d0);
        if(object->auxiliary_effect_work)return stop(SonicPlayerFailure::conflicting_live_binding,0x49a9d0);
        binding.frame=SonicPlayerFrame{object,object->primary_state,std::move(world),std::move(player),collision};
        binding.result=done();
        return binding;
    }
    float SonicPlayerConstants::at(std::uint32_t a)const{for(std::size_t i=0;i<addresses.size();++i)if(addresses[i]==a)  {
        float v;
        std::memcpy(&v,&bits[i],4);
        return v;
    }
    throw std::out_of_range("Sonic source constant");
    }
    std::optional<SonicPlayerConstants> decode_sonic_player_constants(const std::uint8_t* b,std::size_t n)  {
        if(!b||n!=52)return {};
        SonicPlayerConstants c;
        for(std::size_t i=0;i<13;++i)c.bits[i]=std::uint32_t(b[4*i])|(std::uint32_t(b[4*i+1])<<8)|(std::uint32_t(b[4*i+2])<<16)|(std::uint32_t(b[4*i+3])<<24);
        return c;
    }
    std::optional<std::uint32_t> sonic_action_target(std::uint8_t action)  {
        static constexpr std::uint32_t table[]={
        0x49ab30,0x49ad2d,0x49ad69,0x49ae04,0x49ae2f,0x49aebb,0x49aec8,0x49af4b,0x49b324,0x49af71,0x49af7e,0x49af9f,0x49b324,0x49b083,0x49afac,0x49aff3,0x49b072,0x49b083,0x49b083,0x49b0ab,0x49be04,0x49b11e,0x49b11e,0x49b24d,0x49b262,0x49b28c,0x49b277,0x49b083,0x49b2e2,0x49b324,0x49af71,0x49af7e,0x49b366,0x49b3be,0x49b3f6,0x49b406,0x49b0c0,0x49b0c0,0x49b0de,0x49b0de,0x49b0fc,0x49be04,0x49b11e,0x49b12b,0x49b200,0x49be04,0x49b438,0x49b506,0x49b6af,0x49b5ca,0x49b771,0x49b7d1,0x49b79a,0x49b7e3,0x49b7f3,0x49b828,0x49b872,0x49b816,0x49b83e,0x49b828,0x49b866,0x49b888,0x49b8bf,0x49be04,0x49b9b1,0x49b9e1,0x49b9e1,0x49b9f6,0x49b9b1,0x49ba36,0x49bb62,0x49bc36,0x49b24d,0x49b24d,0x49b24d,0x49b24d,0x49bc43,0x49bca6,0x49bcde,0x49af7e,0x49af71,0x49bcf3,0x49bcde,0x49b083,0x49bd53,0x49bd61,0x49bdb4,0x49bdf0};
        static_assert(sizeof(table)/4==88);
        if(action>=88)return {};
        return table[action];
    }
    SonicPlayerResult sonic_ground_input(SonicPlayerFrame& f,const SonicPlayerNativeMotion& m,const SonicPlayerConstants& k,X87Context& fp)  {
        if(!f.task)return fail(SonicPlayerFailure::null_required_resource,0x44bb6a);
        if(f.task->orientation.flags&0x4000)return done();
        auto ready=require_player(f,0x44bb81);
        if(!ready)return ready;
        if(!f.world)return fail(SonicPlayerFailure::null_required_resource,0x44bd46);
        auto& p=*player(f)->player;
        SonicPlayerResult r;
        auto mutation=[&](const GroundAccelerationMutation& owner,std::uint32_t address,std::uint32_t site)  {
            if(!owner)return fail(SonicPlayerFailure::missing_native_owner,site,address);
            if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,site);
            auto result=owner(*f.task,*f.world,p,*player(f)->parameters,fp);
            if(!result)  {
                auto out=fail(SonicPlayerFailure::acceleration_failed,site,address);
                out.acceleration=result;
                return out;
            }
            return done();
        }
        ;
        if(f.task->orientation.flags&0x2000)  {
            r=mutation(m.ground_callees.direction_reset,0x43c130,0x44bb98);
            if(!r)return r;
        }
        else  {
            std::int32_t angle=0;
            auto input=get_player_input(input_task_view(*f.task),m.acceleration_runtime.input,fp,&angle,nullptr,m.acceleration_runtime.forced_angle);
            if(!input||!input.active)  {
                r=fail(SonicPlayerFailure::input_failed,0x44bbaa,0x43bdc0);
                r.input=input;
                return r;
            }
            if(!*input.active)  {
                if(f.task->orientation.flags&3)  {
                    auto cmp=fp.compare_float_memory(fp.from_float(p.motion.speed.x),k.at(0x7dcb94));
                    return mutation(m.ground_callees.passive_rotation,0x4491e0,cmp==X87Comparison::greater?0x44bbd7:0x44bbed);
                }
                f.world->target_yaw=f.task->orientation.angles[1];
            }
            else if(p.attributes&0x20000)  {
                if(!m.ground_input_projection)return fail(SonicPlayerFailure::missing_native_owner,0x44bc19);
                const auto& g=*m.ground_input_projection;
                const auto index=f.task->player_index;
                if(index>=g.surface.raw_axes.size()||!g.surface.raw_axes[index])return fail(SonicPlayerFailure::unknown_field,0x44bc19);
                const auto raw=*g.surface.raw_axes[index];
                auto x=fp.from_int64(static_cast<std::int32_t>(raw[0])*256);
                auto y=fp.from_int64(static_cast<std::int32_t>(raw[1])*256);
                auto a=X87Transcendental::fpatan_bochs_p5_p6(fp,x,y);
                a=fp.multiply_double_memory(a,g.angle_units);
                a=fp.multiply_double_memory(a,g.positive_factor);
                auto rawangle=crt_ftol_low32(a,fp);
                m.coordinates.begin_unit_frame();
                auto& camera=m.coordinates.current_matrix();
                if(!g.surface.camera_pitch_yaw)return fail(SonicPlayerFailure::unknown_field,0x44bc7b);
                const auto angles=*g.surface.camera_pitch_yaw;
                if(angles[1])m.coordinates.rotate_y(camera,static_cast<std::uint16_t>(angles[1]),fp);
                if(angles[0])m.coordinates.rotate_x(camera,static_cast<std::uint16_t>(angles[0]),fp);
                NinjaVector v{0,0,-1},camera_direction,projected;
                auto c=m.coordinates.calculate_vector(camera,v,camera_direction,fp);
                if(!c)  {
                    r=fail(SonicPlayerFailure::coordinate_failed,0x44bcad,0x784d60);
                    r.coordinate=c;
                    return r;
                }
                m.coordinates.end_frame();
                m.coordinates.begin_unit_frame();
                auto& floor=m.coordinates.current_matrix();
                if(!p.floor_normal)return fail(SonicPlayerFailure::unknown_field,0x44bcd0);
                m.coordinates.rotate_axis(floor,*p.floor_normal,static_cast<std::int32_t>(rawangle),g.axis,fp);
                c=m.coordinates.calculate_vector(floor,camera_direction,projected,fp);
                if(!c)  {
                    r=fail(SonicPlayerFailure::coordinate_failed,0x44bce0,0x784d60);
                    r.coordinate=c;
                    return r;
                }
                m.coordinates.end_frame();
                v={1,0,0};
                c=m.coordinates.task_to_world(&f.task->orientation,v,fp);
                if(!c)  {
                    r=fail(SonicPlayerFailure::coordinate_failed,0x44bd0d,0x43ec90);
                    r.coordinate=c;
                    return r;
                }
                auto projection=projection_vector_angle(v,projected,&*p.floor_normal,g.axis,g.projection,m.inverse_math,fp);
                if(!projection)  {
                    r=fail(SonicPlayerFailure::coordinate_failed,0x44bd1d,0x438410);
                    r.projection=projection;
                    return r;
                }
                f.world->target_yaw=static_cast<std::int32_t>((static_cast<std::uint32_t>(f.task->orientation.angles[1])+static_cast<std::uint32_t>(*projection.angle))&65535);
                if(!m.ground_callees.yaw_blended)return fail(SonicPlayerFailure::missing_native_owner,0x44bd38,0x443e60);
                if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x44bd38);
                auto blended=m.ground_callees.yaw_blended(*f.task,*f.world,p,*player(f)->parameters,f.world->target_yaw,fp);
                if(!blended)  {
                    r=fail(SonicPlayerFailure::acceleration_failed,0x44bd38,0x443e60);
                    r.acceleration=blended;
                    return r;
                }
            }
            else  {
                // Preserve the actual angle-output contract. The entry4000 guard
                // excludes GetAnalog's event-only active/no-store branch; if a
                // future input owner violates that invariant, do not use seed0.
                if(!(input.outputs_written&1))  {
                    r=fail(SonicPlayerFailure::input_failed,0x44bd42,0x43bdc0);
                    r.input=input;
                    return r;
                }
                f.world->target_yaw=angle;
            }
        }
        if(!m.ground_callees.yaw_regular)return fail(SonicPlayerFailure::missing_native_owner,0x44bd5a,0x443c50);
        if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x44bd5a);
        auto rotation=m.ground_callees.yaw_regular(*f.task,*f.world,p,*player(f)->parameters,f.world->target_yaw,fp);
        if(!rotation)  {
            r=fail(SonicPlayerFailure::acceleration_failed,0x44bd5a,0x443c50);
            r.acceleration=rotation;
            return r;
        }
        return done();
    }
    SonicPlayerResult sonic_copy_local_speed(SonicPlayerFrame& f,const SonicPlayerNativeMotion& m,const SonicPlayerConstants& k,X87Context& fp)  {
        if(!f.task||!f.world)return fail(SonicPlayerFailure::null_required_resource,0x43ee85);
        auto ready=require_player(f,0x43ee9f);
        if(!ready)return ready;
        auto& p=*player(f)->player;
        NinjaVector v=f.world->motion.speed;
        const bool ground=f.task->orientation.flags&3;
        SonicPlayerResult r;
        if(ground&&(p.attributes&0x4000))  {
            r=floor_inverse(f,m,v,fp);
            if(!r)return r;
        }
        else  {
            auto c=m.coordinates.world_to_player(f.task->orientation,v,fp);
            if(!c)  {
                r=fail(SonicPlayerFailure::coordinate_failed,0x43eec4,0x43ec00);
                r.coordinate=c;
                return r;
            }
        }
        return sonic_finalize_local_speed(f,v,k,m.acceleration_runtime.world_gravity,fp);
    }
    SonicPlayerResult sonic_finalize_local_speed(SonicPlayerFrame& f,NinjaVector& v,const SonicPlayerConstants& k,const std::optional<NinjaVector>& gravity,X87Context& fp)  {
        if(!f.task||!f.world)return fail(SonicPlayerFailure::null_required_resource,0x43eecc);
        auto ready=require_player(f,0x43eee5);
        if(!ready)return ready;
        auto& p=*player(f)->player;
        const bool ground=f.task->orientation.flags&3;
        if(ground)v.y=0;
        if(ground)  {
            if(!p.prior_attributes)return fail(SonicPlayerFailure::unknown_field,0x43eee5);
            if(!(*p.prior_attributes&0xc))  {
                if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x43eeef);
                auto threshold=fp.multiply_float_memory(fp.from_float(player(f)->parameters->run_accel),k.at(0x7dfff0));
                auto c=fp.compare_float_memory(fp.from_float(v.x),k.at(0x7dd554));
                if(c==X87Comparison::less&&fp.compare_float_memory(fp.from_float(v.x),k.at(0x7e0090))==X87Comparison::greater)v.x=0;
                auto z=fp.compare(fp.from_float(v.z),threshold);
                if(!gt_unordered(z))  {
                    auto n=fp.compare_float_memory(threshold.negated(),v.z);
                    if(!gt_unordered(n))v.z=0;
                }
            }
        }
        else  {
            for(float* s:{&v.x,&v.z})  {
                auto c=fp.compare_float_memory(fp.from_float(*s),k.at(0x7dd57c));
                if(!gt_unordered(c))  {
                    auto d=fp.compare_float_memory(fp.from_float(*s),k.at(0x7df1bc));
                    if(d!=X87Comparison::less&&d!=X87Comparison::unordered)*s=0;
                }
            }
        }
        p.motion.speed=v;
        for(auto& a:f.task->orientation.angles)a=static_cast<std::uint16_t>(a);
        if(!f.world->angle_x_at_1c)return fail(SonicPlayerFailure::unknown_field,0x43efc0);
        f.world->angle_x_at_1c=static_cast<std::uint16_t>(*f.world->angle_x_at_1c);
        f.world->target_yaw=static_cast<std::uint16_t>(f.world->target_yaw);
        if(!f.world->angle_z_at_24)return fail(SonicPlayerFailure::unknown_field,0x43efc8);
        f.world->angle_z_at_24=static_cast<std::uint16_t>(*f.world->angle_z_at_24);
        if(ground&&!(p.attributes&0x4000))  {
            if(!p.floor_normal||!gravity)return fail(SonicPlayerFailure::unknown_field,0x43efe2);
            p.dotp=fp.to_float(inner_product_extended(*gravity,*p.floor_normal,fp).negated());
        }
        else p.dotp=1;
        return done();
    }
    SonicPlayerResult sonic_update_collision_center(SonicPlayerFrame& f,PlayerCoordinates& coords,X87Context& fp)  {
        if(!f.task)return fail(SonicPlayerFailure::null_required_resource,0x4915f3);
        if(!f.collision||!f.collision->task_collision)return fail(SonicPlayerFailure::unknown_field,0x4915f3);
        auto ready=require_player(f,0x4915f6);
        if(!ready)return ready;
        if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x4915f6);
        SonicPlayerResult r;
        auto w=work(f,r,0x4915fd);
        if(!w)return r;
        auto info=infos(f,*w,r,0x4915fd);
        if(!info)return r;
        NinjaVector v{0,player(f)->parameters->center_height,0};
        auto c=coords.task_to_world(&f.task->orientation,v,fp);
        if(!c)  {
            r=fail(SonicPlayerFailure::coordinate_failed,0x49161a,0x43ec90);
            r.coordinate=c;
            return r;
        }
        auto it=info->records.find(0);
        if(it==info->records.end())return fail(SonicPlayerFailure::unresolved_resource,0x491628);
        it->second.center=v;
        if(!f.task->position)return fail(SonicPlayerFailure::unknown_field,0x491638);
        auto& center=*it->second.center;
        const auto pos=*f.task->position;
        center.x=fp.to_float(fp.add_float_memory(fp.from_float(pos.x),center.x));
        center.y=fp.to_float(fp.add_float_memory(fp.from_float(pos.y),center.y));
        center.z=fp.to_float(fp.add_float_memory(fp.from_float(pos.z),center.z));
        return done();
    }
    namespace  {
        SonicPlayerResult pad_test(SonicPlayerFrame& f,const SonicPlayerRuntime& rt,bool held,
        std::uint32_t mask_address,std::uint32_t mask_load,std::uint32_t pad_read)  {
            if(!f.task)return fail(SonicPlayerFailure::null_required_resource,pad_read);
            if(!rt.controls||!rt.controls->masks)return fail(SonicPlayerFailure::unknown_control,mask_load);
            const auto& c=*rt.controls->masks;
            const std::optional<std::uint32_t>* mask=nullptr;
            switch(mask_address) {
                case 0x909f94:mask=&c.mask_at_909f94;
                break;
                case 0x909f98:mask=&c.mask_at_909f98;
                break;
                case 0x909fa4:mask=&c.mask_at_909fa4;
                break;
                case 0x909fa8:mask=&c.mask_at_909fa8;
                break;
                case 0x909fac:mask=&c.mask_at_909fac;
                break;
                default:throw std::out_of_range("source button mask");
            }
            if(!*mask)return fail(SonicPlayerFailure::unknown_control,mask_load);
            const auto index=f.task->player_index;
            const auto* pads=rt.controls->engine_pads;
            if(!pads||index>=pads->records.size())return fail(SonicPlayerFailure::unknown_control,pad_read);
            const auto word=pads->records[index].read_u32(held?GameLogicController::held_offset:GameLogicController::pressed_offset);
            if(!word)return fail(SonicPlayerFailure::unknown_control,pad_read);
            return done((*word&**mask)!=0?1:0);
        }
        SonicPlayerResult native_hold(SonicPlayerFrame& f,SonicPlayerRuntime& rt,X87Context& fp,std::uint32_t site)  {
            (void)fp;
            // Actual43b9c0 is integer-only; no invented FWAIT.
            if(!f.collision)return fail(SonicPlayerFailure::unknown_field,site);
            PlayerHoldRuntime context{[&](std::uint8_t index) {
                return rt.players.lookup_player(index);
            }
            ,rt.scheduler.work_arena()};
            auto query=query_player_hold(*f.task,*f.collision,context);
            PositionHoldResult held{query?PlayerPositionFailure::none:PlayerPositionFailure::hold_owner_failed,
            query.flags,query.source_boundary,std::make_shared<PlayerHoldResult>(query)};
            if(!query||!query.flags) {
                auto r=fail(SonicPlayerFailure::native_owner_failed,site,0x43b9c0);
                r.hold=held;
                r.nested_source_boundary=query.source_boundary;
                return r;
            }
            auto r=done(*query.flags);
            r.hold=held;
            return r;
        }
        SonicPlayerResult input_active(SonicPlayerFrame& f,SonicPlayerRuntime& rt,X87Context& fp,std::uint32_t site,std::int32_t* angle=nullptr)  {
            if(!rt.motion)return fail(SonicPlayerFailure::missing_native_owner,site,0x43bdc0);
            auto q=get_player_input(input_task_view(*f.task),rt.motion->acceleration_runtime.input,fp,angle,nullptr,rt.motion->acceleration_runtime.forced_angle);
            if(!q||!q.active) {
                auto r=fail(SonicPlayerFailure::input_failed,site,0x43bdc0);
                r.input=q;
                r.nested_source_boundary=q.source_boundary;
                return r;
            }
            auto r=done(*q.active?1:0);
            r.input=q;
            return r;
        }
        SonicPlayerResult clear_source_tasks(SonicPlayerRuntime& rt)  {
            for(std::size_t i=0;i<10;++i) {
                const auto site=0x4b8320+15*static_cast<std::uint32_t>(i);
                const auto& field=rt.globals.tasks_at_3c5a27c[i];
                if(!field)return fail(SonicPlayerFailure::unknown_field,site);
                const auto& task=*field;
                if(!task)continue;
                if(!task->primary_work)return fail(SonicPlayerFailure::null_required_resource,site+12);
                if(task->primary_state)task->primary_state->action_at_0=0;
                else {
                    auto arena=rt.scheduler.work_arena();
                    if(!arena)return fail(SonicPlayerFailure::missing_native_owner,site+12);
                    std::uint8_t zero=0;
                    auto w=arena->write(task->primary_work,0,&zero,1);
                    if(!w) {
                        auto r=fail(SonicPlayerFailure::work_binding_failed,site+12);
                        r.work_failure=w.failure;
                        return r;
                    }
                }
            }
            return done();
        }
        PositionHitActor* hit_actor(SonicPlayerFrame& f,PositionCollisionWork& own,SonicPlayerResult& error,
        std::uint32_t hit_site,std::uint32_t task_site,std::uint32_t work_site)  {
            if(!own.hit_work) {
                error=fail(SonicPlayerFailure::unknown_field,hit_site);
                return nullptr;
            }
            if(!own.hit_work->id) {
                error=done(0);
                return nullptr;
            }
            auto hit=f.collision->works.find(*own.hit_work->id);
            if(hit==f.collision->works.end()) {
                error=fail(SonicPlayerFailure::unresolved_resource,task_site);
                return nullptr;
            }
            auto ref=hit->second.owning_task;
            if(!ref) {
                error=fail(SonicPlayerFailure::unknown_field,task_site);
                return nullptr;
            }
            if(!ref->id) {
                error=fail(SonicPlayerFailure::null_required_resource,work_site);
                return nullptr;
            }
            auto object=f.collision->tasks.find(*ref->id);
            if(object==f.collision->tasks.end()) {
                error=fail(SonicPlayerFailure::unresolved_resource,work_site);
                return nullptr;
            }
            if(!object->second.work) {
                error=fail(SonicPlayerFailure::unknown_field,work_site);
                return nullptr;
            }
            if(!object->second.work->id) {
                error=fail(SonicPlayerFailure::null_required_resource,work_site+3);
                return nullptr;
            }
            auto actor=f.collision->actors.find(*object->second.work->id);
            if(actor==f.collision->actors.end()) {
                error=fail(SonicPlayerFailure::unresolved_resource,work_site+3);
                return nullptr;
            }
            return &actor->second;
        }
        SonicPlayerResult request_animation(SonicPlayerPredicateEntry entry,SonicPlayerFrame& f,SonicPlayerRuntime& rt)  {
            const std::uint32_t base=static_cast<std::uint32_t>(entry);
            if(!rt.globals.alternate_model_at_3b18db5)return fail(SonicPlayerFailure::unknown_field,base);
            const bool walk=entry==SonicPlayerPredicateEntry::request_walk_animation_495ba0;
            auto ready=require_player(f,base+(*rt.globals.alternate_model_at_3b18db5?(walk?9:15):(walk?0x33:0x40)));
            if(!ready)return ready;
            auto& p=*player(f)->player;
            const std::uint16_t requested=walk?11:entry==SonicPlayerPredicateEntry::request_jump_animation_495b50?16:14;
            bool create=false;
            if(*rt.globals.alternate_model_at_3b18db5) {
                if(!p.requested_animation_at_124)return fail(SonicPlayerFailure::unknown_field,base+(walk?9:15));
                create=walk?*p.requested_animation_at_124==0x92:*p.requested_animation_at_124!=requested;
            }
            if(create) {
                const auto site=base+(walk?0x1d:0x21);
                if(!rt.spin_effect_callback_495a50)return fail(SonicPlayerFailure::missing_native_owner,site,0x495a50);
                auto result=rt.scheduler.create_element(2,3,rt.spin_effect_callback_495a50);
                if(!result&&result.failure!=TaskFailure::source_pool_exhausted) {
                    auto r=fail(SonicPlayerFailure::native_owner_failed,site,0x40b860);
                    r.task_creation=result;
                    return r;
                }
                if(result.task) {
                    if(!result.task->primary_work)return fail(SonicPlayerFailure::null_required_resource,base+(walk?0x2c:0x30));
                    const std::int32_t counter=walk?5:requested==16?20:10;
                    if(result.task->primary_state)result.task->primary_state->source_counter_at_c=counter;
                    else {
                        auto arena=rt.scheduler.work_arena();
                        if(!arena)return fail(SonicPlayerFailure::missing_native_owner,base+(walk?0x2c:0x30));
                        auto w=arena->write(result.task->primary_work,0xc,&counter,4);
                        if(!w) {
                            auto r=fail(SonicPlayerFailure::work_binding_failed,base+(walk?0x2c:0x30));
                            r.work_failure=w.failure;
                            return r;
                        }
                    }
                }
            }
            p.requested_animation_at_124=requested;
            return done();
        }
    }
    SonicPlayerResult sonic_action_predicate(SonicPlayerPredicateEntry entry,SonicPlayerFrame& f,SonicPlayerRuntime& rt,X87Context& fp,std::optional<std::uint32_t> argument)  {
        if(entry==SonicPlayerPredicateEntry::clear_tasks_4b8320)return clear_source_tasks(rt);
        if(entry==SonicPlayerPredicateEntry::request_spin_animation_495c30||entry==SonicPlayerPredicateEntry::request_jump_animation_495b50||entry==SonicPlayerPredicateEntry::request_walk_animation_495ba0)return request_animation(entry,f,rt);
        if(!f.task)return fail(SonicPlayerFailure::null_required_resource,static_cast<std::uint32_t>(entry));
        auto& flags=f.task->orientation.flags;
        auto predicate=[&](SonicPlayerPredicateEntry e) {
            return sonic_action_predicate(e,f,rt,fp);
        }
        ;
        switch(entry) {
            case SonicPlayerPredicateEntry::wall_form_43bfe0:{
            if(!rt.motion||!rt.motion->position_callees.normal)return fail(SonicPlayerFailure::missing_native_owner,0x43bff1,0x41bed0);
            if(!f.collision)return fail(SonicPlayerFailure::unknown_field,0x43bff1);
            auto normal=rt.motion->position_callees.normal(*f.task,*f.collision,fp);
            if(normal.failure!=PlayerPositionFailure::none||!normal.normal) {
                auto r=fail(SonicPlayerFailure::native_owner_failed,0x43bff1,0x41bed0);
                r.normal=normal;
                r.nested_source_boundary=normal.source_boundary;
                return r;
            }
            auto z=fp.from_float(normal.normal->z).negated();
            auto x=fp.from_float(normal.normal->x).negated();
            auto a=X87Transcendental::fpatan_bochs_p5_p6(fp,z,x);
            a=fp.multiply_double_memory(a,rt.motion->position_constants.angle_units);
            a=fp.multiply_double_memory(a,rt.motion->position_constants.positive_factor);
            const auto angle=crt_ftol_low32(a,fp);
            const auto distance=angle_distance(f.task->orientation.angles[1],static_cast<std::int32_t>(angle));
            auto result=done(distance>0x6000?1:distance>0x3400?2:distance>0x1c00?3:4);
            result.normal=normal;
            return result;
            }
            case SonicPlayerPredicateEntry::skid_end_4930d0:{
            auto ready=require_player(f,0x4930d0);
            if(!ready)return ready;
            if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x4930d3);
            auto& p=*player(f)->player;
            if(fp.compare_float_memory(fp.from_float(p.motion.speed.x),player(f)->parameters->run_speed)!=X87Comparison::less)return done(0);
            auto c=fp.compare_float_memory(fp.from_float(p.motion.speed.x),rt.constants.at(0x7dcb2c));
            if(c==X87Comparison::greater||c==X87Comparison::unordered)f.task->action_at_0=2;
            else {
                f.task->action_at_0=1;
                p.idle_timer_at_e=0;
            }
            p.requested_animation_at_124=29;
            return done(1);
            }
            case SonicPlayerPredicateEntry::jump_classify_4b83f0:{
            SonicPlayerResult error;
            auto own=work(f,error,0x4b83f4);
            if(!own)return error;
            if(!own->hit_work)return fail(SonicPlayerFailure::unknown_field,0x4b83f7);
            PositionHitActor* actor=nullptr;
            if(own->hit_work->id&&f.task->player_index==0) {
                actor=hit_actor(f,*own,error,0x4b83f7,0x4b840a,0x4b8410);
                if(!actor)return error;
                auto id=position_actor_task_id(*actor);
                if(!id)return fail(SonicPlayerFailure::unknown_field,0x4b8413);
                if(*id!=8)actor=nullptr;
            }
            auto reset_return=[&](std::uint32_t value) {
                if(f.task->player_index==0) {
                    auto clear=clear_source_tasks(rt);
                    if(!clear)return clear;
                }
                return done(value);
            }
            ;
            if(!actor)return reset_return(0);
            auto timer=position_actor_control_timer(*actor);
            if(!timer)return fail(SonicPlayerFailure::unknown_field,0x4b8433);
            if(*timer==0)return reset_return(0xffffffff);
            if(!own->my_info_index)return fail(SonicPlayerFailure::unknown_field,0x4b8444);
            if(*own->my_info_index!=1)return reset_return(0);
            auto counter=position_actor_source_counter(*actor);
            if(!counter)return fail(SonicPlayerFailure::unknown_field,0x4b844e);
            auto c=fp.compare_float_memory(fp.from_int64(*counter),rt.constants.at(0x7df1c8));
            if(c==X87Comparison::less||c==X87Comparison::unordered)return done(1);
            return reset_return(0);
            }
            case SonicPlayerPredicateEntry::jump_press_43bf40:{
            if(flags&0x4000)return done(0);
            auto pressed=pad_test(f,rt,false,0x909f98,0x43bf59,0x43bf62);
            if(!pressed||!*pressed.value)return pressed;
            auto classified=predicate(SonicPlayerPredicateEntry::jump_classify_4b83f0);
            if(!classified)return classified;
            return done(static_cast<std::int32_t>(*classified.value)>0?2:1);
            }
            case SonicPlayerPredicateEntry::event_action_495fa0:{
            if(!f.task->event_move||!f.task->event_move->mode)return fail(SonicPlayerFailure::unknown_field,0x495fac);
            if(*f.task->event_move->mode) {
                auto effect=call(rt,f,fp,0x45a870,0x495fb9);
                if(!effect)return effect;
                return done(0);
            }
            auto list=f.task->event_move->action_list_at_4c;
            if(!list)return fail(SonicPlayerFailure::unknown_field,0x495fc7);
            if(list->id) {
                flags&=0xdaff;
                f.task->action_at_0=84;
                return done(1);
            }
            if(!(flags&0x1000))return done(0);
            if(!f.task->secondary_mode_at_1)return fail(SonicPlayerFailure::unknown_field,0x495fee);
            const auto sub=static_cast<std::int8_t>(*f.task->secondary_mode_at_1);
            if(sub<=0||sub>50)return done(0);
            return fail(SonicPlayerFailure::unimplemented_action,0x495ff8,0x495fa0);
            }
            case SonicPlayerPredicateEntry::fall_494f70:{
            if(flags&3)return done(0);
            if(flags&0x100) {
                f.task->action_at_0=8;
                return done(1);
            }
            if(flags&0x800) {
                auto held=native_hold(f,rt,fp,0x494fa3);
                if(!held)return held;
                if(*held.value&0x200) {
                    f.task->action_at_0=28;
                    auto ready=require_player(f,0x494fb4);
                    if(!ready)return ready;
                    player(f)->player->requested_animation_at_124=57;
                    return done(1);
                }
                auto drop=call(rt,f,fp,0x473b00,0x494fc9);
                if(!drop)return drop;
            }
            f.task->action_at_0=12;
            auto ready=require_player(f,0x494fd5);
            if(!ready)return ready;
            player(f)->player->requested_animation_at_124=18;
            return done(1);
            }
            case SonicPlayerPredicateEntry::roll_release_492f90:{
            auto held=pad_test(f,rt,true,0x909f94,0x492f94,0x492f9d);
            if(!held)return held;
            if(*held.value||!(flags&0x200))return done(0);
            auto ready=require_player(f,0x492fbe);
            if(!ready)return ready;
            auto& p=*player(f)->player;
            if(!p.equipment_at_4)return fail(SonicPlayerFailure::unknown_field,0x492fbe);
            bool upgraded=false;
            if(*p.equipment_at_4&4) {
                auto check=call(rt,f,fp,0x492710,0x492fc6);
                if(!check)return check;
                if(!check.value)return fail(SonicPlayerFailure::native_owner_failed,0x492fce,0x492710);
                upgraded=*check.value!=0;
            }
            flags&=0xfdff;
            if(upgraded) {
                f.task->action_at_0=44;
                p.requested_animation_at_124=15;
                p.motion.speed.y=8;
                flags|=0x500;
                auto sound=call(rt,f,fp,0x423d70,0x492ff8,{0x2fc,0,0,0});
                if(!sound)return sound;
                p.timer_at_7e=0;
                if(!rt.globals.stage_at_3b22dcc)return fail(SonicPlayerFailure::unknown_field,0x493003);
                auto stage=static_cast<std::int16_t>(*rt.globals.stage_at_3b22dcc);
                if(stage<15||stage>25) {
                    auto effect=call(rt,f,fp,0x437d20,0x493021,{0x464df0,0,0});
                    if(!effect)return effect;
                }
                SonicPlayerResult error;
                auto own=work(f,error,0x493029);
                if(!own)return error;
                auto info=infos(f,*own,error,0x49302c);
                if(!info)return error;
                auto it=info->records.find(0);
                if(it==info->records.end())return fail(SonicPlayerFailure::unresolved_resource,0x49302f);
                if(!it->second.damage)return fail(SonicPlayerFailure::unknown_field,0x49302f);
                it->second.damage=static_cast<std::uint8_t>(*it->second.damage|3);
                it->second.damage=static_cast<std::uint8_t>(*it->second.damage|12);
                return done(1);
            }
            f.task->action_at_0=6;
            p.requested_animation_at_124=64;
            p.timer_at_80=10;
            p.timer_at_82=0;
            p.motion.speed.x=8;
            flags=static_cast<std::uint16_t>((flags&0xfeff)|0x400);
            auto sound=call(rt,f,fp,0x423d70,0x49308d,{0x2fc,0,0,0});
            if(!sound)return sound;
            if(!rt.globals.alternate_model_at_3b18db5)return fail(SonicPlayerFailure::unknown_field,0x493092);
            if(*rt.globals.alternate_model_at_3b18db5==0) {
                sound=call(rt,f,fp,0x424210,0x4930a3,{0x4cb});
                if(!sound)return sound;
                sound=call(rt,f,fp,0x423d70,0x4930b3,{0x4cc,0,0,0});
                if(!sound)return sound;
            }
            return done(1);
            }
            case SonicPlayerPredicateEntry::charge_496ee0:{
            auto pressed=pad_test(f,rt,false,0x909f94,0x496ee0,0x496ef0);
            if(!pressed||!*pressed.value)return pressed;
            auto ready=require_player(f,0x496ef8);
            if(!ready)return ready;
            auto& p=*player(f)->player;
            f.task->action_at_0=4;
            auto animation=predicate(SonicPlayerPredicateEntry::request_spin_animation_495c30);
            if(!animation)return animation;
            flags|=0x500;
            const auto c=fp.compare_float_memory(fp.from_float(p.motion.speed.x),rt.constants.at(0x7dcb94));
            p.timer_at_7e=0;
            p.source_charge_at_0=c==X87Comparison::greater?p.motion.speed.x:2;
            auto sound=call(rt,f,fp,0x423f50,0x496f33,{0x2ff,{},1,0,2},1);
            if(!sound)return sound;
            return done(1);
            }
            case SonicPlayerPredicateEntry::hold_493120:{
            auto pressed=pad_test(f,rt,false,0x909fa4,0x493124,0x49312d);
            if(!pressed||!*pressed.value)return pressed;
            auto held=native_hold(f,rt,fp,0x49313b);
            if(!held||!*held.value)return held;
            auto form=predicate(SonicPlayerPredicateEntry::wall_form_43bfe0);
            if(!form)return form;
            if(!form.value)return fail(SonicPlayerFailure::native_owner_failed,0x49315b,0x43bfe0);
            const auto type=static_cast<std::int32_t>(*form.value);
            if(type<=0||type>4)return done(0);
            auto ready=require_player(f,0x49316c);
            if(!ready)return ready;
            auto& p=*player(f)->player;
            if(*held.value&0x400) {
                f.task->action_at_0=24;
                p.requested_animation_at_124=53;
                SonicPlayerResult error;
                auto own=work(f,error,0x493175);
                if(!own)return error;
                if(!own->hit_work)return fail(SonicPlayerFailure::unknown_field,0x493178);
                if(!own->hit_work->id)return fail(SonicPlayerFailure::null_required_resource,0x49317e);
                auto hit=f.collision->works.find(*own->hit_work->id);
                if(hit==f.collision->works.end())return fail(SonicPlayerFailure::unresolved_resource,0x49317e);
                if(!hit->second.owning_task)return fail(SonicPlayerFailure::unknown_field,0x49317e);
                auto ref=*hit->second.owning_task;
                if(!ref.id)p.held_task_at_68=TaskHandle{};
                else {
                    if(!rt.collision_tasks)return fail(SonicPlayerFailure::unresolved_resource,0x493184);
                    auto object=rt.collision_tasks->find(*ref.id);
                    if(object==rt.collision_tasks->end())return fail(SonicPlayerFailure::unresolved_resource,0x493184);
                    p.held_task_at_68=object->second;
                }
                flags|=0x800;
                return done(1);
            }
            if(*held.value&0x200) {
                f.task->action_at_0=21;
                if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x49319d);
                auto c=fp.compare_float_memory(fp.from_float(p.motion.speed.x),player(f)->parameters->jog_speed);
                p.requested_animation_at_124=33;
                if(c!=X87Comparison::less&&c!=X87Comparison::unordered)p.motion.speed.x=player(f)->parameters->jog_speed;
                return done(1);
            }
            if(*held.value&0x800) {
                f.task->action_at_0=24;
                p.requested_animation_at_124=53;
                return done(1);
            }
            return done(0);
            }
            case SonicPlayerPredicateEntry::jump_495e60:{
            auto first=predicate(SonicPlayerPredicateEntry::jump_press_43bf40);
            if(!first)return first;
            if(*first.value!=2) {
                auto second=predicate(SonicPlayerPredicateEntry::jump_press_43bf40);
                if(!second||!*second.value)return second;
            }
            auto ready=require_player(f,0x495e70);
            if(!ready)return ready;
            auto& p=*player(f)->player;
            if(*first.value==2) {
                if(flags&0x800) {
                    auto drop=call(rt,f,fp,0x473b00,0x495e86);
                    if(!drop)return drop;
                }
                auto trigger=call(rt,f,fp,0x4b8470,0x495e8f);
                if(!trigger)return trigger;
                f.task->action_at_0=40;
                auto animation=call(rt,f,fp,0x495c80,0x495e9a);
                if(!animation)return animation;
                flags&=0xfffd;
            }
            else {
                bool carried=false;
                if(flags&0x800) {
                    auto held=native_hold(f,rt,fp,0x495ede);
                    if(!held)return held;
                    if(*held.value&0x200) {
                        f.task->action_at_0=29;
                        if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x495eee);
                        p.motion.speed.y=player(f)->parameters->jmp_y_spd;
                        p.requested_animation_at_124=55;
                        flags&=0xfffd;
                        carried=true;
                    }
                    else {
                        auto drop=call(rt,f,fp,0x473b00,0x495f07);
                        if(!drop)return drop;
                    }
                }
                if(!carried) {
                    f.task->action_at_0=8;
                    if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x495f12);
                    p.motion.speed.y=player(f)->parameters->jmp_y_spd;
                    if(!p.moving_task_at_6c)return fail(SonicPlayerFailure::unknown_field,0x495f1b);
                    if(*p.moving_task_at_6c) {
                        auto moving=*p.moving_task_at_6c;
                        if(moving->force_work) {
                            auto arena=rt.scheduler.work_arena();
                            if(!arena)return fail(SonicPlayerFailure::missing_native_owner,0x495f29);
                            std::array<float,3> vector;
                            auto bytes=arena->read(moving->force_work,0x10,vector.data(),12);
                            if(!bytes) {
                                auto r=fail(SonicPlayerFailure::work_binding_failed,0x495f29);
                                r.work_failure=bytes.failure;
                                return r;
                            }
                            const auto index=f.task->player_index;
                            if(index>=rt.players.motion_work.size()||!rt.players.motion_work[index])return fail(SonicPlayerFailure::unknown_motion_binding,0x441755);
                            auto world=*rt.players.motion_work[index];
                            if(!world)return fail(SonicPlayerFailure::null_required_resource,0x441764);
                            world->motion.acceleration={vector[0],vector[1],vector[2]};
                        }
                    }
                    auto animation=predicate(SonicPlayerPredicateEntry::request_spin_animation_495c30);
                    if(!animation)return animation;
                    p.source_charge_at_0=5;
                    flags=static_cast<std::uint16_t>((flags&0xfffd)|0x500);
                }
            }
            p.jump_timer_at_8=0;
            if(!rt.globals.alternate_model_at_3b18db5)return fail(SonicPlayerFailure::unknown_field,0x495f69);
            const bool alternate=*rt.globals.alternate_model_at_3b18db5!=0;
            auto sound=call(rt,f,fp,0x423d70,alternate?0x495f7f:0x495eaa,{alternate?0x307U:0x11U,0,0,0});
            if(!sound)return sound;
            return done(1);
            }
            case SonicPlayerPredicateEntry::charge_tick_495080:{
            auto ready=require_player(f,0x495080);
            if(!ready)return ready;
            auto& p=*player(f)->player;
            if(!p.timer_at_7e)return fail(SonicPlayerFailure::unknown_field,0x495080);
            const auto timer=static_cast<std::int16_t>(*p.timer_at_7e);
            bool enhanced=false;
            if(timer>=120) {
                if(!p.equipment_at_4)return fail(SonicPlayerFailure::unknown_field,0x49508a);
                enhanced=(*p.equipment_at_4&3)==3;
            }
            if(!enhanced&&timer>=210) {
                if(!p.equipment_at_4)return fail(SonicPlayerFailure::unknown_field,0x49509f);
                enhanced=(*p.equipment_at_4&2)!=0;
            }
            auto cleanup=[&](bool effect) {
                auto speed=fp.from_float(p.motion.speed.x);
                p.timer_at_7e=0;
                auto comparison=fp.compare_float_memory(speed,rt.constants.at(0x7dcb2c));
                if(comparison==X87Comparison::greater||comparison==X87Comparison::unordered)f.task->action_at_0=2;
                else {
                    f.task->action_at_0=1;
                    p.idle_timer_at_e=0;
                }
                p.requested_animation_at_124=29;
                flags&=0xfaff;
                auto stop=call(rt,f,fp,0x424210,effect?0x495120:0x495190,{0x2ff});
                if(!stop)return stop;
                stop=call(rt,f,fp,0x424210,effect?0x49512a:0x49519a,{0x2fb});
                if(!stop)return stop;
                if(effect) {
                    if(!rt.globals.alternate_model_at_3b18db5)return fail(SonicPlayerFailure::unknown_field,0x49512f);
                    if(*rt.globals.alternate_model_at_3b18db5==0) {
                        stop=call(rt,f,fp,0x423d70,0x495146,{0x4cb,0,0,0});
                        if(!stop)return stop;
                    }
                }
                return done(1);
            }
            ;
            if(enhanced) {
                flags|=0x200;
                if(!rt.spin_release_effect_callback_4940e0)return fail(SonicPlayerFailure::missing_native_owner,0x4950b6,0x4940e0);
                if(!rt.spin_release_destructor_494120)return fail(SonicPlayerFailure::missing_native_owner,0x4950b6,0x494120);
                auto effect=rt.scheduler.create_element(8,0,rt.spin_release_effect_callback_4940e0);
                if(!effect&&effect.failure!=TaskFailure::source_pool_exhausted) {
                    auto error=fail(SonicPlayerFailure::native_owner_failed,0x4950b6,0x40b860);
                    error.task_creation=effect;
                    return error;
                }
                if(effect.task) {
                    auto arena=rt.scheduler.work_arena();
                    if(!arena)return fail(SonicPlayerFailure::missing_native_owner,0x4950c5);
                    if(!f.object||!f.object->primary_work)return fail(SonicPlayerFailure::unknown_field,0x4950c5);
                    auto write=arena->set_reference(effect.task->auxiliary_work,0,f.object->primary_work);
                    if(!write) {
                        auto error=fail(SonicPlayerFailure::work_binding_failed,0x4950c5);
                        error.work_failure=write.failure;
                        error.task_creation=effect;
                        return error;
                    }
                    if(!player(f)->player_work)return fail(SonicPlayerFailure::unknown_field,0x4950ca);
                    write=arena->set_reference(effect.task->auxiliary_work,4,player(f)->player_work);
                    if(!write) {
                        auto error=fail(SonicPlayerFailure::work_binding_failed,0x4950ca);
                        error.work_failure=write.failure;
                        error.task_creation=effect;
                        return error;
                    }
                    if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x4950cd);
                    const auto old=player(f)->parameters->max_x_spd;
                    write=arena->write(effect.task->auxiliary_work,8,&old,4);
                    if(!write) {
                        auto error=fail(SonicPlayerFailure::work_binding_failed,0x4950d6);
                        error.work_failure=write.failure;
                        error.task_creation=effect;
                        return error;
                    }
                    player(f)->parameters->max_x_spd=.5F;
                    effect.task->destructor=rt.spin_release_destructor_494120;
                }
                return cleanup(true);
            }
            if(timer>=300)return cleanup(false);
            auto held=pad_test(f,rt,true,0x909f94,0x4951ac,0x4951b4);
            if(!held)return held;
            if(!*held.value) {
                f.task->action_at_0=5;
                if(!p.source_charge_at_0)return fail(SonicPlayerFailure::unknown_field,0x4951c1);
                p.requested_animation_at_124=15;
                p.motion.speed.x=*p.source_charge_at_0;
                auto sound=call(rt,f,fp,0x423d70,0x4951d8,{0x300,0,0,0});
                if(!sound)return sound;
                sound=call(rt,f,fp,0x424210,0x4951e2,{0x2ff});
                if(!sound)return sound;
                sound=call(rt,f,fp,0x424210,0x4951ec,{0x2fb});
                if(!sound)return sound;
                p.timer_at_7e=0;
                p.source_charge_at_0=0;
                if(!(flags&0x100)) {
                    sound=call(rt,f,fp,0x424210,0x495216,{0x2fb});
                    if(!sound)return sound;
                }
                return done(1);
            }
            if(!p.source_charge_at_0)return fail(SonicPlayerFailure::unknown_field,0x495224);
            if(fp.compare_float_memory(fp.from_float(*p.source_charge_at_0),rt.constants.at(0x7dcb34))==X87Comparison::less)p.source_charge_at_0=fp.to_float(fp.add_float_memory(fp.from_float(*p.source_charge_at_0),rt.constants.at(0x7dd550)));
            if(!p.equipment_at_4)return fail(SonicPlayerFailure::unknown_field,0x49523d);
            SonicPlayerResult sound;
            if(*p.equipment_at_4&2) {
                if(flags&0x100)sound=call(rt,f,fp,0x423f50,0x49525f,{0x2fb,{},1,0,2},1);
                else sound=call(rt,f,fp,0x424210,0x49526f,{0x2fb});
            }
            else sound=call(rt,f,fp,0x423f50,0x495286,{0x2ff,{},1,0,2},1);
            if(!sound)return sound;
            return done(0);
            }
            case SonicPlayerPredicateEntry::stage_pickup_442680:{
            if(!rt.globals.stage_at_3b22dcc)return fail(SonicPlayerFailure::unknown_field,0x4146b0);
            auto stage=*rt.globals.stage_at_3b22dcc;
            if(stage<0x27||stage>0x2a)return done(0);
            auto held=pad_test(f,rt,true,0x909fac,0x44268a,0x44269c);
            if(!held||!*held.value)return held;
            auto h=native_hold(f,rt,fp,0x4426a6);
            if(!h||!*h.value)return h;
            auto form=predicate(SonicPlayerPredicateEntry::wall_form_43bfe0);
            if(!form)return form;
            const auto type=static_cast<std::int32_t>(*form.value);
            if(type<=0||type>4||!(*h.value&0x200))return done(0);
            auto found=call(rt,f,fp,0x41c0c0,0x4426e9,{std::nullopt,0x4000U-static_cast<std::uint32_t>(f.task->orientation.angles[1]),0x2000,0x92},0);
            if(!found)return found;
            if(!found.task_work_return)return fail(SonicPlayerFailure::native_owner_failed,0x4426f1,0x41c0c0);
            if(!*found.task_work_return)return done(0);
            if(!(*found.task_work_return)->position)return fail(SonicPlayerFailure::unknown_field,0x4426f5);
            auto y=fp.from_float((*found.task_work_return)->position->y);
            if(!f.task->position)return fail(SonicPlayerFailure::unknown_field,0x4426f8);
            auto delta=fp.subtract_float_memory(y,f.task->position->y);
            if(delta.negative())delta=delta.negated();
            auto c=fp.compare_float_memory(delta,rt.constants.at(0x7e00f4));
            return done(c==X87Comparison::greater?0:1);
            }
            case SonicPlayerPredicateEntry::stage_action_442570:{
            if(!rt.globals.stage_at_3b22dcc)return fail(SonicPlayerFailure::unknown_field,0x4146b0);
            auto stage=*rt.globals.stage_at_3b22dcc;
            if(stage<0x27||stage>0x2a)return done(0);
            if(!f.task->action_at_0||!argument)return fail(SonicPlayerFailure::unknown_field,0x442581);
            if(*f.task->action_at_0!=static_cast<std::uint8_t>(*argument))return done(0);
            auto pressed=pad_test(f,rt,false,0x909fa8,0x44258d,0x442593);
            if(!pressed||!*pressed.value)return pressed;
            f.task->action_at_0=static_cast<std::uint8_t>(*argument>>8);
            auto ready=require_player(f,0x4425aa);
            if(!ready)return ready;
            player(f)->player->requested_animation_at_124=static_cast<std::uint16_t>(*argument>>16);
            if(!rt.motion||!rt.motion->acceleration_runtime.input.current_character)return fail(SonicPlayerFailure::unknown_field,0x4144e0);
            auto character=*rt.motion->acceleration_runtime.input.current_character;
            const std::array<std::uint32_t,8> sounds{{0x307,0,0x308,0x31a,0,0x334,0,0x358}};
            const std::array<std::uint32_t,8> sites{{0x4425d1,0,0x4425ea,0x442603,0,0x44261c,0x442632,0x44264b}};
            if(character>7||character==1||character==4)return done(1);
            auto sound=character==6?call(rt,f,fp,0x423be0,sites[character],{1,0x5d,0,0x1f}):call(rt,f,fp,0x423d70,sites[character],{sounds[character],0,0,0});
            if(!sound)return sound;
            return done(1);
            }
            case SonicPlayerPredicateEntry::idle_494ff0:{
            auto active=input_active(f,rt,fp,0x494ffb);
            if(!active)return active;
            if(*active.value)return done(0);
            auto ready=require_player(f,0x495007);
            if(!ready)return ready;
            auto& p=*player(f)->player;
            if(fp.compare_float_memory(fp.from_float(p.motion.speed.x),rt.constants.at(0x7dcb2c))!=X87Comparison::equal)return done(0);
            if(flags&0x800) {
                auto held=native_hold(f,rt,fp,0x495028);
                if(!held)return held;
                f.task->action_at_0=(*held.value&0xc00)?24:23;
                p.requested_animation_at_124=(*held.value&0xc00)?54:37;
                return done(1);
            }
            f.task->action_at_0=1;
            p.idle_timer_at_e=0;
            auto animation=call(rt,f,fp,0x491660,0x49506b);
            if(!animation)return animation;
            return done(1);
            }
            case SonicPlayerPredicateEntry::turn_break_4429c0:{
            if(flags&0x4000) {
                if(!f.task->event_move||!f.task->event_move->mode)return fail(SonicPlayerFailure::unknown_field,0x4429db);
                if(*f.task->event_move->mode!=1)return done(0);
                const auto index=f.task->player_index;
                if(index>=rt.players.player_work.size()||!rt.players.player_work[index])return fail(SonicPlayerFailure::unknown_player_binding,0x4429e5);
                auto p=*rt.players.player_work[index];
                if(p) {
                    if(!p->player)return fail(SonicPlayerFailure::unknown_player_binding,0x4429f2);
                    p->player->motion.speed.z=0;
                    p->player->motion.speed.y=0;
                    p->player->motion.speed.x=0;
                }
                f.task->event_move->mode=0;
                flags&=0xbfff;
                return done(1);
            }
            if(flags&0x2000)return done(0);
            std::int32_t angle=0;
            auto active=input_active(f,rt,fp,0x442a1e,&angle);
            if(!active||!*active.value)return active;
            if(!active.input||!(active.input->outputs_written&1))return fail(SonicPlayerFailure::input_failed,0x442a2e,0x43bdc0);
            return done(angle_distance(f.task->orientation.angles[1],angle)>0x6000?1:0);
            }
            default:return fail(SonicPlayerFailure::unimplemented_action,static_cast<std::uint32_t>(entry));
        }
    }
    SonicPlayerResult sonic_action_checks(SonicPlayerFrame& f,SonicPlayerRuntime& rt,X87Context& fp)  {
        if(!f.task)return fail(SonicPlayerFailure::null_required_resource,0x496f59);
        if(!rt.globals.stage_at_3b22dcc)return fail(SonicPlayerFailure::unknown_field,0x4146b0);
        auto stage=*rt.globals.stage_at_3b22dcc;
        if(stage>=0x27&&stage<=0x2a)  {
            auto a=call(rt,f,fp,0x491550,0x496f70);
            if(!a)return a;
            if(!a.value)return fail(SonicPlayerFailure::native_owner_failed,0x496f7c,0x442720);
            a=call(rt,f,fp,0x442720,0x496f7c,{f.task->player_index,*a.value,{},{}});
            if(!a)return a;
        }
        if(!f.player)return fail(SonicPlayerFailure::unknown_player_binding,0x496f84);
        auto p=player(f);
        if(p&&!p->player)return fail(SonicPlayerFailure::null_required_resource,0x496f88);
        if(p&&!p->player->source_flags_at_6)return fail(SonicPlayerFailure::unknown_field,0x496f88);
        auto& flags=f.task->orientation.flags;
        if(p&&(*p->player->source_flags_at_6&0x8000))  {
            flags|=0x8000;
            f.task->control_timer_at_6=0;
        }
        else  {
            if(!f.task->control_timer_at_6)return fail(SonicPlayerFailure::unknown_field,0x496f9c);
            if(*f.task->control_timer_at_6)  {
                flags&=0xfffb;
                f.task->control_timer_at_6=static_cast<std::uint16_t>(*f.task->control_timer_at_6-1);
            }
            else  {
                if(!f.task->task_id_at_2)return fail(SonicPlayerFailure::unknown_field,0x492d70);
                const bool event=*f.task->task_id_at_2!=2&&((flags&4)||(p&&(p->player->attributes&0x10000)));
                if(event)  {
                    f.task->control_timer_at_6=120;
                    auto c=call(rt,f,fp,0x4bcc10,0x492da6,{f.task->player_index,7,0x3b,6});
                    if(!c)return c;
                    return fail(SonicPlayerFailure::unimplemented_action,0x492dab,0x492d70);
                }
                flags&=0x7fff;
            }
        }
        if(!f.task->action_at_0)return fail(SonicPlayerFailure::unknown_field,0x497017);
        auto index=static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(*f.task->action_at_0))-1);
        if(index>0x56)return done(index);
        const auto action=*f.task->action_at_0;
        auto pred=[&](SonicPlayerPredicateEntry e,std::optional<std::uint32_t> arg={}) {
            return sonic_action_predicate(e,f,rt,fp,arg);
        }
        ;
        auto common=[&](bool roll,bool hold,bool charge) {
            for(auto e:{SonicPlayerPredicateEntry::event_action_495fa0,SonicPlayerPredicateEntry::roll_release_492f90,SonicPlayerPredicateEntry::fall_494f70,SonicPlayerPredicateEntry::jump_495e60,SonicPlayerPredicateEntry::hold_493120,SonicPlayerPredicateEntry::charge_496ee0}) {
                if((e==SonicPlayerPredicateEntry::roll_release_492f90&&!roll)||(e==SonicPlayerPredicateEntry::hold_493120&&!hold)||(e==SonicPlayerPredicateEntry::charge_496ee0&&!charge))continue;
                auto r=pred(e);
                if(!r)return r;
                if(!r.value)return fail(SonicPlayerFailure::native_owner_failed,static_cast<std::uint32_t>(e));
                if(*r.value)return r;
            }
            return done(0);
        }
        ;
        if(action==1) {
            auto r=common(true,true,true);
            if(!r)return r;
            if(*r.value)return done();
            r=pred(SonicPlayerPredicateEntry::stage_pickup_442680);
            if(!r)return r;
            auto ready=require_player(f,0x4970d4);
            if(!ready)return ready;
            auto& state=*player(f)->player;
            if(*r.value==1) {
                f.task->action_at_0=71;
                state.requested_animation_at_124=130;
            }
            else {
                r=pred(SonicPlayerPredicateEntry::stage_action_442570,0x00854a01);
                if(!r)return r;
                if(*r.value==1)return done();
            }
            r=input_active(f,rt,fp,0x49710e);
            if(!r)return r;
            if(*r.value) {
                f.task->action_at_0=2;
                state.requested_animation_at_124=9;
                return done();
            }
            if(!state.contact_work||!state.contact_work->direction_at_1c)return fail(SonicPlayerFailure::unknown_field,0x497121);
            auto slope=fp.compare_float_memory(fp.from_float(state.contact_work->direction_at_1c->y),rt.constants.at(0x7e0844));
            if(slope!=X87Comparison::less)return done();
            auto length=rotation_vector_length(state.motion.speed,fp);
            if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x49713d);
            if(fp.compare_float_memory(length,player(f)->parameters->slide_speed)!=X87Comparison::greater)return done();
            f.task->action_at_0=19;
            state.requested_animation_at_124=31;
            return done();
        }
        if(action==2) {
            auto r=pred(SonicPlayerPredicateEntry::event_action_495fa0);
            if(!r)return r;
            if(*r.value)return done();
            r=pred(SonicPlayerPredicateEntry::roll_release_492f90);
            if(!r)return r;
            if(*r.value)return done();
            for(auto e:{SonicPlayerPredicateEntry::fall_494f70,SonicPlayerPredicateEntry::jump_495e60}) {
                r=pred(e);
                if(!r)return r;
                if(*r.value) {
                    flags&=0xdfff;
                    return done();
                }
            }
            r=pred(SonicPlayerPredicateEntry::hold_493120);
            if(!r)return r;
            if(*r.value)return done();
            for(auto e:{SonicPlayerPredicateEntry::charge_496ee0,SonicPlayerPredicateEntry::idle_494ff0}) {
                r=pred(e);
                if(!r)return r;
                if(*r.value) {
                    flags&=0xdfff;
                    return done();
                }
            }
            auto ready=require_player(f,0x49720c);
            if(!ready)return ready;
            auto& state=*player(f)->player;
            SonicPlayerResult error;
            auto own=work(f,error,0x49720f);
            if(!own)return error;
            if(!own->flags)return fail(SonicPlayerFailure::unknown_field,0x49720f);
            if((*own->flags&0x10)&&!(flags&4)) {
                if(!own->my_info_index)return fail(SonicPlayerFailure::unknown_field,0x497223);
                if(!own->hit_work)return fail(SonicPlayerFailure::unknown_field,0x49722a);
                if(!own->hit_work->id)return fail(SonicPlayerFailure::null_required_resource,0x497230);
                auto hit=f.collision->works.find(*own->hit_work->id);
                if(hit==f.collision->works.end())return fail(SonicPlayerFailure::unresolved_resource,0x497230);
                auto info=infos(f,hit->second,error,0x497230);
                if(!info)return error;
                auto record=info->records.find(*own->my_info_index);
                if(record==info->records.end())return fail(SonicPlayerFailure::unresolved_resource,0x497239);
                if(!record->second.attributes)return fail(SonicPlayerFailure::unknown_field,0x497239);
                if(!(*record->second.attributes&0x2000)&&(*own->flags&4)) {
                    r=call(rt,f,fp,0x43c060,0x497247);
                    if(!r)return r;
                    if(!r.value)return fail(SonicPlayerFailure::native_owner_failed,0x49724f,0x43c060);
                    if(*r.value==4) {
                        f.task->action_at_0=15;
                        state.requested_animation_at_124=21;
                        flags&=0xdfff;
                    }
                }
            }
            r=pred(SonicPlayerPredicateEntry::turn_break_4429c0);
            if(!r)return r;
            if(!*r.value)return done();
            if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x49727a);
            auto speed=fp.compare_float_memory(fp.from_float(state.motion.speed.x),player(f)->parameters->jog_speed);
            if(speed==X87Comparison::less||speed==X87Comparison::unordered)return done();
            f.task->action_at_0=13;
            state.requested_animation_at_124=20;
            if(!rt.globals.alternate_model_at_3b18db5)return fail(SonicPlayerFailure::unknown_field,0x49729a);
            const auto alternate=*rt.globals.alternate_model_at_3b18db5!=0;
            r=call(rt,f,fp,0x423d70,alternate?0x4972ac:0x4972c2,{alternate?0x308U:0x12U,0,0,0});
            if(!r)return r;
            flags&=0xdfff;
            return done();
        }
        if(action==3) {
            flags&=0xfaff;
            auto r=pred(SonicPlayerPredicateEntry::event_action_495fa0);
            if(!r)return r;
            if(*r.value)return done();
            for(auto e:{SonicPlayerPredicateEntry::roll_release_492f90,SonicPlayerPredicateEntry::fall_494f70,SonicPlayerPredicateEntry::jump_495e60}) {
                r=pred(e);
                if(!r)return r;
                if(*r.value) {
                    flags&=0xdfff;
                    return done();
                }
            }
            auto ready=require_player(f,0x497330);
            if(!ready)return ready;
            if(!player(f)->parameters)return fail(SonicPlayerFailure::unknown_field,0x497333);
            auto& state=*player(f)->player;
            if(fp.compare_float_memory(fp.from_float(state.motion.speed.x),player(f)->parameters->jog_speed)==X87Comparison::less) {
                f.task->action_at_0=2;
                state.requested_animation_at_124=9;
                flags&=0xdfff;
                return done();
            }
            r=pred(SonicPlayerPredicateEntry::idle_494ff0);
            if(!r)return r;
            if(*r.value)flags&=0xdfff;
            return done();
        }
        if(action==4) {
            auto r=pred(SonicPlayerPredicateEntry::event_action_495fa0);
            if(!r)return r;
            if(!*r.value) {
                r=pred(SonicPlayerPredicateEntry::fall_494f70);
                if(!r)return r;
                if(!*r.value) {
                    r=pred(SonicPlayerPredicateEntry::charge_tick_495080);
                    if(!r)return r;
                    return done();
                }
                r=pred(SonicPlayerPredicateEntry::request_spin_animation_495c30);
                if(!r)return r;
                auto ready=require_player(f,0x4973a7);
                if(!ready)return ready;
                player(f)->player->source_charge_at_0=5;
                flags|=0x500;
            }
            r=call(rt,f,fp,0x424210,0x4973b6,{0x2ff});
            if(!r)return r;
            r=call(rt,f,fp,0x424210,0x4973c0,{0x2fb});
            if(!r)return r;
            return done();
        }
        if(action==5) {
            auto r=pred(SonicPlayerPredicateEntry::event_action_495fa0);
            if(!r)return r;
            if(*r.value)return done();
            r=pred(SonicPlayerPredicateEntry::fall_494f70);
            if(!r)return r;
            if(*r.value) {
                r=pred(SonicPlayerPredicateEntry::request_spin_animation_495c30);
                if(!r)return r;
                auto ready=require_player(f,0x497418);
                if(!ready)return ready;
                player(f)->player->source_charge_at_0=5;
                flags=static_cast<std::uint16_t>((flags&0xdfff)|0x500);
                return done();
            }
            r=pred(SonicPlayerPredicateEntry::jump_495e60);
            if(!r)return r;
            if(*r.value) {
                flags&=0xdfff;
                return done();
            }
            r=pred(SonicPlayerPredicateEntry::skid_end_4930d0);
            if(!r)return r;
            if(!*r.value) {
                r=pad_test(f,rt,false,0x909f94,0x497468,0x497470);
                if(!r)return r;
                if(!*r.value)return done();
                auto ready=require_player(f,0x497480);
                if(!ready)return ready;
                f.task->action_at_0=2;
                player(f)->player->requested_animation_at_124=29;
            }
            flags&=0xfaff;
            return done();
        }
        return fail(SonicPlayerFailure::unimplemented_action,0x497025,0x496f50);
    }
    SonicPlayerResult dispatch_sonic_player_action(SonicPlayerFrame& f,SonicPlayerRuntime& rt,X87Context& fp)  {
        if(!f.task||!f.world)return fail(SonicPlayerFailure::null_required_resource,0x49ab1d);
        if(!f.task->action_at_0)return fail(SonicPlayerFailure::unknown_field,0x49ab1d);
        auto a=*f.task->action_at_0;
        auto target=sonic_action_target(a);
        SonicPlayerResult r;
        std::optional<PlayerPositionResult> movement_position;
        std::optional<PlayerAnimationResult> animation_diagnostics;
        auto finish=[&](SonicPlayerResult out)  {
            if(!out.position&&movement_position)out.position=movement_position;
            if(!out.animation&&animation_diagnostics)out.animation=animation_diagnostics;
            out.dispatched_action=a;
            out.dispatch_target=target?*target:0x49be04;
            return out;
        }
        ;
        if(!target||*target==0x49be04)  {
            r=call(rt,f,fp,0x44a800,0x49be0b);
            if(r.animation)animation_diagnostics=r.animation;
            if(!r)return finish(r);
            return finish(fail(SonicPlayerFailure::unimplemented_action,0x49be10));
        }
        if(a==0)  {
            f.task->character_id=0;
            if(!rt.startup_runtime)return finish(fail(SonicPlayerFailure::missing_native_owner,0x49ab3e,0x442750));
            auto startup=initialize_player_startup(f.task->player_index,f.object,f.world,rt.scheduler,rt.players,*rt.startup_runtime,fp);
            if(startup.partial)f.player=startup.partial;
            if(!startup)  {
                r=fail(SonicPlayerFailure::startup_failed,0x49ab3e,0x442750);
                r.startup=startup;
                return finish(r);
            }
            // The actual immediate names the table initialized by7cec90.
            if(!rt.initialized_animation_table)return finish(fail(SonicPlayerFailure::unresolved_resource,0x49ab47));
            if(!*rt.initialized_animation_table)return finish(fail(SonicPlayerFailure::null_required_resource,0x49ab47));
            player(f)->player->animation_tail.table_at_140=*rt.initialized_animation_table;
            player(f)->player->requested_animation_at_124=0;
            if(!rt.globals.alternate_model_at_3b18db5)return finish(fail(SonicPlayerFailure::unknown_field,0x49ab58));
            // ___SONIC_OBJECTS graph mutation/join-vertex resources are not yet bound.
            return finish(fail(SonicPlayerFailure::unresolved_resource,*rt.globals.alternate_model_at_3b18db5?0x49abb0:0x49ab64));
        }
        auto ready=require_player(f,*target);
        if(!ready)return finish(ready);
        if(a!=1&&a!=2&&a!=9&&a!=11&&a!=23&&a!=30&&a!=72&&a!=73&&a!=74&&a!=75&&a!=80)return finish(fail(SonicPlayerFailure::unimplemented_action,*target));
        if(!rt.motion)return finish(fail(SonicPlayerFailure::missing_native_owner,*target));
        auto& p=*player(f)->player;
        if(a==1||a==23||a==72||a==73||a==74||a==75)  {
            r=sonic_ground_input(f,*rt.motion,rt.constants,fp);
            if(!r)return finish(r);
            r=movement(f,rt,fp,true,false,false,a==1?0x49ad3c:0x49b258,a==1?0x49ad44:0x49bdd6,a==1?0x49ad4c:0x49bdde);
            if(r.position)movement_position=r.position;
            if(!r)return finish(r);
            r=sonic_copy_local_speed(f,*rt.motion,rt.constants,fp);
            if(!r)return finish(r);
            if(a==1)  {
                r=call(rt,f,fp,0x491660,0x49ad5c);
                if(!r)return finish(r);
            }
        }
        else if(a==2)  {
            if(!p.previous_animation_at_128)return finish(fail(SonicPlayerFailure::unknown_field,0x49ad6e));
            if(*p.previous_animation_at_128==30)  {
                if(!p.current_animation_at_126)return finish(fail(SonicPlayerFailure::unknown_field,0x49ad77));
                if(*p.current_animation_at_126!=30)  {
                    if(!player(f)->parameters)return finish(fail(SonicPlayerFailure::unknown_field,0x49ad80));
                    p.motion.speed.x=fp.to_float(fp.add_float_memory(fp.from_float(player(f)->parameters->jog_speed),p.motion.speed.x));
                }
            }
            r=movement(f,rt,fp,true,false,false,0x49ad8f,0x49ad97,0x49ad9f);
            if(r.position)movement_position=r.position;
            if(!r)return finish(r);
            if(*r.value==2)  {
                if(f.task->player_index==0)  {
                    if(!rt.globals.alternate_model_at_3b18db5)return finish(fail(SonicPlayerFailure::unknown_field,0x49adb3));
                    if(*rt.globals.alternate_model_at_3b18db5==0)  {
                        r=call(rt,f,fp,0x423d70,0x49adc7,{0x4c6,0,0,0});
                        if(!r)return finish(r);
                    }
                }
                f.task->action_at_0=18;
                p.requested_animation_at_124=27;
                r=call(rt,f,fp,0x440790,0x49addc);
                if(!r)return finish(r);
                p.motion.speed.x=fp.to_float(fp.multiply_float_memory(fp.from_float(p.motion.speed.x),rt.constants.at(0x7e01a8)));
                p.motion.speed.z=fp.to_float(fp.multiply_float_memory(fp.from_float(p.motion.speed.z),rt.constants.at(0x7e01a8)));
            }
            else  {
                r=sonic_copy_local_speed(f,*rt.motion,rt.constants,fp);
                if(!r)return finish(r);
                r=call(rt,f,fp,0x495cd0,0x49adfa);
                if(!r)return finish(r);
            }
        }
        else if(a==9||a==30||a==80||a==11)  {
            r=movement(f,rt,fp,false,a!=11,a==11,a==11?0x49afa2:0x49af74,0x49b08e,0x49b096);
            if(r.position)movement_position=r.position;
            if(!r)return finish(r);
            r=sonic_copy_local_speed(f,*rt.motion,rt.constants,fp);
            if(!r)return finish(r);
        }
        else return finish(fail(SonicPlayerFailure::unimplemented_action,*target));
        r=call(rt,f,fp,0x44a800,0x49be0b);
            if(r.animation)animation_diagnostics=r.animation;
        if(!r)return finish(r);
        // No fabricated model-bank mutations or omitted common tail.
        return finish(fail(SonicPlayerFailure::unimplemented_action,0x49be10));
    }
    SonicPlayerResult run_sonic_player(SonicPlayerFrame& f,SonicPlayerRuntime& rt,X87Context& fp)  {
        if(!f.object)return fail(SonicPlayerFailure::null_required_resource,0x49a9b8);
        if(!f.task)return fail(SonicPlayerFailure::unknown_field,0x49a9b8);
        if(!f.world)return fail(SonicPlayerFailure::unknown_motion_binding,0x49a9c0);
        if(!f.player)return fail(SonicPlayerFailure::unknown_player_binding,0x49a9c0);
        rt.globals.current_task_at_3abdf60=f.object->primary_work;
        rt.globals.current_player_at_3abdf64=player(f)?player(f)->player_work:TaskWorkHandle{};
        auto& flags=f.task->orientation.flags;
        if((flags&0x20)&&!gt_unordered(fp.compare_float_memory(fp.from_float(f.world->motion.speed.y),rt.constants.at(0x7dcb2c))))  {
            flags|=2;
            SonicPlayerResult r;
            auto w=work(f,r,0x49a9f7);
            if(!w)return r;
            if(!w->owning_task)return fail(SonicPlayerFailure::unknown_field,0x49a9f7);
            if(!w->owning_task->id)return fail(SonicPlayerFailure::null_required_resource,0x49a9fd);
            auto t=f.collision->tasks.find(*w->owning_task->id);
            if(t==f.collision->tasks.end())return fail(SonicPlayerFailure::unresolved_resource,0x49a9fd);
            if(!t->second.work)return fail(SonicPlayerFailure::unknown_field,0x49a9fd);
            if(!t->second.work->id)return fail(SonicPlayerFailure::null_required_resource,0x49aa00);
            auto actor=f.collision->actors.find(*t->second.work->id);
            if(actor==f.collision->actors.end())return fail(SonicPlayerFailure::unresolved_resource,0x49aa00);
            auto x=position_actor_angle(actor->second,0);
            if(!x)return fail(SonicPlayerFailure::unknown_field,0x49aa00);
            f.task->orientation.angles[0]=*x;
            auto z=position_actor_angle(actor->second,2);
            if(!z)return fail(SonicPlayerFailure::unknown_field,0x49aa0f);
            f.task->orientation.angles[2]=*z;
        }
        else flags&=0xfffd;
        if(!rt.globals.forced_at_3b29d0c)return fail(SonicPlayerFailure::unknown_field,0x4258f0);
        if(!rt.globals.forced_at_3b29d08||!*rt.globals.forced_at_3b29d08)return fail(SonicPlayerFailure::unknown_field,0x4258f7);
        const auto forced=static_cast<std::int32_t>(*rt.globals.forced_at_3b29d0c)|static_cast<std::int32_t>(static_cast<std::int16_t>(**rt.globals.forced_at_3b29d08));
        if(forced)f.task->action_at_0=87;
        else  {
            if(!f.task->action_at_0)return fail(SonicPlayerFailure::unknown_field,0x49aa2c);
            if(*f.task->action_at_0!=86)  {
                auto r=sonic_action_checks(f,rt,fp);
                if(!r)return r;
            }
        }
        if(!f.collision||!f.collision->task_collision)return fail(SonicPlayerFailure::unknown_field,0x49aa3c);
        if(f.collision->task_collision->id)  {
            SonicPlayerResult r;
            auto w=work(f,r,0x49aa48);
            if(!w)return r;
            auto info=infos(f,*w,r,0x49aa48);
            if(!info)return r;
            auto ready=require_player(f,0x49aa50);
            if(!ready)return ready;
            auto& p=*player(f)->player;
            if(!p.source_flags_at_6)return fail(SonicPlayerFailure::unknown_field,0x49aa50);
            std::uint8_t low=0,high=0;
            if(*p.source_flags_at_6&0x8000)low=high=3;
            else  {
                if(!p.equipment_at_4)return fail(SonicPlayerFailure::unknown_field,0x49aa5f);
                if(*p.equipment_at_4&0x8000)low=high=3;
                else  {
                    auto a=*f.task->action_at_0;
                    if(a==44)  {
                        low=2;
                        high=3;
                    }
                    else if(a==6)  {
                        low=1;
                        high=3;
                    }
                    else if(flags&0x100)low=high=1;
                    else if(a==45)high=1;
                }
            }
            auto first=info->records.find(0);
            if(first==info->records.end())return fail(SonicPlayerFailure::unresolved_resource,0x49aab9);
            if(!first->second.damage)return fail(SonicPlayerFailure::unknown_field,0x49aab9);
            first->second.damage=static_cast<std::uint8_t>((*first->second.damage&0xf0)|(high<<2)|low);
            auto second=info->records.find(1);
            if(second==info->records.end())return fail(SonicPlayerFailure::unresolved_resource,0x49aad9);
            if(!second->second.attributes)return fail(SonicPlayerFailure::unknown_field,0x49aad9);
            second->second.attributes=*f.task->action_at_0==72?(*second->second.attributes&~0x10u):(*second->second.attributes|0x10u);
        }
        if(flags&0x100)  {
            auto ready=require_player(f,0x49ab01);
            if(!ready)return ready;
            auto& timer=player(f)->player->timer_at_7e;
            if(!timer)return fail(SonicPlayerFailure::unknown_field,0x49ab01);
            timer=static_cast<std::uint16_t>(*timer+1);
        }
        auto action=static_cast<std::int8_t>(*f.task->action_at_0);
        rt.globals.range_at_3c55d45=action>=75&&action<=82?1:0;
        return dispatch_sonic_player_action(f,rt,fp);
    }
}
// namespace sadx
