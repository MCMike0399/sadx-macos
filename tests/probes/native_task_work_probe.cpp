#include "game/task_work.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" int sadx_native_task_work(const std::uint32_t* commands,std::size_t words,
    std::uint8_t* memory,std::size_t memory_size,std::uint32_t* trace,std::size_t trace_size) {
    if(!commands||words%2||!memory||memory_size!=sadx::TaskWorkArena::usable_bytes+32||!trace||trace_size!=words/2*3)return 1;
    try {
        sadx::TaskWorkArena arena(0x20000000);
        std::vector<sadx::TaskWorkHandle> allocations;
        for(std::size_t i=0;i<words;i+=2) {
            sadx::TaskWorkResult result;
            if(commands[i]==0){result=arena.allocate(commands[i+1]);allocations.push_back(result.work);}
            else if(commands[i]==1 && commands[i+1]<allocations.size())result=arena.release(allocations[commands[i+1]]);
            else return 2;
            const auto at=i/2*3;
            trace[at]=commands[i]==0 && result.work ? 0x20000000+result.work->data_offset():0;
            trace[at+1]=static_cast<std::uint32_t>(result.failure);
            trace[at+2]=arena.free_head()?0x20000000+*arena.free_head():0;
        }
        std::memcpy(memory,arena.source_bytes().data(),memory_size);
        return 0;
    }catch(...){return 3;}
}
