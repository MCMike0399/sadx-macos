#pragma once
#include "cpu/x87_math.hpp"
#include "engine/ninja_fog_initial_state.hpp"
#include <functional>
#include <stdexcept>

namespace sadx::fog {
// NOTSA: native session values; the reference adapter borrows its existing owners lazily.
struct State {
    NinjaFogInitialState values{};
    std::int32_t display_switch = 0;
    std::uint8_t profile = 0, special_flags = 0, active = 0;
};
enum class RenderState : std::uint32_t {
    Enabled = 28,
    Color = 34,
    Mode = 35,
    Start = 36,
    End = 37,
    Density = 38
};
enum class ErrorKind { TableIndex, NonfiniteRange, MissingDevice };
class Error : public std::runtime_error {
  public:
    explicit Error(ErrorKind kind) : std::runtime_error("fog operation failed"), kind(kind) {}
    ErrorKind kind;
};
// NOTSA: native composition supplies a device command sink and its established capabilities.
struct Runtime {
    State &state;
    X87Context &arithmetic;
    std::function<void(RenderState, std::uint32_t)> render;
    std::uint32_t raster_caps, device_flags;
};
// NOTSA: native entry points use owned values; reference entry points use the same implementation.
void initialize(Runtime &);
void set_color(Runtime &, std::uint32_t);
void generate_table(Runtime &, float start, float end);
void set_table(Runtime &);
void set_enabled(Runtime &, bool);
} // namespace sadx::fog
