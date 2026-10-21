#include "engine/native_message_texture.hpp"
#include <algorithm>
extern "C" void sadx_message_image_probe(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx;
    std::fill(out, out + 32, 0);
    std::vector<texture_lifetime::Memory> pool(in[0]);
    std::int32_t cached = -1;
    texture_lifetime::Memory *active = nullptr;
    const texture_lifetime::List *current = nullptr;
    std::array<std::uint32_t, 8> scratch{};
    texture_palette::State palette;
    texture_lifetime::State state{pool, cached, active, current, scratch};
    if (pool.size() > 2 && in[4]) {
        pool[2].global_index = static_cast<std::int32_t>(in[1]);
        pool[2].references = static_cast<std::uint16_t>(in[5]);
        pool[2].width = in[6];
    }
    std::vector<std::uint16_t> pixels(in[2] * in[3], 65535);
    texture_lifetime::List list;
    message_image::load(list, in[1], pixels.data(), in[2], in[3], state, palette);
    auto *m = list.entries[0];
    if (!m) return;
    const bool defined = in[0] > 2 && in[4] && in[5] != 0;
    out[0] = static_cast<std::uint32_t>(m - pool.data()) + 1;
    out[1] = static_cast<std::uint32_t>(m->global_index);
    out[2] = static_cast<std::uint32_t>(m->bank);
    out[3] = defined;
    out[4] = defined;
    out[5] = m->texture_flags;
    out[6] = m->image_data != nullptr;
    out[7] = m->type;
    out[8] = m->bit_depth;
    out[9] = m->pixel_format;
    out[10] = m->width;
    out[11] = m->height;
    out[12] = m->texture_size;
    out[13] = m->surface_flags;
    out[14] = m->surface != nullptr;
    out[15] = m->virtual_data != nullptr;
    out[16] = m->physical_data != nullptr;
    out[17] = m->references;
    out[18] = defined;
}
extern "C" void sadx_message_reload_probe(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx;
    std::fill(out, out + 32, 0);
    std::vector<texture_lifetime::Memory> pool(3);
    std::int32_t cached = -1;
    texture_lifetime::Memory *active = nullptr;
    const texture_lifetime::List *current = nullptr;
    std::array<std::uint32_t, 8> scratch{};
    texture_palette::State palette;
    texture_lifetime::State state{pool, cached, active, current, scratch};
    std::vector<std::uint16_t> pixels(in[1] * in[2], 65535);
    texture_lifetime::List list;
    message_image::load(list, in[0], pixels.data(), in[1], in[2], state, palette);
    auto *m = list.entries[0];
    m->references = static_cast<std::uint16_t>(in[3]);
    out[0] = static_cast<std::uint32_t>(message_image::reload(in[4] ? in[0] + 1 : in[0], pixels.data(), state, palette));
    out[1] = m->references;
    out[2] = m->type;
    out[3] = m->pixel_format;
    out[4] = m->width;
    out[5] = m->height;
    out[6] = m->texture_size;
}
extern "C" void sadx_palette_probe(const std::uint32_t *in, std::uint32_t *out) {
    using namespace sadx;
    texture_palette::State palette;
    auto first = std::make_shared<std::array<std::uint32_t, 256>>();
    auto second = std::make_shared<std::array<std::uint32_t, 256>>();
    auto previous = std::make_shared<std::array<std::uint32_t, 256>>();
    palette.current_colors = previous;
    palette.entries[in[0] % 128].bank = static_cast<std::int32_t>(in[1]);
    palette.entries[in[0] % 128].colors = first;
    palette.entries[in[2] % 128].bank = static_cast<std::int32_t>(in[3]);
    palette.entries[in[2] % 128].colors = second;
    texture_palette::select_bank(static_cast<std::int32_t>(in[4]), palette);
    out[0] = !palette.current_colors ? 0 : palette.current_colors == first ? 1 :
             palette.current_colors == second ? 2 : 3;
}
extern "C" void sadx_palette_initial_probe(std::uint32_t *out) {
    sadx::texture_palette::State palette;
    for (std::size_t index = 0; index < palette.entries.size(); ++index) {
        const auto &entry = palette.entries[index];
        out[index * 4] = static_cast<std::uint32_t>(entry.bank);
        out[index * 4 + 1] = entry.slot;
        out[index * 4 + 2] = entry.colors != nullptr;
        out[index * 4 + 3] = entry.version;
    }
    out[512] = palette.current_colors != nullptr;
    for (std::size_t index = 0; index < palette.upload_banks.size(); ++index)
        out[513 + index] = static_cast<std::uint32_t>(palette.upload_banks[index]);
}
