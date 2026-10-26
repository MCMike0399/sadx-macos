#include "engine/texture_list_gate.hpp"
#include "engine/native_texture_lifetime.hpp"
#include <cassert>

int main() {
    using namespace sadx;
    assert(texture_selection::list_not_ready(nullptr));
    texture_lifetime::List list;
    assert(texture_selection::list_not_ready(&list));
    list.first_filename_invalid = true;
    assert(texture_selection::list_not_ready(&list));

    texture_lifetime::Memory memory;
    // Readiness tests the binding alone, even after its surface was released.
    assert(!memory.surface);
    list.entries.assign(9, &memory);
    assert(texture_selection::list_not_ready(&list));
    list.first_filename_invalid = false;
    assert(!texture_selection::list_not_ready(&list));
    for (auto &entry : list.entries) {
        entry = nullptr;
        assert(texture_selection::list_not_ready(&list));
        entry = &memory;
    }
    assert(!texture_selection::list_not_ready(&list));
    assert(!list.first_filename_invalid && list.entries.size() == 9);
    for (const auto *entry : list.entries)
        assert(entry == &memory);
    assert(!memory.surface);
}
