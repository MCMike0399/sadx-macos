#include "player/player_path.hpp"
#include "cpu/crt_integer.hpp"
#include <cstring>
namespace sadx {
std::optional<PlayerPathConstants> decode_player_path_constants(const std::uint8_t* b,std::size_t size){
    if(!b||size!=92)return {};
    const auto floor=decode_crt_floor_constants(b+12,80);if(!floor)return {};
    PlayerPathConstants out;std::memcpy(&out.query.scalar.zero,b,4);std::memcpy(&out.query.scalar.one,b+4,4);
    std::memcpy(&out.query.range_factor,b+8,4);out.floor=*floor;return out;
}
namespace {
std::int32_t signed_angle_delta(std::int32_t a,std::int32_t b){
    const auto d=(static_cast<std::uint32_t>(b)-static_cast<std::uint32_t>(a))&0xffffU;
    return d<0x8000U?static_cast<std::int32_t>(d):static_cast<std::int32_t>(d)-65536;
}
std::int16_t word(std::uint32_t value){const auto low=static_cast<std::uint16_t>(value);std::int16_t result;std::memcpy(&result,&low,2);return result;}
std::int32_t signed_word(std::uint32_t value){std::int32_t result;std::memcpy(&result,&value,4);return result;}
std::int16_t midpoint(std::int16_t a,std::int16_t b){const auto delta=signed_angle_delta(a,b);const auto half=delta>=0?delta/2:(delta-1)/2;return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(half));}
const PlayerPathPoint* point(const PlayerPathResource& path,std::int32_t index){
    return index>=0&&static_cast<std::size_t>(index)<path.points.size()?&path.points[static_cast<std::size_t>(index)]:nullptr;
}
X87Comparison compare(float a,float b,X87Context& fp){return fp.compare_float_memory(fp.from_float(a),b);}
std::int32_t interpolate_angle(std::int16_t a,std::int16_t b,float ratio,X87Context& fp){
    const auto scaled=fp.multiply_float_memory(fp.from_int64(signed_angle_delta(a,b)),ratio);
    return signed_word(static_cast<std::uint32_t>(a)+crt_ftol_low32(scaled,fp));
}
void store_direction(PathVectorFacts& out,std::int32_t a,std::int32_t b,const NinjaMath& math,X87Context& fp){
    const float sine_b=math.sin(b);const auto cosine_a=math.cos_extended(a);
    out[0]=fp.to_float(fp.multiply_float_memory(cosine_a,sine_b).negated());
    const float cosine_b=math.cos(b);
    out[1]=fp.to_float(fp.multiply_float_memory(math.cos_extended(a),cosine_b));
    out[2]=math.sin(a);
}
}
PathSegmentResult path_segment_endpoints(const PlayerPathResource& path,std::int32_t index){
    const auto* current=point(path,index);if(!current)return {PathQueryFailure::point_outside_storage,{},0x49c22d,index};
    auto previous=signed_word(static_cast<std::uint32_t>(index)-1U),next=signed_word(static_cast<std::uint32_t>(index)+1U);
    if(path.type==1){if(previous<0)previous=static_cast<std::int32_t>(path.point_count)-1;
        else if(next>=path.point_count)next=0;}
    else if(path.type==0){if(previous<0)previous=0;else if(next>=path.point_count)next=static_cast<std::int32_t>(path.point_count)-1;}
    const auto* before=point(path,previous);if(!before)return {PathQueryFailure::point_outside_storage,{},0x49c27c,previous};
    const auto* after=point(path,next);if(!after)return {PathQueryFailure::point_outside_storage,{},0x49c28e,next};
    return {PathQueryFailure::none,PathSegmentEndpoints{{midpoint(before->source_angle0,current->source_angle0),midpoint(before->source_angle2,current->source_angle2)},
        {midpoint(current->source_angle0,after->source_angle0),midpoint(current->source_angle2,after->source_angle2)},current->position,after->position},0,{}};
}
PathQueryResult query_player_path(const PlayerPathResource& path,PlayerPathStatus& status,
    const PathQueryConstants& c,const CrtFloorConstants& floor_constants,const CrtFloorRuntime& floor_runtime,
    const NinjaMath& math,X87Context& fp){
    PathQueryResult result;
    if(path.type==1){
        const float period=path.total_length,input_distance=status.distance;
        // Actual4393c0: period-zero short circuit, FSTP64 ratio argument,
        // source floor, then period product and reverse m32 subtraction.
        if(compare(period,c.scalar.zero,fp)==X87Comparison::equal)
            status.distance=fp.to_float(fp.from_float(c.scalar.zero));
        else{
            const auto ratio=fp.divide_float_memory(fp.from_float(input_distance),period);
            const double argument=fp.to_double(ratio);
            result.floor_argument=argument;
            const auto floored=crt_floor(argument,floor_constants,floor_runtime,fp);
            if(!floored||!floored.value){result.failure=PathQueryFailure::floor_unavailable;result.source_boundary=0x4393e6;result.floor=floored;return result;}
            const auto turns=fp.multiply_float_memory(*floored.value,period);
            status.distance=fp.to_float(fp.reverse_subtract_float_memory(turns,input_distance));
        }
    }else if(path.type==0){
        if(compare(status.distance,c.scalar.zero,fp)==X87Comparison::less){result.source_return=false;return result;}
        const auto distance=fp.multiply_float_memory(fp.from_float(status.distance),c.range_factor);
        const auto total=fp.multiply_float_memory(fp.from_float(path.total_length),c.range_factor);
        if(fp.compare(total,distance)==X87Comparison::less){result.source_return=false;return result;}
    }
    std::int32_t selected=0;float accumulated=0.0F,segment=0;bool segment_known=false;
    for(;selected<path.point_count;++selected){
        const auto* p=point(path,selected);
        if(!p){result.failure=PathQueryFailure::point_outside_storage;result.source_boundary=0x49c3a2;result.unavailable_index=selected;result.selected_index=selected;result.accumulated_distance=accumulated;return result;}
        segment=compare(p->segment_length,c.scalar.zero,fp)==X87Comparison::equal?1.0F:p->segment_length;segment_known=true;
        const auto total=fp.add_float_memory(fp.from_float(segment),accumulated);
        if(fp.compare_float_memory(total,status.distance)==X87Comparison::greater)break;
        accumulated=fp.to_float(total);
    }
    result.selected_index=selected;result.accumulated_distance=accumulated;
    const auto endpoints=path_segment_endpoints(path,selected);
    if(endpoints.failure!=PathQueryFailure::none){result.failure=endpoints.failure;result.source_boundary=endpoints.source_boundary;result.unavailable_index=endpoints.unavailable_index;return result;}
    const auto& e=*endpoints.endpoints;const auto* current=point(path,selected);
    const auto difference=fp.subtract_float_memory(fp.from_float(status.distance),accumulated);
    // Count0 never assigned the source scratch length. The load/subtract above
    // still execute before that actual uninitialized FDIV memory boundary.
    if(!segment_known){result.failure=PathQueryFailure::uninitialized_segment_length;result.source_boundary=0x49c3ff;return result;}
    const auto ratio_extended=fp.divide_float_memory(difference,segment);
    status.angle0=current->source_angle0;status.angle4=current->source_angle2;
    const float ratio=fp.to_float(ratio_extended);
    status.angle8=interpolate_angle(e.begin_angles[0],e.end_angles[0],ratio,fp);
    status.anglec=interpolate_angle(e.begin_angles[1],e.end_angles[1],ratio,fp);
    const float dx=fp.to_float(fp.subtract_float_memory(fp.from_float(e.end_position.x),e.begin_position.x));
    const float dy=fp.to_float(fp.subtract_float_memory(fp.from_float(e.end_position.y),e.begin_position.y));
    const float dz=fp.to_float(fp.subtract_float_memory(fp.from_float(e.end_position.z),e.begin_position.z));
    const auto position=[&](float delta,float base){return fp.add_float_memory(fp.multiply_float_memory(fp.from_float(delta),ratio),base);};
    const auto px=position(dx,e.begin_position.x);status.position_at20[0]=fp.to_float(px);
    const auto py=position(dy,e.begin_position.y);status.position_at20[1]=fp.to_float(py);
    const auto pz=position(dz,e.begin_position.z);const float z=fp.to_float(pz);
    status.position_at20[2]=fp.to_float(pz);status.position_at14[2]=z;
    status.position_at14[0]=fp.to_float(px);status.position_at14[1]=fp.to_float(py);
    store_direction(status.direction_at2c,*status.angle0,*status.angle4,math,fp);
    store_direction(status.direction_at38,*status.angle8,*status.anglec,math,fp);
    const auto zz=fp.multiply_float_memory(fp.from_float(dz),dz);
    const auto yy=fp.multiply_float_memory(fp.from_float(dy),dy);
    const auto zy=fp.add(zz,yy);const auto xx=fp.multiply_float_memory(fp.from_float(dx),dx);
    const auto length=fp.sqrt(fp.add(zy,xx));
    if(fp.compare_float_memory(length,c.scalar.zero)==X87Comparison::equal){status.tangent_at44={0.0F,0.0F,0.0F};}
    else{
        const auto inverse=fp.reverse_divide_float_memory(length,c.scalar.one);
        status.tangent_at44[0]=fp.to_float(fp.multiply(fp.from_float(dx),inverse));
        status.tangent_at44[1]=fp.to_float(fp.multiply(fp.from_float(dy),inverse));
        status.tangent_at44[2]=fp.to_float(fp.multiply_float_memory(inverse,dz));
    }
    result.source_return=true;return result;
}
} // namespace sadx
