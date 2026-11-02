#pragma once
#include <cstdint>
#include <map>
#include <stdexcept>

namespace sadx::reference {
// NOTSA: stable real host objects represent guest identities, never resources.
// No token is dereferenced or passed to a native allocator. Identity preservation
// through aliases does not establish allocation ownership, generations or lifetime.
class ResetBufferTokens {
    struct Token {};
    std::map<std::uint32_t, Token> tokens_;
  public:
    void *host(std::uint32_t identity) {
        return identity == 0 ? nullptr : &tokens_[identity];
    }
    std::uint32_t guest(const void *token) const {
        if (token == nullptr)
            return 0;
        for (const auto &entry : tokens_)
            if (&entry.second == token)
                return entry.first;
        throw std::invalid_argument("foreign reset buffer token");
    }
};
} // namespace sadx::reference
