#include "game/file_select_display.hpp"
#include "game/menu_eligibility.hpp"
#include "cpu/fidelity_float.hpp"
#include "engine/menu_square.hpp"
#include <cstdio>
#include <stdexcept>

namespace sadx::file_select_display {
namespace {
using fidelity::ExactFloat;
// NOTSA: a readable composition of the source's repeated draw formulas. Every
// value is borrowed from the caller; this object owns no game or renderer state.
class Display {
  public:
    title_menu::FileSelectWork &work;
    View &view;
    pvr::State &pvr_state;
    Calls &calls;
    X87Context &fp;
    Display(title_menu::FileSelectWork &w, View &v, pvr::State &p, Calls &c, X87Context &f)
        : work(w), view(v), pvr_state(p), calls(c), fp(f) {}
    ExactFloat value(float v) { return ExactFloat(fp, v); }
    float screen_x(ExactFloat x) {
        const auto offset = x - 320.0f;
        const auto center = value(view.horizontal_scale) * 320.0f;
        return (offset + center).rounded();
    }
    float screen_y(ExactFloat y) {
        const auto offset = y - 240.0f;
        const auto center = value(view.vertical_scale) * 240.0f;
        return (offset + center).rounded();
    }
    ExactFloat panel_x() { return value(work.offset_x) + 248.0f; }
    void texture(std::uint32_t index, ExactFloat x, ExactFloat y, ExactFloat z) {
        const auto depth = z.rounded();
        const auto py = screen_y(y);
        const auto px = screen_x(x);
        pvr::draw_texture(index, px, py, depth, pvr_state, view.display_disabled, calls, fp);
    }
    void scaled(std::uint32_t index, ExactFloat x, ExactFloat y, ExactFloat z, float sx, float sy) {
        const auto depth = z.rounded();
        const auto py = screen_y(y);
        const auto px = screen_x(x);
        pvr::draw_scaled_texture(index, px, py, depth, sx, sy, pvr_state, view.display_disabled,
                                 calls, fp);
    }
    void sized(std::uint32_t index, ExactFloat x, ExactFloat y, ExactFloat z, float width,
               float height) {
        const auto depth = z.rounded();
        const auto py = screen_y(y);
        const auto px = screen_x(x);
        menu_draw::draw_sized_texture(index, px, py, depth, width, height, pvr_state,
                                      view.display_disabled, calls, fp);
    }
    void material(std::uint32_t color) { pvr::set_material(color, pvr_state); }
    void colors(std::uint32_t top, std::uint32_t bottom) {
        menu_draw::set_vertex_colors({top, bottom, top, bottom}, pvr_state);
    }
    void select(MenuTexture list) { calls.select_menu_texture(list); }
    // NOTSA: both native menus consume the same source eligibility helpers.
    std::int32_t emblem_count() const { return menu_eligibility::emblem_count(view.save); }
    bool character_enabled(unsigned character) {
        return menu_eligibility::character_enabled(character,
            {view.save, view.event_flags, view.unlock_override, view.tutorial_variant}) != 0;
    }
    // FUNCTION: SONIC 0x506950 FileSelect_LastCharacter
    unsigned last_character() const {
        switch (view.save.last_character) {
        case 2:
            return 1;
        case 3:
            return 2;
        case 5:
            return 3;
        case 6:
            return 5;
        case 7:
            return 4;
        default:
            return 0;
        }
    }
    std::uint8_t stage_texture(unsigned stage) const {
        if (stage >= view.tables.stage_names.size())
            throw std::runtime_error(
                "file-select stage label is outside the imported source prefix");
        return view.tables.stage_names[stage];
    }
    void sky(float depth) {
        draw_sky(depth, {view.horizontal_scale, view.vertical_scale, view.display_disabled,
                         view.screen_width, view.screen_height, view.tables.sky_tiles},
                 pvr_state, calls, fp);
    }
    // NOTSA: adapts the shared square drawings to this display's canonical PVR owner.
    struct SquareCalls final : menu_square::Calls {
        Display &display;
        explicit SquareCalls(Display &owner) : display(owner) {}
        void select_square() override {
            display.calls.select_standard_texture(menu_draw::StandardTexture::Square);
        }
        void set_material(std::uint32_t color) override { display.material(color); }
        void draw_scaled(std::uint32_t index, float x, float y, float depth, float sx,
                         float sy) override {
            pvr::draw_scaled_texture(index, x, y, depth, sx, sy, display.pvr_state,
                                     display.view.display_disabled, display.calls, display.fp);
        }
    };
    // NOTSA: borrowed adapter for the source square helper.
    void square(std::uint32_t color, float x, float y, float depth, float sx, float sy) {
        SquareCalls square_calls(*this);
        menu_square::draw(color, x, y, depth, sx, sy,
                          {view.horizontal_scale, view.vertical_scale}, square_calls, fp);
    }
    // NOTSA: borrowed adapter for the source square cursor helper.
    void cursor(std::uint8_t alpha, float x, float y, float depth, float sx, float sy) {
        SquareCalls square_calls(*this);
        menu_square::draw_cursor(alpha, x, y, depth, sx, sy,
                                 {view.horizontal_scale, view.vertical_scale}, square_calls, fp);
    }
    // FUNCTION: SONIC 0x5077b0 MenuDrawArrow
    void arrow(bool up, float x, float y, float depth) {
        const auto anchor = up ? 1191182336U : 16777216U;
        select(MenuTexture::ScrollArrows);
        material(4294967295U);
        texture(anchor | 1U, value(x), value(y), value(depth));
        texture(anchor, value(x), value(y), value(depth) - 6.0f);
    }
    // FUNCTION: SONIC 0x504a00 FileSelect_FileCards
    void file_cards() {
        auto node = view.files;
        auto small = menu_draw::current_font(view.font);
        const auto original = small;
        auto skip = static_cast<std::uint16_t>(view.first_visible_file);
        small.char_width = 13.0f;
        small.char_height = 13.0f;
        while (skip != 0) {
            if (!node)
                throw std::runtime_error("file-select display scroll exceeds the live list");
            node = node->next;
            --skip;
        }
        for (std::int32_t row = 0;; ++row) {
            const auto row_offset = (ExactFloat::integer(fp, row) * 60.0f).rounded();
            const auto depth = (value(work.presentation.depth) - 9.0f).rounded();
            const auto y = (value(row_offset) + 165.0f).rounded();
            square(4279416063U, 147.0f, y, depth, 0.7f, 0.7f);
            if (view.selection_has_files < 4 && row == 0 && view.first_visible_file == 0) {
                if (view.file_count > 98) {
                    small.colors.fill(1728053247U);
                    menu_draw::set_current_font(small, view.font);
                }
                const auto text_depth = (value(work.presentation.depth) - 12.0f).rounded();
                const auto text_y = (value(view.vertical_scale) * 240.0f - 80.0f).rounded();
                const auto text_x = (value(view.horizontal_scale) * 320.0f - 240.0f).rounded();
                menu_draw::draw_text("Create New File", text_x, text_y, text_depth, view.font,
                                     calls, fp);
                small.colors.fill(4294967295U);
            } else {
                if (!node || !node->file)
                    throw std::runtime_error(
                        "file-select display reached an indeterminate source filename/time");
                const auto local = calls.local_calendar(node->file->last_write_time);
                std::array<char, 128> date{};
                std::snprintf(date.data(), date.size(), "%02d/%02d/%02d  %02d:%02d:%02d",
                              static_cast<int>(local.year) - 2000, local.month, local.day,
                              local.hour, local.minute, local.second);
                menu_draw::set_current_font(small, view.font);
                const auto text_depth = (value(work.presentation.depth) - 7.0f).rounded();
                const auto text_y = screen_y(value(row_offset) + 170.0f);
                const auto text_x = (value(view.horizontal_scale) * 320.0f - 255.0f).rounded();
                menu_draw::draw_text(date.data(), text_x, text_y, text_depth, view.font, calls, fp);
                auto name = node->file->name.substr(0, 63);
                const auto extension = name.find('.');
                if (extension != std::string::npos)
                    name.resize(extension);
                while (menu_draw::text_width(name, view.font) > 165) {
                    if (name.empty())
                        throw std::runtime_error("font width cannot fit an empty source filename");
                    name.pop_back();
                }
                menu_draw::set_current_font(original, view.font);
                const auto name_depth = (value(work.presentation.depth) - 7.0f).rounded();
                const auto name_y = screen_y(value(row_offset) + 150.0f);
                const auto name_x = (value(view.horizontal_scale) * 320.0f - 255.0f).rounded();
                menu_draw::draw_text(name, name_x, name_y, name_depth, view.font, calls, fp);
            }
            if (!node)
                throw std::runtime_error("file-select display has no source list sentinel");
            node = node->next;
            if (!node || row == 3)
                break;
        }
        auto cursor_y = (ExactFloat::integer(fp, view.selected_card) * 60.0f + 165.0f).rounded();
        if (view.file_count == 0) {
            view.selected_file = 0;
            cursor_y = 165.0f;
            view.selected_card = 0;
        }
        const auto sy = (value(work.cursor_scale) * 0.75f).rounded();
        const auto sx = (value(work.cursor_scale) * 0.7f).rounded();
        const auto cursor_depth = (value(work.presentation.depth) - 14.0f).rounded();
        cursor(work.cursor_alpha, 147.0f, cursor_y, cursor_depth, sx, sy);
        if (view.first_visible_file != 0)
            arrow(true, 147.0f, 116.0f, (value(work.presentation.depth) - 6.0f).rounded());
        if (view.file_count >= 4 &&
            static_cast<std::uint16_t>(view.first_visible_file) < view.file_count - 3)
            arrow(false, 147.0f, 392.0f, (value(work.presentation.depth) - 6.0f).rounded());
    }
    // FUNCTION: SONIC 0x504f40 FileSelect_MemoryCard
    void memory_card() {
        material(4294967295U);
        select(MenuTexture::MemoryCard);
        const auto back_depth = (value(work.presentation.depth) - 8.0f).rounded();
        const auto back_y = (value(view.vertical_scale) * 240.0f - 202.0f).rounded();
        const auto back_x = (value(view.horizontal_scale) * 320.0f - 320.0f).rounded();
        menu_draw::draw_title_back(back_x, back_y, back_depth, 564.0f, 41.0f, pvr_state,
                                   view.display_disabled, calls, fp);
        texture(8, value(42.0f), value(46.0f), value(work.presentation.depth) - 14.0f);
        material(2013286399U);
        const auto window_depth = (value(work.presentation.depth) - 6.0f).rounded();
        const auto window_y = (value(view.vertical_scale) * 240.0f - 156.0f).rounded();
        const auto window_x = (value(view.horizontal_scale) * 320.0f - 273.0f).rounded();
        menu_draw::draw_shadow_window(window_x, window_y, window_depth, 200.0f, 340.0f, pvr_state,
                                      view.display_disabled, calls, fp);
        file_cards();
    }
    // FUNCTION: SONIC 0x5041e0 FileSelect_Dialog
    void dialog_frame() {
        colors(2014491903U, 2014491903U);
        const auto depth = (value(work.presentation.depth) - 4.0f).rounded();
        const auto y = screen_y(value(78.0f));
        const auto x = screen_x(panel_x());
        menu_draw::draw_shadow_window(x, y, depth, 320.0f, 360.0f, pvr_state, view.display_disabled,
                                      calls, fp);
        select(MenuTexture::FileSelect);
        for (auto it = view.tables.frame_rows.rbegin(); it != view.tables.frame_rows.rend(); ++it) {
            const auto &row = *it;
            const auto first = title_menu::interpolate_colors(row.top_color, row.bottom_color,
                                                              0.0694444477558136f, fp);
            colors(row.top_color, first);
            scaled(row.corner_texture, panel_x(), value(78.0f),
                   value(row.depth) + work.presentation.depth, 0.78125f, 0.78125f);
            scaled(row.corner_texture | 2181038080U, panel_x() + 320.0f, value(78.0f),
                   value(row.depth) + work.presentation.depth, 0.78125f, 0.78125f);
            sized(row.horizontal_texture, panel_x() + 25.0f, value(78.0f),
                  value(row.depth) + work.presentation.depth, 270.0f, 25.0f);
            const auto second = title_menu::interpolate_colors(row.top_color, row.bottom_color,
                                                               0.9305555820465088f, fp);
            colors(first, second);
            sized(row.vertical_texture, panel_x(), value(103.0f),
                  value(row.depth) + work.presentation.depth, 25.0f, 310.0f);
            sized(row.vertical_texture | 2181038080U, value(row.right_x) + work.offset_x,
                  value(103.0f), value(row.depth) + work.presentation.depth, 25.0f, 310.0f);
            colors(second, row.bottom_color);
            scaled(row.corner_texture | 1174405120U, panel_x(), value(438.0f),
                   value(row.depth) + work.presentation.depth, 0.78125f, 0.78125f);
            scaled(row.corner_texture | 3355443200U, panel_x() + 320.0f, value(438.0f),
                   value(row.depth) + work.presentation.depth, 0.78125f, 0.78125f);
            sized(row.horizontal_texture | 1073741824U, panel_x() + 25.0f, value(row.bottom_y),
                  value(row.depth) + work.presentation.depth, 270.0f, 25.0f);
        }
    }
    // FUNCTION: SONIC 0x504810 FileSelect_CharacterIcons
    void character_icons() {
        if (view.selection_has_files == 0)
            return;
        select(MenuTexture::FileSelect);
        material(4294967295U);
        const auto count = character_enabled(6) ? 7 : 6;
        auto x = (panel_x() + (count == 7 ? 34.0f : 50.0f)).rounded();
        const auto step = count == 7 ? 42.0f : 44.0f;
        const auto depth = (value(work.presentation.depth) - 12.0f).rounded();
        for (unsigned character = 0; character < static_cast<unsigned>(count); ++character) {
            if (character_enabled(character)) {
                texture(67108886U, value(x), value(164.0f), value(depth));
                scaled(view.tables.unlocked_icons[character] | 67108864U, value(x), value(164.0f),
                       value(depth) - 0.3f, 0.5f, 0.5f);
            } else {
                scaled(view.tables.locked_icons[character] | 67108864U, value(x), value(164.0f),
                       value(depth), 0.5f, 0.5f);
            }
            x = (value(step) + x).rounded();
        }
    }
    // FUNCTION: SONIC 0x5045d0 FileSelect_DetailPanel
    void detail_panel() {
        select(MenuTexture::FileSelect);
        material(4294967295U);
        for (unsigned line = 0; line < 4; ++line) {
            const auto y = (ExactFloat::integer(fp, line) * 57.0f + 200.0f).rounded();
            select(MenuTexture::FileSelect);
            material(4294967295U);
            texture(67108864U, panel_x() + 160.0f, value(y), value(work.presentation.depth) - 6.0f);
            texture(view.tables.detail_labels[line] | 67108864U, panel_x() + 160.0f, value(y),
                    value(work.presentation.depth) - 8.0f);
            const auto square_depth = (value(work.presentation.depth) - 8.0f).rounded();
            const auto square_y = (value(y) + 28.0f).rounded();
            const auto square_x = (panel_x() + 160.0f).rounded();
            square(4279416063U, square_x, square_y, square_depth, 0.968999981880188f, 0.5f);
        }
        // Pinned loader changes50473e75f0 toEB00: one bind, not65536.
        select(MenuTexture::FileCommon);
        select(MenuTexture::FileSelect);
        material(4294967295U);
        scaled(67108872U, panel_x() + 160.0f, value(122.0f), value(work.presentation.depth) - 6.0f,
               1.8830000162124634f, 1.0f);
    }
    void number(std::int32_t value_number, std::int32_t digits, bool zero_fill, float x, float y,
                float depth, float spacing) {
        draw_number(value_number, digits, zero_fill, x, y, depth, spacing,
                    {view.horizontal_scale, view.vertical_scale, view.display_disabled, view.tables.digits},
                    pvr_state, calls, fp);
    }
    // FUNCTION: SONIC 0x504100 FileSelect_Emblems
    void emblems() {
        if (view.selection_has_files == 0)
            return;
        select(MenuTexture::Emblem);
        material(4294967295U);
        texture(67108864U, panel_x() + 128.0f, value(401.0f),
                value(work.presentation.depth) - 12.0f);
        select(MenuTexture::Emblem);
        material(4294967295U);
        texture(67108865U, panel_x() + 128.0f + 41.0f, value(401.0f),
                value(work.presentation.depth) - 12.0f);
        const auto z = (value(work.presentation.depth) - 12.0f).rounded();
        const auto x = (panel_x() + 128.0f + 74.0f).rounded();
        const auto emblems = emblem_count();
        auto counter = emblems;
        std::int32_t digits = 0;
        do {
            counter /= 10;
            ++digits;
        } while (counter != 0);
        number(emblems, digits, true, x, 401.0f, z, 1.0f);
    }
    // FUNCTION: SONIC 0x5047c0 FileSelect_CharacterName
    void character_name() {
        if (view.selection_has_files == 0)
            return;
        const auto z = (value(work.presentation.depth) - 12.0f).rounded();
        const auto x = (panel_x() + 160.0f).rounded();
        const auto character = last_character();
        select(character == 6 && view.tutorial_variant == 1 ? MenuTexture::MetalName
                                                      : MenuTexture::CharacterNames);
        material(4294967295U);
        scaled(
            (character == 6 && view.tutorial_variant == 1 ? 2U : view.tables.character_names[character]) |
                67108864U,
            value(x), value(122.0f), value(z), 0.7f, 0.7f);
    }
    // FUNCTION: SONIC 0x503e50 FileSelect_LastAdventureField
    void last_field() {
        if (view.selection_has_files == 0)
            return;
        select(MenuTexture::StageNames);
        material(4294967295U);
        if (static_cast<std::uint8_t>(view.save.last_stage) == 100)
            return;
        const auto character = last_character();
        unsigned stage;
        if (view.unlock_override != 0 ||
            view.event_flags.at(view.tables.clear_flags[character]) != 0) {
            stage = 26;
        } else {
            const auto sequence = view.tables.sequence_indices[character];
            if (sequence >= view.save.sequences.size())
                throw std::runtime_error("file-select sequence table exceeds native save");
            stage = view.save.sequences[sequence].stage >> 8;
        }
        texture(stage_texture(stage) | 67108864U, panel_x() + 160.0f, value(228.0f),
                value(work.presentation.depth) - 12.0f);
    }
    // FUNCTION: SONIC 0x503ef0 FileSelect_StageCompleted
    void completed_stage() {
        if (view.selection_has_files == 0)
            return;
        select(MenuTexture::StageNames);
        material(4294967295U);
        if (static_cast<std::uint8_t>(view.save.last_stage) == 100)
            return;
        if (last_character() == 1) {
            if (static_cast<std::uint8_t>(view.save.last_stage) == 1)
                return;
            if (static_cast<std::uint8_t>(view.save.last_stage) == 5) {
                const auto z = (value(work.presentation.depth) - 12.0f).rounded();
                const auto x = (panel_x() + 160.0f).rounded();
                pvr::draw_texture(67108880U, x, 285.0f, z, pvr_state, view.display_disabled, calls,
                                  fp);
                return;
            }
        }
        texture(stage_texture(static_cast<std::uint8_t>(view.save.last_stage)) | 67108864U,
                panel_x() + 160.0f, value(285.0f), value(work.presentation.depth) - 12.0f);
    }
    // FUNCTION: SONIC 0x503ff0 FileSelect_PlayTime
    void play_time() {
        if (view.selection_has_files == 0)
            return;
        select(MenuTexture::Digits);
        material(4294967295U);
        const auto z = (value(work.presentation.depth) - 12.0f).rounded();
        texture(67108864U, panel_x() + 160.0f, value(342.0f), value(z));
        const auto hours_x = (panel_x() + 160.0f - 64.0f).rounded();
        auto minutes = static_cast<std::int32_t>(view.save.total_time) / 3600;
        number(minutes / 60, 4, false, hours_x, 342.0f, z, 1.0f);
        const auto minutes_x = (panel_x() + 160.0f + 16.0f).rounded();
        minutes = static_cast<std::int32_t>(view.save.total_time) / 3600;
        number(minutes % 60, 2, true, minutes_x, 342.0f, z, 1.0f);
    }
};
} // namespace
void draw_sky(float depth, SkyView view, pvr::State &state, Calls &calls, X87Context &fp) {
    if (view.screen_width != 640 || view.screen_height != 480 ||
        view.horizontal_scale != 1.0f || view.vertical_scale != 1.0f)
        throw std::runtime_error("menu background scaling outside qualified640x480");
    calls.select_menu_texture(MenuTexture::Background);
    pvr::set_material(4294967295U, state);
    for (const auto &tile : view.tiles) {
        const auto y = (ExactFloat::integer(fp, tile.y) * view.vertical_scale + 0.0f).rounded();
        const auto x = (ExactFloat::integer(fp, tile.x) * view.horizontal_scale + 0.0f).rounded();
        pvr::draw_scaled_texture(static_cast<std::uint32_t>(tile.texture), x, y, depth,
                                 view.horizontal_scale, view.vertical_scale, state,
                                 view.display_disabled, calls, fp);
    }
}
void draw_sky_profiled(float depth, SkyView view, pvr::State &state, Calls &calls,
                       X87Context &fp, ui_scale::State &scaling) {
    ui_scale::run_identity_background(scaling, view.screen_width, view.screen_height,
        [&] { draw_sky(depth, view, state, calls, fp); });
}
void draw_number(std::int32_t value_number, std::int32_t digits, bool zero_fill,
                 float x, float y, float depth, float spacing, NumberView view,
                 pvr::State &state, Calls &calls, X87Context &fp) {
    if (digits == 0)
        return;
    auto count = digits;
    std::int32_t visible = 0;
    auto quotient = value_number;
    do {
        quotient /= 10;
        ++visible;
    } while (quotient != 0);
    calls.select_menu_texture(MenuTexture::Digits);
    pvr::set_material(4294967295U, state);
    const auto size = calls.texture(1);
    const auto width = static_cast<std::int32_t>(size.width);
    const auto initial =
        static_cast<std::int32_t>((static_cast<std::uint32_t>(digits) - 1U) * size.width);
    x = (ExactFloat::integer(fp, initial) * spacing + x).rounded();
    do {
        if (zero_fill || visible > 0) {
            const auto digit = value_number % 10;
            if (digit < 0)
                throw std::runtime_error("negative number indexes before source digit table");
            const auto z = ExactFloat(fp, depth).rounded();
            const auto offset_y = ExactFloat(fp, y) - 240.0f;
            const auto center_y = ExactFloat(fp, view.vertical_scale) * 240.0f;
            const auto py = (offset_y + center_y).rounded();
            const auto offset_x = ExactFloat(fp, x) - 320.0f;
            const auto center_x = ExactFloat(fp, view.horizontal_scale) * 320.0f;
            const auto px = (offset_x + center_x).rounded();
            pvr::draw_texture(view.digits[static_cast<unsigned>(digit)] | 67108864U,
                              px, py, z, state, view.display_disabled, calls, fp);
            x = (ExactFloat(fp, x) - ExactFloat::integer(fp, width) * spacing).rounded();
        }
        --visible;
        --count;
        value_number /= 10;
    } while (count != 0);
}
// NOTSA: original drawing and native profile share this ordered body.
static void draw_impl(title_menu::FileSelectWork &work, View &view, pvr::State &state, Calls &calls,
          X87Context &fp, ui_scale::State *scaling) {
    if (work.presentation.color == 0)
        return;
    Display display(work, view, state, calls, fp);
    pvr::default_blending([&](auto target, auto mode) { calls.set_blend(target, mode); });
    const auto background_depth = ExactFloat(fp, work.presentation.depth) - 2.0f;
    work.offset_x = 20.0f;
    const auto depth = background_depth.rounded();
    if (scaling)
        ui_scale::run_identity_background(*scaling, view.screen_width, view.screen_height,
                                          [&] { display.sky(depth); });
    else
        display.sky(depth);
    display.memory_card();
    pvr::set_base_color(work.presentation.color, state);
    display.dialog_frame();
    display.character_icons();
    display.detail_panel();
    display.emblems();
    display.character_name();
    display.last_field();
    display.completed_stage();
    display.play_time();
    if (view.first_message_visible != 0)
        calls.display_message(view.first_message);
    if (view.second_message_visible != 0)
        calls.display_message(view.second_message);
}
void draw(title_menu::FileSelectWork &work, View &view, pvr::State &state, Calls &calls,
           X87Context &fp) {
    draw_impl(work, view, state, calls, fp, nullptr);
}
void draw_profiled(title_menu::FileSelectWork &work, View &view, pvr::State &state, Calls &calls,
                   X87Context &fp, ui_scale::State &scaling) {
    ui_scale::require_stretch_owners(scaling, view.horizontal_scale, view.vertical_scale);
    draw_impl(work, view, state, calls, fp, &scaling);
}
} // namespace sadx::file_select_display
