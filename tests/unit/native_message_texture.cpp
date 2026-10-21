#include "engine/native_message_texture.hpp"
#include <cassert>
#include <stdexcept>

int main() {
    using namespace sadx;
    std::vector<texture_lifetime::Memory> pool(2);
    std::int32_t cached = -1;
    texture_lifetime::Memory *active = nullptr;
    const texture_lifetime::List *current = nullptr;
    std::array<std::uint32_t, 8> scratch{};
    texture_palette::State palette;
    const auto sentinel = std::make_shared<std::array<std::uint32_t, 256>>();
    palette.current_colors = sentinel;
    texture_lifetime::State state{pool, cached, active, current, scratch};
    texture_lifetime::List list;
    const std::uint16_t pixels[] = {0, 65535, 4660, 43981};
    assert(message_image::load(list, 7, pixels, 2, 2, state, palette) == 1);
    auto *memory = list.entries[0];
    assert(memory == &pool[0] && memory->references == 1);
    assert(memory->type == 151126016 && memory->pixel_format == 268435456);
    assert(memory->width == 2 && memory->height == 2 && memory->texture_size == 8);
    assert(!memory->tsp_parameters && !memory->texture_parameters && !memory->palette_version);
    assert(memory->surface->texture->bgra == std::vector<std::uint8_t>({
        0,0,0,0,255,255,255,255,68,51,34,17,221,204,187,170}));
    graphics::Draw earlier;
    earlier.texture = memory->surface->texture;
    const auto *original_data = memory->image_data;
    const std::uint16_t replacement[] = {65535, 0, 0, 0};
    assert(message_image::reload(7, replacement, state, palette) == 1);
    assert(memory == list.entries[0] && memory->references == 1);
    assert(!palette.current_colors);
    assert(memory->surface->texture != earlier.texture);
    assert(earlier.texture->bgra[0] == 0 && memory->surface->texture->bgra[0] == 255);
    assert(memory->image_data == original_data && memory->texture_size == 8);
    // Cached load does not dereference even a null new bitmap or alter metadata.
    texture_lifetime::List alias;
    assert(message_image::load(alias, 7, nullptr, 999, 999, state, palette) == 1);
    assert(alias.entries[0] == memory && memory->references == 2 && memory->width == 2);
    palette.current_colors = sentinel;
    assert(message_image::reload(99, nullptr, state, palette) == -1);
    assert(palette.current_colors == sentinel);
    const auto previous = memory->surface->texture;
    bool failed = false;
    try { message_image::reload(7, nullptr, state, palette); }
    catch (const std::runtime_error &) { failed = true; }
    assert(failed && memory->surface->texture == previous && memory->references == 2);
    assert(!palette.current_colors);
    texture_lifetime::Services services{[] {}};
    texture_lifetime::release_list(&list, state, services);
    assert(memory->references == 1);
    texture_lifetime::release_list(&alias, state, services);
    assert(memory->references == 0 && !memory->surface && earlier.texture->bgra[0] == 0);
    for (auto &entry : pool) entry.references = 1;
    texture_lifetime::List full;
    assert(message_image::load(full, 999, nullptr, 0, 0, state, palette) == 1);
    assert(full.entries.size() == 1 && full.entries[0] == nullptr);
}
