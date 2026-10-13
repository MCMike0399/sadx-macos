#include "player/sonic_animation_table.hpp"
#include <cassert>
#include <iostream>
int main(){using namespace sadx;PlayerAnimationTable table;
    auto missing=initialize_sonic_animation_table(table,{});assert(!missing&&missing.source_boundary==0x7cec90&&table.records.empty());
    auto null=initialize_sonic_animation_table(table,std::shared_ptr<const SonicAnimationExport>{});assert(!null&&null.failure==PlayerAnimationFailure::null_resource&&null.source_boundary==0x7cec95&&table.records.empty());
    auto source=std::make_shared<SonicAnimationExport>(149);for(std::size_t i=0;i<source->size();++i)source->at(i)=AnimationResourceReference{7000+i};
    source->at(122).reset();auto partial=initialize_sonic_animation_table(table,std::shared_ptr<const SonicAnimationExport>(source));assert(!partial&&partial.completed_records==1&&partial.source_boundary==0x7cecd2&&table.records.size()==1);auto& first=table.records.at(0);assert(first.action->id==7001&&first.object_count==73&&first.mode==3&&first.next==0&&first.transition_step==0.0625F&&first.frame_step==0.5F);
    source->at(122)=AnimationResourceReference{};auto initialized=initialize_sonic_animation_table(table,std::shared_ptr<const SonicAnimationExport>(source));assert(initialized&&initialized.completed_records==147&&table.records.size()==147);assert(table.records.at(1).action&&!table.records.at(1).action->id);assert(table.records.at(75).action&&!table.records.at(75).action->id);assert(table.records.at(76).action&&!table.records.at(76).action->id);
    auto identity=table.records.at(146).action;source->at(123).reset();auto later=initialize_sonic_animation_table(table,std::shared_ptr<const SonicAnimationExport>(source));assert(!later&&later.source_boundary==0x7ced0c&&later.completed_records==2);assert(table.records.at(146).action->id==identity->id);
    std::cout<<"Sonic animation initializer preserved actual instruction write order, late unknowns, observed nulls and147 records\n";
}
