#include "player/gamma_setup_resource.hpp"

#include <cstring>
#include <stdexcept>

namespace sadx::gamma_setup {
namespace {
// NOTSA: explicit little-endian pack reader; never reinterpret imported pointers.
class Reader {
  public:
    explicit Reader(const std::vector<std::uint8_t> &bytes) : bytes_(bytes) {}
    std::uint16_t half() {
        const auto low = byte();
        return static_cast<std::uint16_t>(low | (std::uint16_t(byte()) << 8));
    }
    std::uint32_t word() {
        const auto low = half();
        return low | (std::uint32_t(half()) << 16);
    }
    float scalar() {
        const auto bits = word();
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    std::int16_t signed_half() {
        const auto value = half();
        return static_cast<std::int16_t>(value <= 32767 ? value : int(value) - (1 << 16));
    }
  private:
    std::uint8_t byte() {
        if (position_ >= bytes_.size())
            throw std::runtime_error("gamma_setup_import_truncated");
        return bytes_[position_++];
    }
    const std::vector<std::uint8_t> &bytes_;
    std::size_t position_ = 8;
};
} // namespace

Asset decode(const std::vector<std::uint8_t> &bytes, const texture_lifetime::List &textures) {
    constexpr std::size_t kPackBytes = 244;
    constexpr char kMagic[] = "SDXGSP01";
    if (bytes.size() != kPackBytes || std::memcmp(bytes.data(), kMagic, 8) != 0)
        throw std::runtime_error("gamma_setup_import_format");
    Reader input(bytes);
    if (input.word() != 12 || textures.entries.size() != 12)
        throw std::runtime_error("gamma_setup_E102EFFECT_texture_list");
    Asset asset{};
    auto &definition = asset.definition;
    definition.textures = &textures;
    definition.animation_count = input.half();
    definition.count_padding = input.half();
    definition.source_blend = input.word();
    definition.destination_blend = input.word();
    if (definition.animation_count != 8 || definition.count_padding != 0)
        throw std::runtime_error("gamma_setup_import_animation_count");
    auto &parameters = asset.parameters;
    parameters.leading_parameter = input.scalar();
    parameters.scale_growth = input.scalar();
    parameters.animation_speed = input.scalar();
    parameters.drag = input.scalar();
    parameters.vertical_acceleration = input.scalar();
    for (auto &value : parameters.remaining)
        value = input.scalar();
    for (unsigned index = 0; index < definition.animation_count; ++index) {
        sprite::AnimationFrame frame{};
        frame.width = input.signed_half();
        frame.height = input.signed_half();
        frame.center_x = input.signed_half();
        frame.center_y = input.signed_half();
        frame.u_begin = input.signed_half();
        frame.v_begin = input.signed_half();
        frame.u_end = input.signed_half();
        frame.v_end = input.signed_half();
        frame.texture_index = input.signed_half();
        frame.attributes = input.signed_half();
        definition.animation.push_back(frame);
    }
    return asset;
}

void load_resource(State &state, sprite_registry::State &registry, TaskWorkArena &arena) {
    // Capture before CAlloc can reenter. Repeated setup overwrites only the
    // Gamma result; earlier nodes remain registered until the shared cleanup.
    const auto *definition = state.definition;
    state.sprite = sprite_registry::register_sprite(registry, arena, definition, 0);
}
} // namespace sadx::gamma_setup
