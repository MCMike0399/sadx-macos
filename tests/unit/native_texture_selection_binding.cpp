#include "runtime/native_display.hpp"
#include <cassert>
#include <iostream>

namespace sadx::native {
struct DisplayTextureProbe {
    static std::shared_ptr<graphics::Texture> image(std::array<std::uint8_t, 4> pixel) {
        auto texture = std::make_shared<graphics::Texture>();
        texture->width = texture->height = 1;
        texture->bgra.assign(pixel.begin(), pixel.end());
        return texture;
    }
    static void alias_and_lifetime(Display &display) {
        constexpr std::int32_t kAliasedIndex = 41, kOtherIndex = 83;
        constexpr std::uint32_t kOpaqueWhite = 0xffffffff;
        auto first_image = image({11, 12, 13, 255});
        auto old_image = image({21, 22, 23, 255});
        auto next_image = image({31, 32, 33, 255});
        auto &first = display.texture_pool_[0];
        auto &old = display.texture_pool_[1];
        auto &next = display.texture_pool_[2];
        first.global_index = old.global_index = kAliasedIndex;
        next.global_index = kOtherIndex;
        first.references = old.references = next.references = 1;
        first.surface =
            std::make_shared<texture_lifetime::Surface>(texture_lifetime::Surface{first_image, {}});
        old.surface =
            std::make_shared<texture_lifetime::Surface>(texture_lifetime::Surface{old_image, {}});
        next.surface =
            std::make_shared<texture_lifetime::Surface>(texture_lifetime::Surface{next_image, {}});
        texture_lifetime::List first_list{{&first}, false, {}, {}};
        texture_lifetime::List next_list{{&next}, false, {}, {}};
        texture_lifetime::List old_list{{&old}, false, {}, {}};
        display.output().select_direct_texture_list(old_list);
        display.requested_texture_memory_ = &next;
        display.queued_texture_index_ = 19;

        display.output().select_texture_list(first_list);
        assert(display.current_texture_list_ == &first_list);
        assert(display.queued_texture_list_ == &first_list && display.queued_texture_index_ == 0);
        assert(display.requested_texture_memory_ == &first);
        assert(display.active_texture_memory_ == &old);
        assert(display.device_state_.texture == old_image);
        const std::array<pvr::Vertex, 4> vertices{{{0, 0, 1, 0, 0, kOpaqueWhite},
                                                   {1, 0, 1, 1, 0, kOpaqueWhite},
                                                   {0, 1, 1, 0, 1, kOpaqueWhite},
                                                   {1, 1, 1, 1, 1, kOpaqueWhite}}};
        display.output().draw_texture(vertices, kAliasedIndex, 0);
        const auto first_draw = display.frame_.commands.size() - 1;
        assert(std::get<graphics::Draw>(display.frame_.commands[first_draw]).texture == old_image);
        auto uploaded = image({24, 25, 26, 255});
        texture_lifetime::update_surface(old, uploaded);
        display.output().draw_texture(vertices, kAliasedIndex, 0);
        assert(std::get<graphics::Draw>(display.frame_.commands.back()).texture == uploaded);
        assert(std::get<graphics::Draw>(display.frame_.commands[first_draw]).texture == old_image);

        display.queued_texture_index_ = 19;
        display.output().select_direct_texture_list(next_list);
        assert(display.current_texture_list_ == &next_list);
        assert(display.queued_texture_list_ == &first_list && display.queued_texture_index_ == 19);
        assert(display.requested_texture_memory_ == &next &&
               display.active_texture_memory_ == &next);
        assert(display.cached_texture_global_index_ == kOtherIndex);
        assert(display.device_state_.texture == next_image);
        assert(display.output().select_texture_index(0) == 1);
        display.release_texture_list(old_list);
        assert(old.references == 0 && !old.surface);
        old_image.reset();
        uploaded.reset();
        assert(std::get<graphics::Draw>(display.frame_.commands[first_draw]).texture->bgra[0] ==
               21);
        assert(std::get<graphics::Draw>(display.frame_.commands.back()).texture->bgra[0] == 24);
    }
    static void null_and_failure_prefix(Display &display) {
        constexpr std::uint32_t kIndexedPaletteFormat = 0x28000000;
        auto &palette = display.texture_pool_[3];
        palette.global_index = 97;
        palette.pixel_format = kIndexedPaletteFormat;
        palette.bank = -1;
        palette.surface.reset();
        texture_lifetime::List palette_list{{&palette}, false, {}, {}};
        display.output().select_texture_list(palette_list);
        assert(!display.device_state_.texture); // the palette path really calls SetTexture(NULL)
        assert(display.cached_texture_global_index_ == 97);
        assert(display.active_texture_memory_ == &palette);

        auto &missing = display.texture_pool_[4];
        missing.global_index = 113;
        missing.surface = std::make_shared<texture_lifetime::Surface>();
        texture_lifetime::List missing_list{{&missing}, false, {}, {}};
        bool failed = false;
        try {
            display.output().select_texture_list(missing_list);
        } catch (const std::runtime_error &) {
            failed = true;
        }
        assert(failed);
        assert(display.current_texture_list_ == &missing_list);
        assert(display.queued_texture_list_ == &missing_list && display.queued_texture_index_ == 0);
        assert(display.requested_texture_memory_ == &missing);
        assert(display.cached_texture_global_index_ == 97 &&
               display.active_texture_memory_ == &palette);
        assert(
            !display.device_state_.texture); // no fabricated cache store after a failed device bind
    }
};
} // namespace sadx::native

int main() {
    sadx::frame::ModeState timing{};
    sadx::native::Display display({}, timing);
    sadx::native::DisplayTextureProbe::alias_and_lifetime(display);
    sadx::native::DisplayTextureProbe::null_and_failure_prefix(display);
    std::cout
        << "Native texture binding: queued/direct owners, equal-ID device image, frame retention, "
           "null palette binding and failure prefix pass\n";
}
