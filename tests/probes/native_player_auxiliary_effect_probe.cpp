#include "player/player_auxiliary_effect.hpp"
#include <cstring>
#include <vector>
namespace {
std::uint32_t address(const sadx::TaskWorkHandle& w){return w?0x20000000+w->data_offset():0;}
std::uint32_t callback(sadx::TaskCallback f){
    if(!f)return 0;
    if(f==&sadx::player_auxiliary_effect_initialize_callback)return 0x4a2b60;
    if(f==&sadx::player_auxiliary_effect_update_callback)return 0x4a2570;
    if(f==&sadx::player_auxiliary_effect_display_callback)return 0x4a0760;
    if(f==&sadx::player_auxiliary_effect_destroy_callback)return 0x4a0720;
    if(f==&sadx::TaskScheduler::destroy_callback)return 0x40b570;
    if(f==&sadx::TaskScheduler::source_no_op)return 0x51f680;
    return 0xffffffff;
}
void put(std::uint8_t* p,std::uint32_t w){for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(w>>(i*8));}
}
// Explicit seeded owner fixtures:16 LE words,16 output words, complete arena.
// Source addresses are only provenance bytes for this comparison, never
// functional pointers in the native production task/work owners.
extern "C" int sadx_native_player_auxiliary_effect(const std::uint8_t* table,std::size_t table_size,
    const std::uint32_t* w,std::size_t words,std::uint16_t cw,
    std::uint32_t* out,std::size_t count,std::uint8_t* memory,std::size_t memory_size){
    if(!table||table_size!=0x438||!w||words!=16||!out||count!=16||!memory||memory_size!=sadx::TaskWorkArena::usable_bytes+32||w[6]>4)return 1;
    try{
        auto arena=std::make_shared<sadx::TaskWorkArena>(0x20000000);sadx::TaskScheduler scheduler;
        if(!scheduler.initialize_source_pool(arena,std::shared_ptr<void>{}))return 2;
        auto task=scheduler.create_element(2,2,&sadx::player_auxiliary_effect_initialize_callback).task;if(!task)return 2;
        const auto primary=task->primary_work;std::array<std::uint8_t,64> seed{};seed.fill(static_cast<std::uint8_t>(w[15]));
        if(!arena->write(primary,0,seed.data(),64))return 3;
        sadx::PlayerStartupRegistry registry;
        if(w[1]==1)registry.task_work[0]=std::shared_ptr<sadx::PlayerTaskState>{};
        else if(w[1]==2){auto player=std::make_shared<sadx::PlayerTaskState>();player->character_id=static_cast<std::uint8_t>(w[2]);if(w[12])player->secondary_mode_at_1=static_cast<std::uint8_t>(w[13]);registry.task_work[0]=std::move(player);}
        sadx::X87Context fp(cw);sadx::PlayerAuxiliaryEffectRuntime runtime;runtime.players=&registry;runtime.fp=&fp;
        std::optional<std::uint16_t> character;if(w[3])character=static_cast<std::uint16_t>(w[4]);runtime.current_character_at_3b22dc0=&character;
        if(w[5])runtime.table=sadx::decode_player_auxiliary_effect_table(table,table_size);
        std::vector<sadx::TaskWorkHandle> nodes;
        for(unsigned i=0;i<w[6];++i){auto n=arena->allocate(52).work;if(!n||!arena->fill(n,0,0,52))return 3;nodes.push_back(n);}
        if(w[0]!=0){
            if(!arena->set_reference(primary,0x10,nodes.empty()?sadx::TaskWorkHandle{}:nodes[0]))return 3;
            for(unsigned i=0;i<nodes.size();++i)if(!arena->set_reference(nodes[i],0x2c,i?nodes[i-1]:sadx::TaskWorkHandle{})||!arena->set_reference(nodes[i],0x30,i+1<nodes.size()?nodes[i+1]:sadx::TaskWorkHandle{}))return 3;
        }
        if(w[0]==2){
            task->auxiliary_effect_work=std::make_shared<sadx::PlayerAuxiliaryEffectTaskWork>();task->auxiliary_effect_work->allocation=primary;
            if(w[10]==1)task->auxiliary_effect_work->selection_at_8=std::shared_ptr<const sadx::PlayerAuxiliaryEffectSelection>{};
            else if(w[10]==2){auto selected=std::make_shared<sadx::PlayerAuxiliaryEffectSelection>();selected->table=runtime.table;selected->group=w[11];task->auxiliary_effect_work->selection_at_8=selected;}
            const auto secondary=static_cast<std::uint8_t>(w[8]),variant=static_cast<std::uint8_t>(w[9]);
            if(!arena->write(primary,0x34,w+7,4)||!arena->write(primary,1,&secondary,1)||!arena->write(primary,3,&variant,1))return 3;
            const auto ptr=w[10]==2?0x9275a0+w[11]*216:0;if(!arena->write(primary,8,&ptr,4))return 3;
        }
        if(!w[14])task->primary_work.reset();
        sadx::PlayerAuxiliaryEffectResult result;
        if(w[0]==0)result=sadx::initialize_player_auxiliary_effect(scheduler,task,runtime);
        else if(w[0]==1)result=sadx::destroy_player_auxiliary_effect(scheduler,task);
        else if(w[0]==2)result=sadx::update_player_auxiliary_effect(scheduler,task,runtime);
        else return 1;
        std::memset(out,0,16*4);std::memcpy(memory,arena->source_bytes().data(),memory_size);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;out[2]=result.nested_source_boundary;out[3]=static_cast<std::uint32_t>(result.work_failure);
        out[4]=callback(task->exec);out[5]=callback(task->destructor);out[6]=callback(task->display);out[7]=address(primary);
        if(task->auxiliary_effect_work && task->auxiliary_effect_work->selection_at_8){out[8]=*task->auxiliary_effect_work->selection_at_8?2:1;if(*task->auxiliary_effect_work->selection_at_8){out[9]=static_cast<std::uint32_t>((*task->auxiliary_effect_work->selection_at_8)->group);put(memory+primary->data_offset()+8,0x9275a0+out[9]*216);}}
        out[10]=arena->free_head()?0x20000000+*arena->free_head():0;out[11]=fp.control_word();out[12]=fp.exception_flags();
        for(unsigned i=0;i<nodes.size();++i)if(nodes[i]->released())out[13]|=1U<<i;
        out[14]=task->auxiliary_effect_work?1:0;out[15]=task->primary_state?1:0;return 0;
    }catch(const sadx::X87Error&){return 4;}
}
