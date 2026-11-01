#include "player/sonic_animation_table.hpp"
#include <cstring>
namespace {
float real(std::uint32_t x) { float y; std::memcpy(&y,&x,4); return y; }
std::uint32_t bits(float x) { std::uint32_t y; std::memcpy(&y,&x,4); return y; }
}
// Input: 149 export words; 147 seven-word old rows (mask + six fields);
// then two invocations of missing,size,null,source-state. Both keep the same table.
extern "C" int final_table(const std::uint32_t* in, std::uint32_t* out) {
    using namespace sadx;
    PlayerAnimationTable table;
    for(unsigned i=0;i<147;++i) {
        const auto* p=in+149+i*7; const auto mask=p[0];
        if(mask==64)continue;
        auto& r=table.records[i];
        if(mask&1)r.action=AnimationResourceReference{p[1]};
        if(mask&2)r.object_count=static_cast<std::uint8_t>(p[2]);
        if(mask&4)r.mode=static_cast<std::int8_t>(p[3]);
        if(mask&8)r.next=static_cast<std::uint16_t>(p[4]);
        if(mask&16)r.transition_step=real(p[5]);
        if(mask&32)r.frame_step=real(p[6]);
    }
    for(unsigned pass=0;pass<2;++pass) {
        const auto* p=in+1178+pass*4;
        auto exports=std::make_shared<SonicAnimationExport>(p[1]);
        for(unsigned i=0;i<p[1];++i) {
            exports->at(i)=AnimationResourceReference{in[i]};
            if(i==p[2])exports->at(i)=AnimationResourceReference{};
            if(i==p[0])exports->at(i).reset();
        }
        std::optional<std::shared_ptr<const SonicAnimationExport>> source=exports;
        if(p[3]==1)source.reset();
        if(p[3]==2)source=std::shared_ptr<const SonicAnimationExport>{};
        const auto result=initialize_sonic_animation_table(table,source);
        *out++=static_cast<unsigned>(result.failure); *out++=result.source_boundary; *out++=result.completed_records;
        for(unsigned i=0;i<147;++i) {
            auto it=table.records.find(i); const PlayerAnimationRecord empty;
            const auto& r=it==table.records.end()?empty:it->second;
            *out++=it!=table.records.end();
            *out++=r.action?(r.action->id?2:1):0; *out++=r.action&&r.action->id?*r.action->id:0;
            *out++=r.object_count.has_value(); *out++=r.object_count.value_or(0);
            *out++=r.mode.has_value(); *out++=static_cast<std::uint8_t>(r.mode.value_or(0));
            *out++=r.next.has_value(); *out++=r.next.value_or(0);
            *out++=r.transition_step.has_value(); *out++=bits(r.transition_step.value_or(0));
            *out++=r.frame_step.has_value(); *out++=bits(r.frame_step.value_or(0));
        }
    }
    return 0;
}
