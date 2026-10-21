#pragma once
#include "cpu/x87_math.hpp"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace sadx::character_select_labels {
enum class Texture { Labels, Names, MetalName };
struct State {
    const float &horizontal_scale, &vertical_scale;
    const std::int32_t &text_language;
    const std::uint8_t &variant;
    const std::array<std::uint8_t, 7> &label_textures, &name_textures;
};
enum class Boundary {
    // Source indexed reads; unsupported indices have no imported declaration.
    LabelIndex, // SONIC 0x50713d
    NameIndex   // SONIC 0x50728c
};
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::int32_t index)
        : std::runtime_error("character selection texture index has no imported declaration"),
          boundary_(boundary), index_(index) {}
    Boundary boundary() const { return boundary_; }
    std::int32_t index() const { return index_; }
  private:
    Boundary boundary_;
    std::int32_t index_;
};
struct Calls {
    virtual ~Calls() = default;
    virtual void select_texture(Texture) = 0;
    virtual void material(std::uint32_t color) = 0;
    virtual void draw_texture(std::uint32_t texture, float x, float y, float depth) = 0;
    virtual void draw_scaled(std::uint32_t texture, float x, float y, float depth,
                             float scale_x, float scale_y) = 0;
};
// FUNCTION: SONIC 0x507050 Menu_CharacterLabel
void draw_label(std::int32_t character, std::uint32_t color, float x, float y, float depth,
                State, Calls &, X87Context &);
// FUNCTION: SONIC 0x5071d0 Menu_CharacterName
void draw_name(std::int32_t character, std::uint32_t color, float x, float y, float depth,
               float scale_x, float scale_y, State, Calls &, X87Context &);
} // namespace sadx::character_select_labels
