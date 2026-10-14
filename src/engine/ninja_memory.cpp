#include "engine/ninja_memory.hpp"
namespace sadx {
NinjaMemoryResult nj_malloc_780f60(NinjaMemoryState& s,const NinjaMemoryMethods& methods,std::uint32_t size){
    NinjaMemoryResult r;
    if(!s.heap_at_389d628){r.failure=NinjaMemoryFailure::unknown_state;r.source_boundary=0x780f60;return r;}
    if(*s.heap_at_389d628==0xffffffffu){
        const auto handle=methods.create?methods.create():std::nullopt;
        if(!handle){r.failure=NinjaMemoryFailure::host_unavailable;r.source_boundary=0x780f73;return r;}
        s.heap_at_389d628=*handle;r.created=true;
    }
    const auto block=methods.allocate?methods.allocate(*s.heap_at_389d628,size):std::nullopt;
    if(!block){r.failure=NinjaMemoryFailure::host_unavailable;r.source_boundary=0x780f86;return r;}
    r.eax=*block;return r;
}
NinjaMemoryResult nj_free_780fb0(NinjaMemoryState& s,const NinjaMemoryMethods& methods,std::uint32_t address){
    NinjaMemoryResult r;if(address==0)return r;
    if(!s.heap_at_389d628){r.failure=NinjaMemoryFailure::unknown_state;r.source_boundary=0x780fb9;return r;}
    const auto receipt=methods.release?methods.release(*s.heap_at_389d628,address):std::nullopt;
    if(!receipt){r.failure=NinjaMemoryFailure::host_unavailable;r.source_boundary=0x780fc1;return r;}
    r.eax=*receipt;return r;
}
namespace {
NinjaMemoryMethods native_methods(GuestHeap& heap){
    return {[&heap]{return std::optional<std::uint32_t>(heap.create());},
        [&heap](std::uint32_t handle,std::uint32_t size){return std::optional<std::uint32_t>(heap.alloc(handle,size).value_or(0));},
        [&heap](std::uint32_t handle,std::uint32_t address){return std::optional<std::uint32_t>(heap.free(handle,address)?1U:0U);}};
}
}
NinjaMemoryResult nj_malloc_780f60(NinjaMemoryState& s,GuestHeap& heap,std::uint32_t size){return nj_malloc_780f60(s,native_methods(heap),size);}
NinjaMemoryResult nj_free_780fb0(NinjaMemoryState& s,GuestHeap& heap,std::uint32_t address){return nj_free_780fb0(s,native_methods(heap),address);}
} // namespace sadx
