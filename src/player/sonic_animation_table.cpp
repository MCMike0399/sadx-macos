#include "player/sonic_animation_table.hpp"
#include "player/sonic_animation_operations.hpp"
#include <cstring>
#include <stdexcept>
namespace sadx {
SonicAnimationTableResult initialize_sonic_animation_table(PlayerAnimationTable& table,
    const std::optional<std::shared_ptr<const SonicAnimationExport>>& source){
    SonicAnimationTableResult result;
    if(!source){result.failure=PlayerAnimationFailure::unknown_field;result.source_boundary=0x7cec90;return result;}
    std::array<AnimationResourceReference,145> loaded;
    std::array<std::uint8_t,147> field_masks{};
    for(const auto& op:sonic_animation_operations){
        if(op.read){
            if(!*source){result.failure=PlayerAnimationFailure::null_resource;result.source_boundary=op.address;return result;}
            if(op.index>=(*source)->size()||!(*source)->at(op.index)){result.failure=PlayerAnimationFailure::unknown_field;result.source_boundary=op.address;return result;}
            loaded.at(op.slot)=*(*source)->at(op.index);continue;
        }
        auto& row=table.records[op.record];unsigned bit;
        switch(op.field){
            case 0:row.action=op.slot==65535?AnimationResourceReference{}:loaded.at(op.slot);bit=0;break;
            case 4:row.object_count=static_cast<std::uint8_t>(op.bits);bit=1;break;
            case 5:{const auto raw=static_cast<std::uint8_t>(op.bits);std::int8_t mode;std::memcpy(&mode,&raw,1);row.mode=mode;bit=2;break;}
            case 6:row.next=static_cast<std::uint16_t>(op.bits);bit=3;break;
            case 8:{float value;std::memcpy(&value,&op.bits,4);row.transition_step=value;bit=4;break;}
            case 12:{float value;std::memcpy(&value,&op.bits,4);row.frame_step=value;bit=5;break;}
            default:throw std::logic_error("Invalid generated source animation operation");
        }
        field_masks[op.record]|=static_cast<std::uint8_t>(1U<<bit);
        if(field_masks[op.record]==63){++result.completed_records;field_masks[op.record]=255;}
    }
    return result;
}
}
