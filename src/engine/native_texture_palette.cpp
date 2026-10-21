#include "engine/native_texture_palette.hpp"
namespace sadx::texture_palette {
State::State() {
    for (std::uint32_t index = 0; index < entries.size(); ++index)
        entries[index].slot = index;
}
void select_bank(std::int32_t bank, State &state) {
    for (const auto &entry : state.entries) {
        if (entry.bank == bank) {
            state.current_colors = entry.colors;
            return;
        }
    }
}
} // namespace sadx::texture_palette
