#pragma once
#include <optional>
#include "game/guest_heap.hpp"
#include <cstdint>
namespace sadx {
// njMalloc 780f60 / njFree 780fb0: a lazily HeapCreate'd Win32 heap whose
// handle lives in 389d628 (-1 until created). HeapCreate(0, &"LastWater003", 0)
// passes a string address as the initial size; the native GuestHeap ignores it.
struct NinjaMemoryState {std::optional<std::uint32_t> heap_at_389d628;};
enum class NinjaMemoryFailure {none,unknown_state};
struct NinjaMemoryResult {
    NinjaMemoryFailure failure=NinjaMemoryFailure::none;std::uint32_t source_boundary=0;
    std::uint32_t eax=0; // njMalloc: the block (0 when HeapAlloc fails)
    bool created=false;   // this call created the heap
    explicit operator bool()const noexcept{return failure==NinjaMemoryFailure::none;}
};
NinjaMemoryResult nj_malloc_780f60(NinjaMemoryState&,GuestHeap&,std::uint32_t size);
NinjaMemoryResult nj_free_780fb0(NinjaMemoryState&,GuestHeap&,std::uint32_t address);
} // namespace sadx
