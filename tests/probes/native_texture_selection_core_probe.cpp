#include "engine/texture_selection_native.hpp"
#include <cstring>

namespace ts = sadx::texture_selection;
namespace {
std::uint32_t identity(const ts::Memory *memory, const std::array<ts::Memory, 3> &memories) {
    if (!memory)
        return 0;
    for (std::size_t index = 0; index < memories.size(); ++index)
        if (memory == &memories[index])
            return index + 1;
    return 0xffffffffU;
}
std::uint32_t source_boundary(ts::Boundary boundary) {
    switch (boundary) {
    case ts::Boundary::CacheMemory:
        return 0x78cf24;
    case ts::Boundary::ListEntry:
        return 0x77f3dc;
    case ts::Boundary::IndexEntry:
        return 0x77f44e;
    case ts::Boundary::PaletteVersion:
        return 0x78ce3b;
    case ts::Boundary::PaletteSurface:
        return 0x78ce51;
    case ts::Boundary::PaletteUpload:
        return 0x78ce5a;
    case ts::Boundary::DirectBinding:
        return 0x78cf4e;
    case ts::Boundary::PaletteBinding:
        return 0x78ce1b;
    }
    return 0xffffffffU;
}
} // namespace
// Concrete native resources. No missing-fixture masks are accepted by this ABI.
extern "C" int sadx_native_texture_selection_core(const std::uint32_t *w, std::uint32_t *out) {
    if (!w || !out)
        return -1;
    std::array<ts::Memory, 3> memories;
    std::array<std::uint32_t, 3> surfaces;
    for (std::size_t index = 0; index < memories.size(); ++index) {
        auto &memory = memories[index];
        const auto *input = w + 16 + index * 6;
        memory.global_index = static_cast<std::int32_t>(input[0]);
        memory.bank = static_cast<std::int32_t>(input[1]);
        memory.pixel_format = input[2];
        surfaces[index] = input[3];
        if (input[3])
            memory.surface = std::make_shared<sadx::texture_lifetime::Surface>();
        memory.palette_version = static_cast<std::uint16_t>(input[4]);
    }
    ts::List list;
    for (std::size_t index = 0; index < w[41] && index < memories.size(); ++index)
        list.entries.push_back(&memories[index]);
    const ts::List *selected = w[2] == 0 ? nullptr : &list;
    const ts::List *current = w[0] == 1 ? nullptr : selected;
    const ts::List *queued = w[48] ? &list : nullptr;
    std::uint32_t queued_index = w[49];
    auto *requested = w[46] ? &memories[(w[46] - 1) % 3] : nullptr;
    auto *active = w[47] ? &memories[(w[47] - 1) % 3] : nullptr;
    std::int32_t cached = static_cast<std::int32_t>(w[5]);
    sadx::texture_palette::State palettes;
    for (std::size_t index = 0; index < palettes.entries.size(); ++index)
        palettes.entries[index].bank = static_cast<std::int32_t>(w[34] + index);
    palettes.entries[0].bank = static_cast<std::int32_t>(w[35]);
    palettes.entries[0].version = static_cast<std::uint16_t>(w[36]);
    if (w[37] == 1)
        palettes.entries[0].colors = std::make_shared<const std::array<std::uint32_t, 256>>();
    ts::State state{current, requested, cached, active, palettes};
    std::uint32_t call_count = 0;
    ts::BindSurface bind;
    if (w[9] != 1)
        bind = [&](const ts::Surface &surface) {
            std::uint32_t word = 0;
            for (std::size_t index = 0; index < memories.size(); ++index)
                if (surface && surface == memories[index].surface)
                    word = surfaces[index];
            out[12 + call_count * 3] = 0;
            out[13 + call_count * 3] = 0;
            out[14 + call_count * 3] = word;
            ++call_count;
            // Observe stores at the callback, before ID/active publication.
            out[24] = static_cast<std::uint32_t>(cached);
            out[25] = identity(active, memories);
            out[26] = current == &list;
            out[27] = identity(requested, memories);
            out[28] = queued == &list;
            out[29] = queued_index;
            if (w[9] == 2)
                throw ts::Error((w[0] == 0 ? memories[w[3] % 3].pixel_format
                                           : requested->pixel_format) == 0x28000000
                                    ? ts::Boundary::PaletteBinding
                                    : ts::Boundary::DirectBinding,
                                "synthetic unavailable device receipt");
        };
    try {
        if (w[0] == 0)
            out[3] = ts::cache(&memories[w[3] % 3], state, bind);
        else if (w[0] == 1)
            out[3] = ts::select_queued_list(selected, queued, queued_index, state, bind);
        else if (w[0] == 2)
            out[3] = ts::select_index(w[1], state, bind);
        else
            return -2;
        out[2] = 1;
    } catch (const ts::Error &error) {
        out[1] = source_boundary(error.boundary);
    }
    out[4] = current == &list;
    out[5] = static_cast<std::uint32_t>(cached);
    out[6] = identity(requested, memories);
    out[7] = identity(active, memories);
    for (std::size_t index = 0; index < memories.size(); ++index)
        out[8 + index] = memories[index].palette_version.value();
    out[11] = call_count;
    out[22] = queued == &list;
    out[23] = queued_index;
    return 0;
}
