#include "engine/texture_list_gate.hpp"
#include "engine/native_texture_lifetime.hpp"

namespace sadx::texture_selection {
bool list_not_ready(const texture_lifetime::List *list) {
    if (!list || list->entries.empty())
        return true;
    for (const auto *entry : list->entries)
        if (!entry)
            return true;
    return list->first_filename_invalid;
}
} // namespace sadx::texture_selection
