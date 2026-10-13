#pragma once
#include "guest/x86_guest.hpp"
#include <cstdint>
#include <map>
#include <optional>
namespace sadx {
// Guest-address memory the HLE hands out (HeapAlloc, malloc, VirtualAlloc, LocalAlloc…) in one region of
// the guest address space, mapped in 1 MiB steps as it grows. First-fit over freed blocks, 16-byte
// alignment, every block zero-filled when handed out. The reference's heap addresses and the contents of
// a fresh Win32 block are not captured: these are native identities, and zero contents are an assumption
// (a fresh Windows heap page reads zero; a reused block would keep old bytes).
class HleMemory {
public:
    explicit HleMemory(X86Guest& guest,std::uint32_t base=0x40000000U,std::uint32_t limit=0x58000000U);
    std::optional<std::uint32_t> allocate(std::uint32_t size,std::uint32_t alignment=16);
    bool free(std::uint32_t address);
    std::optional<std::uint32_t> size(std::uint32_t address)const;
    std::optional<std::uint32_t> reallocate(std::uint32_t address,std::uint32_t size);
    std::size_t live_blocks()const noexcept{return live_.size();}
    bool contains(std::uint32_t address)const noexcept{return address>=base_&&address<top_;}
private:
    X86Guest& guest_;std::uint32_t base_,limit_,top_,mapped_;
    std::map<std::uint32_t,std::uint32_t> live_,free_; // address -> size
};
} // namespace sadx
