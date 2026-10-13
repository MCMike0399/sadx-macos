#pragma once
#include <optional>
#include <cstdint>
#include <map>
#include <vector>
namespace sadx {
// Native owner of the Win32 heap behind njMalloc 780f60 / njFree 780fb0 (and
// any other guest HeapCreate/HeapAlloc user). The handle and block addresses
// are guest-visible 32-bit values the source stores and compares, but the
// reference's were not captured: this owner issues native identities (a
// handle token, then 8-byte-aligned addresses from `first_address`), backs each
// block with native bytes, and keeps HeapAlloc's indeterminate contents as
// unknown (an unwritten byte reads as nullopt).
class GuestHeap {
public:
    explicit GuestHeap(std::uint32_t handle_token=0x00510000u,std::uint32_t first_address=0x50000000u);
    std::uint32_t create();                                   // HeapCreate: the same token each call (one heap)
    std::optional<std::uint32_t> alloc(std::uint32_t handle,std::uint32_t size); // HeapAlloc(handle, 0, size); 0 size gives a 1-byte block
    bool free(std::uint32_t handle,std::uint32_t address);    // HeapFree(handle, 0, address)
    std::optional<std::uint8_t> read(std::uint32_t address)const;
    bool write(std::uint32_t address,std::uint8_t value);
    std::size_t live_blocks()const noexcept{return blocks_.size();}
    // Calls in order: {'C', handle}, {'A', size, address (0 = failed)}, {'F', address}.
    struct Call {char kind;std::uint32_t a,b;};
    std::vector<Call> trace;
private:
    struct Block {std::uint32_t size;std::vector<std::optional<std::uint8_t>> bytes;};
    std::uint32_t handle_,next_;std::map<std::uint32_t,Block> blocks_;
    const Block* find(std::uint32_t address,std::uint32_t& offset)const;
};
} // namespace sadx
