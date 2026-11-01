#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace sadx {
// Reference-owned stand-in for the Win32 heap used by Ninja allocation and CRT.
// Tokens and block addresses are not captured Windows identities. Fresh bytes
// remain indeterminate until written; zero-byte requests receive one backed byte.
class GuestHeap {
  public:
    explicit GuestHeap(std::uint32_t handle_token = 0x00510000U,
                       std::uint32_t first_address = 0x50000000U);
    std::uint32_t create(); // One heap: repeated creation returns the same token.
    std::optional<std::uint32_t> alloc(std::uint32_t handle, std::uint32_t size);
    bool free(std::uint32_t handle, std::uint32_t address);
    std::optional<std::uint8_t> read(std::uint32_t address) const;
    bool write(std::uint32_t address, std::uint8_t value);
    std::size_t live_blocks() const noexcept { return blocks_.size(); }

    // Compatibility trace: C(handle,0), A(requested size,address or failure0),
    // F(address,0). The optional allocation result distinguishes a valid zero token.
    struct Call {
        char kind;
        std::uint32_t a, b;
    };
    std::vector<Call> trace;

  private:
    struct Block {
        std::uint32_t requested_size;
        std::vector<std::optional<std::uint8_t>> bytes;
    };
    std::uint32_t handle_token_, next_address_;
    std::map<std::uint32_t, Block> blocks_;
    const Block *find(std::uint32_t address, std::uint32_t &offset) const;
};
} // namespace sadx
