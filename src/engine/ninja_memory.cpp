#include "engine/ninja_memory.hpp"
namespace sadx {
NinjaMemoryResult nj_malloc_780f60(NinjaMemoryState& s,GuestHeap& heap,std::uint32_t size){
    NinjaMemoryResult r;
    if(!s.heap_at_389d628){r.failure=NinjaMemoryFailure::unknown_state;r.source_boundary=0x780f60;return r;}
    if(*s.heap_at_389d628==0xffffffffu){s.heap_at_389d628=heap.create();r.created=true;}
    r.eax=heap.alloc(*s.heap_at_389d628,size).value_or(0);return r;
}
NinjaMemoryResult nj_free_780fb0(NinjaMemoryState& s,GuestHeap& heap,std::uint32_t address){
    NinjaMemoryResult r;if(address==0)return r;
    if(!s.heap_at_389d628){r.failure=NinjaMemoryFailure::unknown_state;r.source_boundary=0x780fb9;return r;}
    heap.free(*s.heap_at_389d628,address); // HeapFree's result is ignored
    return r;
}
} // namespace sadx
