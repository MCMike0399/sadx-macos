#include "engine/native_texture_lifetime.hpp"
#include "engine/texture_list_gate.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

// Flat fixture: present, count, first filename is sentinel, then count binding
// identities (0 null, 1..3 real Memory objects). These are validated import facts.
extern "C" int sadx_native_texture_list_gate(const std::uint32_t *words, std::size_t size,
                                             std::uint32_t *out) {
    if (!words || !out || size < 3 || words[0] > 1 || words[2] > 1 || words[1] > 128 ||
        size != 3 + words[1])
        return 0;
    std::array<sadx::texture_lifetime::Memory, 3> bindings{};
    sadx::texture_lifetime::List list;
    list.first_filename_invalid = words[2] != 0;
    for (std::size_t i = 3; i < size; ++i) {
        if (words[i] > bindings.size())
            return 0;
        list.entries.push_back(words[i] ? &bindings[words[i] - 1] : nullptr);
    }
    const auto entries = list.entries;
    const auto sentinel = list.first_filename_invalid;
    out[0] = sadx::texture_selection::list_not_ready(words[0] ? &list : nullptr);
    out[1] = list.entries == entries && list.first_filename_invalid == sentinel;
    return 1;
}
