#include "engine/font_native.hpp"

namespace sadx::font {
void load(const Info *info, State &state, Calls &calls) {
    if (info == nullptr) return;
    state.source = info;
    state.width = info->char_width;
    state.height = info->char_height;
    if (info->textures != nullptr) calls.load_texture(*info->textures);
}
void copy(const Info &info, State &state) {
    state.current = info;
}
void release(const Info &info, const std::function<void(texture_lifetime::List &)> &release_texture) {
    if (info.textures != nullptr) release_texture(*info.textures);
}
} // namespace sadx::font
