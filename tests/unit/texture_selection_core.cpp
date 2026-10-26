#include "engine/texture_selection_native.hpp"
#include <iostream>

namespace ts = sadx::texture_selection;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void tests() {
    ts::Memory first, alias;
    first.global_index = 7;
    first.surface = std::make_shared<sadx::texture_lifetime::Surface>();
    auto image = std::make_shared<sadx::graphics::Texture>();
    first.surface->texture = image;
    alias.global_index = 7;
    ts::List list{{&first}, false, {}, {}};
    const ts::List *current = nullptr, *queued = nullptr;
    std::uint32_t index = 43;
    ts::Memory *requested = &alias, *active = &alias;
    std::int32_t cached = -1;
    sadx::texture_palette::State palettes;
    ts::State state{current, requested, cached, active, palettes};
    sadx::graphics::Draw device;
    unsigned calls = 0;
    ts::BindSurface bind = [&](const ts::Surface &surface) {
        require(queued == &list && index == 0 && current == &list && requested == &first,
                "queue, index, current and requested stores precede device binding");
        require(cached == -1 && active == &alias, "cache stores follow the callback");
        device.texture = surface ? surface->texture : nullptr;
        ++calls;
    };
    require(ts::select_queued_list(&list, queued, index, state, bind) == 1 && calls == 1 &&
                cached == 7 && active == &first && device.texture == image,
            "queued selection binds the resident image through canonical state");
    sadx::graphics::Draw submitted = device;
    require(ts::cache(&alias, state, {}) == 1 && active == &first,
            "same GBIX retains the active alias without reading a new surface");
    first.global_index = 8;
    first.surface.reset();
    require(ts::cache(&first, state, {}) == 1 && cached == 7 && active == &first,
            "null direct surface leaves the cache unchanged");
    require(submitted.texture == image, "submitted draw retains its independent image");
    first.pixel_format = 0x28000000;
    first.bank = -1;
    bind = [&](const ts::Surface &surface) {
        require(!surface, "palette binding permits null surface");
        device.texture.reset();
        ++calls;
    };
    require(ts::cache(&first, state, bind) == 0 && cached == 8 && calls == 2 && !device.texture,
            "palette path unbinds and then publishes cache owners");
    first.bank = 2;
    palettes.entries[0].bank = 2;
    palettes.entries[0].version = 5;
    first.palette_version = 1;
    try {
        ts::cache(&first, state, bind);
        throw std::runtime_error("null dirty palette surface must fail");
    } catch (const ts::Error &error) {
        require(error.boundary == ts::Boundary::PaletteSurface && first.palette_version == 5,
                "dirty revision is stored before null surface failure");
    }
    first.surface = std::make_shared<sadx::texture_lifetime::Surface>();
    first.palette_version = 1;
    try {
        ts::cache(&first, state, bind);
        throw std::runtime_error("palette upload must remain explicit");
    } catch (const ts::Error &error) {
        require(error.boundary == ts::Boundary::PaletteUpload && first.palette_version == 5,
                "dirty revision survives unsupported palette upload");
    }
    first.palette_version.reset();
    try {
        ts::cache(&first, state, bind);
        throw std::runtime_error("uninitialized revision must not be guessed");
    } catch (const ts::Error &error) {
        require(error.boundary == ts::Boundary::PaletteVersion, "uninitialized source revision");
    }
    first.pixel_format = 0;
    first.global_index = 9;
    bind = [&](const ts::Surface &) {
        first.global_index = 10;
        cached = 45;
        active = &alias;
    };
    require(ts::select_index(0x40000000U, state, bind) == 1 && cached == 9 && active == &first,
            "wrapped index selects first entry; original ID survives callback mutation");
    try {
        ts::select_index(0x15555556U, state, bind);
        throw std::runtime_error("cross-field pointer must fail");
    } catch (const ts::Error &error) {
        require(error.boundary == ts::Boundary::IndexEntry, "cross-field index stays unsupported");
    }
    index = 21;
    require(ts::select_queued_list(nullptr, queued, index, state, {}) == 1 && !current && !queued &&
                index == 0 && active == &first && requested == &first,
            "null list clears queue/current only");
    require(ts::select_index(0, state, {}) == 0xffffffffU, "null current list returns failure");
    ts::List empty;
    index = 99;
    try {
        ts::select_queued_list(&empty, queued, index, state, {});
        throw std::runtime_error("empty list must fail at its first source read");
    } catch (const ts::Error &error) {
        require(error.boundary == ts::Boundary::ListEntry && queued == &empty &&
                    current == &empty && index == 0,
                "prefix stores survive an empty list");
    }
}
} // namespace
int main() {
    try {
        tests();
        std::cout << "texture selection core passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
