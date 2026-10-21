#include "game/message_text.hpp"
#include <algorithm>

namespace sadx::message_text {
namespace {
// NOTSA: source directory index is read at each original lookup, not cached
// across callees.
const LanguageRule &language_rule(const State &state, std::int32_t language) {
    const auto index = static_cast<std::uint32_t>(language);
    if (index >= state.directory.languages.size())
        throw Error(Boundary::Language, "message language has no font directory");
    return state.directory.languages[index];
}
// NOTSA: the source remapping resource has a separate bounds boundary from its
// glyph table.
std::int32_t remap(const LanguageRule &rule, std::int32_t index) {
    if (index < 0 || static_cast<std::size_t>(index) >= rule.remapping->size())
        throw Error(Boundary::Mapping, "message character is outside the imported remapping table");
    return (*rule.remapping)[static_cast<std::size_t>(index)];
}
// NOTSA: source signed single-byte adjustment, shared by glyph and advance
// lookups.
std::int32_t adjusted_code(std::int32_t code, const LanguageRule &rule) {
    if (code > rule.lead_last)
        code = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(code) +
            static_cast<std::uint32_t>(static_cast<int>(rule.lead_first) - rule.lead_last));
    return code;
}
// NOTSA: asset bounds replace an unavailable source font read, never a
// fabricated glyph.
const Glyph &record(const std::shared_ptr<const FontData> &font, std::int32_t index) {
    if (!font)
        throw Error(Boundary::Font, "message font resource is absent");
    if (index < 0 || static_cast<std::size_t>(index) >= font->glyphs.size())
        throw Error(Boundary::Glyph, "message glyph is outside the imported source font");
    return font->glyphs[static_cast<std::size_t>(index)];
}
// NOTSA: named source scan of the rightmost occupied column; a blank glyph has
// width12.
std::uint8_t measured_width(const Glyph &glyph) {
    for (int column = 2; column >= 0; --column) {
        unsigned maximum = 0;
        for (unsigned row = 0; row < 24; ++row) {
            auto bits = glyph.bitmap[row * 3 + static_cast<unsigned>(column)];
            unsigned distance = 0;
            while (bits) {
                bits = static_cast<std::uint8_t>(bits << 1U);
                ++distance;
            }
            maximum = std::max(maximum, distance);
        }
        if (maximum)
            return static_cast<std::uint8_t>(static_cast<unsigned>(column) * 8 + maximum);
    }
    return 12;
}
// NOTSA: recovered16-bit cursor stores wrap independently of their signed read
// view.
std::int16_t cursor_step(std::int16_t cursor, std::int32_t delta) {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(cursor) +
                                     static_cast<std::uint32_t>(delta));
}
// NOTSA: bitmap capacity is the actual allocation; source right-edge writes may
// reach its padding.
void or_pixel(menu_caption::Context &context, std::int64_t index, std::uint16_t color) {
    if (index < 0 || static_cast<std::uint64_t>(index) >= context.bitmap_capacity)
        throw Error(Boundary::Bitmap, "message glyph write exceeds its bitmap allocation");
    context.bitmap[static_cast<std::size_t>(index)] |= color;
}
// NOTSA: equivalent named row of the source's unrolled monochrome glyph loop.
void draw_row(menu_caption::Context &context, const Glyph &glyph, unsigned row, std::int32_t y,
              std::uint16_t half_alpha) {
    auto x = static_cast<std::int32_t>(context.cursor_x);
    const auto width = static_cast<std::int32_t>(context.buffer_width);
    for (unsigned group = 0; group < 3; ++group) {
        auto bits = glyph.bitmap[row * 3 + group];
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (x < 0) {
                ++x;
                continue;
            }
            if (x >= width)
                continue;
            const auto present = (bits & 128U) != 0;
            const auto index = static_cast<std::int64_t>(width) * y + x;
            or_pixel(context, index, present ? context.color : 0);
            if (x > 0)
                or_pixel(context, index - 1, present ? half_alpha : 0);
            // Original quirk: the last column still writes its right neighbor in
            // bitmap padding.
            if (x < width)
                or_pixel(context, index + 1, present ? half_alpha : 0);
            bits = static_cast<std::uint8_t>(bits << 1U);
            ++x;
        }
    }
}
} // namespace
const Glyph &glyph(std::int32_t code, const State &state, const std::int32_t &language) {
    const auto &rule = language_rule(state, language);
    if (code < 256) {
        code = adjusted_code(code, rule);
        auto index = static_cast<std::int32_t>(static_cast<std::uint32_t>(code) - 32U);
        if (index < 0)
            index = 0;
        if (rule.remapping)
            return record(state.directory.single, remap(rule, index));
        return record(state.directory.single, index + 32);
    }
    if (!state.directory.wide) {
        if (!state.directory.single)
            throw Error(Boundary::Font, "message fallback font is absent");
        return state.directory.single->fallback;
    }
    const auto low = static_cast<int>(static_cast<std::uint32_t>(code) & 255U);
    const auto high = static_cast<int>((static_cast<std::uint32_t>(code) >> 8U) & 255U) - 129;
    return record(state.directory.wide, high * 188 + low - (low <= 126 ? 64 : 65));
}
std::int32_t advance(std::int32_t code, const State &state, const std::int32_t &language) {
    const auto &rule = language_rule(state, language);
    if (code >= 256)
        return 24;
    code = adjusted_code(code, rule);
    if (rule.remapping)
        code = remap(rule, code - 32);
    const auto index = static_cast<std::int64_t>(code) - 32;
    if (index < 0 || static_cast<std::size_t>(index) >= state.widths.size())
        throw Error(Boundary::Widths, "message advance reads outside its generated widths");
    return static_cast<std::int32_t>(state.widths[static_cast<std::size_t>(index)]) + 1;
}
void initialize(State &state, const std::int32_t &language, LoadCalls &calls) {
    state.loaded_wide = calls.load(FontFamily::Wide);
    state.loaded_single = calls.load(FontFamily::Single);
    state.directory.wide = state.loaded_wide;
    state.directory.single = state.loaded_single;
    for (unsigned index = 0; index < state.widths.size(); ++index)
        state.widths[index] = static_cast<std::int8_t>(
            measured_width(glyph(static_cast<int>(index) + 32, state, language)));
}
void put_character(menu_caption::Context &context, std::int32_t code, const State &state,
                   const std::int32_t &language) {
    if (!context.bitmap)
        return;
    if (code < 32) {
        if (code == 10) {
            context.cursor_y =
                cursor_step(context.cursor_y, static_cast<int>(context.spacing_y) + 24);
            context.cursor_x = 0;
        }
        return;
    }
    const auto &image = glyph(code, state, language);
    const auto width = advance(code, state, language);
    if (static_cast<std::int32_t>(context.cursor_x) >=
        static_cast<int>(context.buffer_width) - width) {
        context.cursor_y = cursor_step(context.cursor_y, static_cast<int>(context.spacing_y) + 24);
        context.cursor_x = 0;
    }
    auto original_y = static_cast<std::int32_t>(context.cursor_y);
    if (original_y < static_cast<int>(context.buffer_height) && original_y + 23 >= 0) {
        unsigned first_row = 0;
        unsigned rows = 24;
        auto physical_y = original_y;
        if (original_y < 0) {
            first_row = static_cast<unsigned>(-original_y);
            rows = static_cast<unsigned>(24 + original_y);
            physical_y = 0;
        }
        const auto half_alpha =
            static_cast<std::uint16_t>(((context.color >> 1U) & 28672U) | (context.color & 4095U));
        for (unsigned row = first_row; row < first_row + rows; ++row) {
            draw_row(context, image, row, physical_y, half_alpha);
            ++physical_y;
            ++original_y;
            if (original_y >= static_cast<int>(context.buffer_height))
                break;
        }
    }
    context.cursor_x = cursor_step(context.cursor_x, static_cast<int>(context.spacing_x) + width);
    context.flags |= 2;
}
void put_text(menu_caption::Context &context, std::string_view text, const State &state,
              const std::int32_t &language) {
    if (text.empty())
        return;
    if (static_cast<std::uint8_t>(text.front()) == 9 ||
        static_cast<std::uint8_t>(text.front()) == 7)
        throw Error(Boundary::TextLayout, "aligned message text has not been recovered");
    const auto &rule = language_rule(state, language);
    const auto first = rule.lead_first, last = rule.lead_last;
    for (std::size_t offset = 0; offset < text.size();) {
        auto code = static_cast<std::uint16_t>(static_cast<std::uint8_t>(text[offset++]));
        if (code == 0)
            return;
        if (code >= first && code <= last) {
            if (offset >= text.size())
                throw Error(Boundary::TextEncoding, "message ends during a two-byte character");
            code = static_cast<std::uint16_t>((code << 8U) |
                                              static_cast<std::uint8_t>(text[offset++]));
        }
        put_character(context, code, state, language);
    }
}
void request_texture_upload(menu_caption::Context &context) { context.flags |= 2; }
} // namespace sadx::message_text
