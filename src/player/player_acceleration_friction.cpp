#include "player/player_acceleration_friction.hpp"

namespace sadx {
void finish_player_ground_friction(NinjaVector& gravity,float drive,float extra_y,
    bool active,PlayerMotionState& world,PlayerPhysicsState& player,const PlayerParameters& p,
    const GroundAccelerationConstants& constants,LongitudinalFriction mode,X87Context& fp) {
    const auto load=[&](float value){return fp.from_float(value);};
    const auto c=[&](std::uint32_t address){return constants.at(address);};
    const auto compare=[&](X87Value a,X87Value b){return fp.compare(a,b);};
    const auto cm=[&](X87Value a,float b){return fp.compare_float_memory(a,b);};
    const auto cf=[&](float a,float b){return cm(load(a),b);};
    const auto greater=[](X87Comparison v){return v==X87Comparison::greater;};
    const auto less=[](X87Comparison v){return v==X87Comparison::less;};
    const auto equal=[](X87Comparison v){return v==X87Comparison::equal;};
    const auto parity=[](X87Comparison value,unsigned mask){
        auto word=static_cast<unsigned>(x87_comparison_status(value)>>8U)&mask;
        unsigned ones=0;while(word){ones+=word&1U;word>>=1U;}return (ones&1U)==0;
    };
    const auto mul=[&](X87Value a,float b){return fp.multiply_float_memory(a,b);};
    const auto update=[&](float& dst,X87Value value){dst=fp.to_float(value);};
    const auto zero=c(0x7dcb2c);float extra_z=0;
    // Independent X friction/drive and Z friction use stored Y acceleration.
    const auto friction=[&](float coefficient){return mul(mul(load(coefficient),world.friction_scale),gravity.y);};
    float threshold=0;
    if(mode==LongitudinalFriction::apply) {
        if(equal(cf(player.motion.speed.x,zero))) {
            threshold=fp.to_float(friction(p.lim_frict));
            update(gravity.x,fp.add_float_memory(load(gravity.x),drive));
            if(!active) {
                const bool contained=less(cf(gravity.x,threshold)) && less(cm(load(threshold).negated(),gravity.x));
                const bool small=less(cf(gravity.x,c(0x7e014c))) && greater(cf(gravity.x,c(0x7e0148)));
                if(contained || small)gravity.x=0;
            }
        } else if(less(cf(drive,zero))) {
            if((player.attributes & 4U)!=0) {
                bool clamped=false;
                if(less(cf(gravity.y,zero))) {
                    const auto wide=friction(p.grd_frict);threshold=fp.to_float(wide);
                    if(greater(cm(wide,zero)) && greater(cm(load(drive).negated(),threshold))) {
                        auto sum=fp.add_float_memory(load(gravity.x),drive);
                        const auto negative=fp.to_float(load(threshold).negated());
                        if(less(cm(sum,zero)) && less(cm(sum,negative))) {
                            world.source_field_38=fp.to_float(fp.subtract_float_memory(sum,threshold));sum=load(negative);
                        } else if(greater(cm(sum,zero)) && greater(cm(sum,threshold))) {
                            world.source_field_38=fp.to_float(fp.subtract_float_memory(sum,threshold));sum=load(threshold);
                        }
                        update(gravity.x,sum);clamped=true;
                    }
                }
                if(!clamped) {
                    if(!active && !parity(cf(player.motion.speed.x,p.jog_speed),0x41) &&
                       less(cf(gravity.x,c(0x7e014c))) && greater(cf(gravity.x,c(0x7e0148))))gravity.x=0;
                    else update(gravity.x,fp.add_float_memory(load(gravity.x),drive));
                }
            } else if(greater(cf(player.motion.speed.x,zero))) {
                auto sum=fp.add_float_memory(load(gravity.x),drive);
                if(less(cm(mul(sum,drive),zero)))sum=load(zero);update(gravity.x,sum);
            } else if(!active && !parity(cf(player.motion.speed.x,p.jog_speed),0x41) &&
                      less(cf(gravity.x,c(0x7e014c))) && greater(cf(gravity.x,c(0x7e0148))))gravity.x=0;
            else {
                auto sum=fp.add_float_memory(friction(p.lim_frict),drive);
                if(!less(cm(sum,zero)) && cm(sum,zero)!=X87Comparison::unordered)sum=load(zero);
                update(gravity.x,sum);
            }
        } else {
            threshold=fp.to_float(friction(p.lim_frict));
            if(less(cf(player.motion.speed.x,zero)) && less(cf(gravity.x,c(0x7e014c))) && greater(cf(gravity.x,c(0x7e0148)))) {
                auto sum=fp.add_float_memory(load(gravity.x),drive);gravity.x=fp.to_float(sum);
                if(less(cm(mul(sum,drive),zero)))gravity.x=0;
            } else if(greater(cf(threshold,zero)) && less(cf(threshold,drive))) {
                auto sum=fp.add_float_memory(load(gravity.x),drive);
                if(greater(cm(sum,threshold))) {
                    world.source_field_38=fp.to_float(fp.subtract_float_memory(sum,threshold));sum=load(threshold);
                }
                update(gravity.x,sum);
            } else if(!active && !parity(cf(player.motion.speed.x,p.jog_speed),0x41) &&
                      less(cf(gravity.x,c(0x7e014c))) && greater(cf(gravity.x,c(0x7e0148))))gravity.x=0;
            else update(gravity.x,fp.add_float_memory(load(gravity.x),drive));
        }

    }
        if(equal(cf(player.motion.speed.z,zero))) {
            const auto wide=friction(p.lim_frict);
            if(less(compare(load(gravity.z),wide)) && less(cm(wide.negated(),gravity.z)))gravity.z=0;
        } else {
            threshold=less(cf(gravity.y,zero)) ? fp.to_float(friction(p.grd_frict_z)) : 0;
            auto z=load(gravity.z);
            if((player.attributes & 4U)!=0) {
                if(greater(cm(z,zero))) {
                    z=fp.subtract_float_memory(z,threshold);if(less(cm(z,zero)))z=load(zero);
                } else if(less(cm(z,zero))) {
                    z=fp.add_float_memory(z,threshold);if(greater(cm(z,zero)))z=load(zero);
                }
                update(gravity.z,z);
            } else {
                if(greater(cm(z,zero)))extra_z=fp.to_float(load(threshold).negated());
                else if(less(cf(gravity.z,zero)))extra_z=threshold;
                const auto summed=fp.add_float_memory(load(gravity.z),extra_z);
                if(!equal(cf(gravity.z,zero)) && !equal(cf(extra_z,zero)) &&
                   less(cm(mul(summed,gravity.z),zero)))update(gravity.z,load(zero));
                else update(gravity.z,summed);
            }
        }
    // FADD Y precedes MOV X; FSTP32 Y follows it. An arithmetic failure here
    // must precede X's store, while a final binary32-store failure retains X.
    const auto final_y=fp.add_float_memory(load(gravity.y),extra_y);
    player.motion.acceleration.x=gravity.x;
    gravity.y=fp.to_float(final_y);
    player.motion.acceleration.y=gravity.y;player.motion.acceleration.z=gravity.z;
}
} // namespace sadx
