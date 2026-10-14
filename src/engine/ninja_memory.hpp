#pragma once
#include <optional>
#include "loader/guest_heap.hpp"
#include <cstdint>
#include <functional>
namespace sadx {
// njMalloc 780f60 / njFree 780fb0: a lazily HeapCreate'd Win32 heap whose
// handle lives in 389d628 (-1 until created). HeapCreate(0, 0x800000, 0)
// requests an 8 MiB initial heap; the native GuestHeap does not model reservation.
struct NinjaMemoryState {std::optional<std::uint32_t> heap_at_389d628;};
// Platform methods return the original DWORD, including zero on ordinary failure.
// nullopt means the platform call could not complete; it is not an allocation failure.
struct NinjaMemoryMethods {
    std::function<std::optional<std::uint32_t>()> create;
    std::function<std::optional<std::uint32_t>(std::uint32_t,std::uint32_t)> allocate,release;
};
enum class NinjaMemoryFailure {none,unknown_state,host_unavailable};
struct NinjaMemoryResult {
    NinjaMemoryFailure failure=NinjaMemoryFailure::none;std::uint32_t source_boundary=0;
    std::uint32_t eax=0; // njMalloc: block; njFree: raw HeapFree result (0 for null address)
    bool created=false;   // this call created the heap
    explicit operator bool()const noexcept{return failure==NinjaMemoryFailure::none;}
};
NinjaMemoryResult nj_malloc_780f60(NinjaMemoryState&,GuestHeap&,std::uint32_t size);
NinjaMemoryResult nj_free_780fb0(NinjaMemoryState&,GuestHeap&,std::uint32_t address);
NinjaMemoryResult nj_malloc_780f60(NinjaMemoryState&,const NinjaMemoryMethods&,std::uint32_t size);
NinjaMemoryResult nj_free_780fb0(NinjaMemoryState&,const NinjaMemoryMethods&,std::uint32_t address);
} // namespace sadx
