// NOTSA: the append guard must verify loaded prefix bytes, not declared metadata.
#include "apps/native_character_selection_assets.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char **argv) {
    assert(argc == 2);
    std::ifstream file(std::filesystem::path(argv[1]) / "characters/sonic-selection.actions",
                       std::ios::binary);
    assert(file);
    const std::vector<unsigned char> canonical{std::istreambuf_iterator<char>(file), {}};
    assert(canonical.size() == sadx::native::app::kCanonicalSelectionBytes);
    // The canonical pack itself satisfies the reconstructed-prefix check.
    assert(sadx::native::app::selection_prefix_matches(canonical));
    // An append-only pack differs only in the record-count word.
    auto appended = canonical;
    const std::uint32_t appended_count = 3497;
    appended[8] = appended_count & 0xff;
    appended[9] = (appended_count >> 8) & 0xff;
    appended[10] = (appended_count >> 16) & 0xff;
    appended[11] = (appended_count >> 24) & 0xff;
    assert(sadx::native::app::selection_prefix_matches(appended));
    // A single altered prefix byte is rejected even when the metadata is unchanged.
    auto tampered = appended;
    tampered[64] ^= 0xff;
    assert(!sadx::native::app::selection_prefix_matches(tampered));
    return 0;
}
