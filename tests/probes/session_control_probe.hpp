#pragma once
#include "runtime/session_control_defaults.hpp"
#include <array>
#include <cstdint>

namespace sadx::test {
// Reference serialization and non-default injection fixture; no native layout is copied.
std::array<std::uint32_t, 51> control_words(const runtime::SessionControlDefaults &);
runtime::SessionControlDefaults varied_control_defaults();
} // namespace sadx::test
