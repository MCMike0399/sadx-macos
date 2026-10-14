#include "engine/task.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sadx {
class TaskSourceProbe {
    static constexpr std::uint32_t base=0x20000020;
    static std::uint32_t address(const TaskHandle& t) {return t && t->source_slot_?base+*t->source_slot_*52:0;}
    static std::uint32_t callback(TaskCallback c) {
        if(c==&TaskScheduler::source_no_op)return 0x51f680;
        if(c==&TaskScheduler::destroy_callback)return 0x40b570;
        if(c==&TaskScheduler::free_children_callback)return 0x40b7e0;
        return 0;
    }
public:
    static int run(const std::uint32_t* commands,std::size_t words,std::uint8_t* memory,
        std::size_t memory_size,std::uint32_t* trace,std::size_t trace_size) {
        if(!commands||words%4||!memory||memory_size!=TaskWorkArena::usable_bytes+32||!trace||trace_size!=words/4*3+11)return 1;
        TaskScheduler scheduler;auto arena=std::make_shared<TaskWorkArena>(0x20000000);
        auto resource=std::make_shared<int>(1);
        if(!scheduler.initialize_source_pool(arena,resource))return 2;
        std::vector<TaskHandle> allocations;
        std::array<TaskHandle,4096> last_generation{};
        for(std::size_t i=0;i<words;i+=4) {
            // Exercise the production interchange before each source call, as
            // the guest adapter does after external callbacks mutate its graph.
            if(!scheduler.restore_source_graph(scheduler.source_graph()))return 6;
            for(auto& task:allocations)if(task&&!task->retired_)task=scheduler.source_slots_[*task->source_slot_];
            for(auto& task:last_generation)if(task&&!task->retired_)task=scheduler.source_slots_[*task->source_slot_];
            const auto op=commands[i],a=commands[i+1],b=commands[i+2],c=commands[i+3];
            TaskResult result;
            if(op==0)result=scheduler.allocate_task(static_cast<int>(a),nullptr);
            else if(op==1)result=scheduler.create_element(a,static_cast<int>(b),nullptr);
            else if(op==2 && b<allocations.size())result=scheduler.create_child(a,nullptr,allocations[b]);
            else if(a<allocations.size()) {
                auto t=allocations[a];
                if(op==3)result=scheduler.destroy_task(t);
                else if(op==4)result=scheduler.free_task(t);
                else if(op==6)result=scheduler.free_children(t);
                else if(op==7)t->exec=&TaskScheduler::free_children_callback;
                else if(op==8) {auto written=arena->write(t->primary_work,b,&c,4);if(!written)return 4;}
                else if(op==9) {
                    auto collision=arena->allocate(32).work,nested=arena->allocate(16).work;
                    auto object=arena->allocate(80).work;
                    auto first=arena->allocate(20).work,second=arena->allocate(20).work;
                    auto extra=arena->allocate(8).work,last=arena->allocate(16).work;
                    if(!collision||!nested||!object||!first||!second||!extra||!last)return 4;
                    for(auto work:{collision,object,first,second,last})
                        if(!arena->fill(work,0,0,work->requested_size()))return 4;
                    const std::uint8_t flag=0x80;
                    if(!arena->write(collision,5,&flag,1)||!arena->set_reference(collision,12,nested)||
                       !arena->set_reference(t->primary_work,56,collision)||
                       !arena->set_reference(t->primary_work,60,object)||
                       !arena->set_reference(first,16,second)||!arena->set_reference(object,64,first)||
                       !arena->set_reference(object,68,extra)||!arena->set_reference(object,76,last))return 4;
                }
                else if(op!=5)return 3;
            }else if(op!=5)return 3;
            if(op==5)result=scheduler.run_index(static_cast<int>(a));
            if(op<=2) {
                allocations.push_back(result.task);
                if(result.task)last_generation[*result.task->source_slot_]=result.task;
            }
            trace[i/4*3]=op<=2 && result ? address(result.task):0;
            trace[i/4*3+1]=static_cast<std::uint32_t>(result.failure);
            trace[i/4*3+2]=scheduler.free_slots_.empty()?0:base+scheduler.free_slots_.back()*52;
        }
        std::memcpy(memory,arena->source_bytes().data(),memory_size);
        // Native tasks contain owned references and function pointers. Encode
        // them only in this explicit test fixture's canonical source address
        // scheme; this is not a functional guest memory layer.
        auto put=[&](std::uint32_t at,std::uint32_t value){std::memcpy(memory+at,&value,4);};
        for(std::size_t slot=0;slot<4096;++slot) {
            const auto& task=last_generation[slot];const auto start=32+slot*52;
            for(unsigned j=0;j<13;++j)put(static_cast<std::uint32_t>(start+j*4),0);
            if(task) {
                put(start+48,task->field30?0x30000000:0);
                if(!task->retired_) {
                    if(!task->linked_ && task->destruction_stage_==Task::DestructionStage::live) {
                        // AllocateTask7 deliberately retains the popped free
                        // node's old next pointer; its isolated node is not
                        // a native active-list edge.
                        put(start,task->detached_free_next_slot_?base+*task->detached_free_next_slot_*52:0);
                    }else put(start,address(scheduler.retain(task->next_)));
                    put(start+4,address(scheduler.retain(task->previous_)));
                    put(start+8,address(scheduler.retain(task->parent_)));
                    put(start+12,address(scheduler.retain(task->first_child_)));
                    put(start+16,callback(task->exec));put(start+20,callback(task->display));put(start+24,callback(task->destructor));
                    const TaskWorkHandle work[]={task->primary_work,task->motion_work,task->force_work,task->auxiliary_work};
                    for(unsigned j=0;j<4;++j)put(start+32+j*4,work[j]?0x20000000+work[j]->data_offset():0);
                }
            }
        }
        for(std::size_t i=0;i<scheduler.free_slots_.size();++i) {
            const auto slot=scheduler.free_slots_[i];const auto start=32+slot*52;
            put(start,i?base+scheduler.free_slots_[i-1]*52:0);
            put(start+4,i+1<scheduler.free_slots_.size()?base+scheduler.free_slots_[i+1]*52:0);
        }
        const auto at=words/4*3;
        for(unsigned i=0;i<8;++i)trace[at+i]=address(scheduler.retain(scheduler.heads_[i]));
        trace[at+8]=callback(scheduler.context_.current_function);
        trace[at+9]=scheduler.context_.field30?0x30000000:0;
        trace[at+10]=arena->free_head()?0x20000000+*arena->free_head():0;
        return 0;
    }
};
}
extern "C" int sadx_native_task_lifecycle(const std::uint32_t* commands,std::size_t words,
    std::uint8_t* memory,std::size_t memory_size,std::uint32_t* trace,std::size_t trace_size) {
    try{return sadx::TaskSourceProbe::run(commands,words,memory,memory_size,trace,trace_size);}catch(...){return 5;}
}
extern "C" int sadx_native_task_inactive_boundary(std::uint32_t* output,std::size_t size) {
    if(!output||size!=4)return 1;
    try {
        sadx::TaskScheduler scheduler;
        if(!scheduler.initialize_source_pool(std::make_shared<sadx::TaskWorkArena>(),std::shared_ptr<void>{}))return 2;
        auto task=scheduler.create_element(2,7,nullptr).task;
        auto result=scheduler.destroy_task(task);
        output[0]=static_cast<std::uint32_t>(result.failure);output[1]=result.source_boundary;
        output[2]=task->retired()?1:0;output[3]=task->primary_work && !task->primary_work->released()?1:0;
        return 0;
    }catch(...){return 3;}
}
