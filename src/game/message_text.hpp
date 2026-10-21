#pragma once
#include "game/menu_caption.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace sadx::message_text {
struct Glyph {
    std::array<std::uint8_t, 72> bitmap{};
};
struct FontData {
    std::vector<Glyph> glyphs;
    // Source wide-font fallback reads the first72 file bytes, including its
    // record metadata.
    Glyph fallback;
};
struct LanguageRule {
    std::uint8_t lead_first = 0, lead_last = 0;
    std::shared_ptr<const std::vector<std::uint8_t>> remapping{};
};
struct Directory {
    std::array<LanguageRule, 5> languages{
        {{128, 255, {}}, {128, 255, {}}, {0, 0, {}}, {0, 0, {}}, {0, 0, {}}}};
    std::shared_ptr<const FontData> single{}, wide{};
};
struct State {
    // GLOBAL: SONIC 0x3abe130 fontdata0 size=4
    std::shared_ptr<const FontData> loaded_wide = {};
    // GLOBAL: SONIC 0x3abe138 fontdata1 size=4
    std::shared_ptr<const FontData> loaded_single = {};
    // GLOBAL: SONIC 0x909f30 MessageFontDirectory size=100
    Directory directory = {};
    // GLOBAL: SONIC 0x3abe250 MessageFontWidths size=224
    std::array<std::int8_t, 224> widths = {};
};
enum class FontFamily { Wide, Single };
struct LoadCalls {
    virtual ~LoadCalls() = default;
    virtual std::shared_ptr<const FontData> load(FontFamily) = 0;
};
enum class Boundary { Language, Mapping, Glyph, Widths, Font, Bitmap, TextLayout, TextEncoding };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, const char *message)
        : std::runtime_error(message), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }

  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x40d8c0 GetMessageGlyph
const Glyph &glyph(std::int32_t code, const State &, const std::int32_t &language);
// FUNCTION: SONIC 0x40d970 GetMessageGlyphAdvance
std::int32_t advance(std::int32_t code, const State &, const std::int32_t &language);
// FUNCTION: SONIC 0x40e2e0 InitMessageFonts
void initialize(State &, const std::int32_t &language, LoadCalls &);
// FUNCTION: SONIC 0x40d9f0 MSG_PutCharacter
void put_character(menu_caption::Context &, std::int32_t code, const State &,
                   const std::int32_t &language);
// FUNCTION: SONIC 0x40e570 MSG_Puts
void put_text(menu_caption::Context &, std::string_view, const State &,
              const std::int32_t &language);
// FUNCTION: SONIC 0x40d290 MSG_LoadTexture
void request_texture_upload(menu_caption::Context &);
} // namespace sadx::message_text
