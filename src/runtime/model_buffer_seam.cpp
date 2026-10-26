#include "game/game_mode.hpp"
#include "runtime/model_device.hpp"
#include "runtime/model_draw_environment.hpp"

namespace sadx::native {
polybuff::BufferState &normal_uv_buffer(Display &display) {
    constexpr std::size_t kNormalUvBufferIndex = 8;
    polybuff::Context context{display};
    return context.buffer(kNormalUvBufferIndex);
}
} // namespace sadx::native
