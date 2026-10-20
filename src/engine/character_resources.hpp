#pragma once
#include <cstdint>

namespace sadx {
// NOTSA: imported action identity, independent of file offsets or original process pointers.
// Zero preserves an actually absent action; the importer owns and validates its payload.
struct ActionHandle {
    std::uint64_t identity = 0;
};
} // namespace sadx
