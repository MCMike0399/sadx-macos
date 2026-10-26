#include "engine/polybuff_initialization.hpp"
#include <stdexcept>

namespace sadx::polybuff {
// Test-only loud boundaries: Root supplies its real library/storage bindings at integration.
// These functions add no Context fields, resource definitions, registry or guessed values.
ConfiguredCounts configured_counts(Context &) {
    throw std::runtime_error("native configured library counts unbound in coexistence test");
}
BufferMemory *allocate_vertex_scratch(Context &, std::uint32_t) {
    throw std::runtime_error("native startup scratch allocation unbound in coexistence test");
}
void observe_initialization_store(Context &, InitializationField, std::size_t) {}
} // namespace sadx::polybuff
