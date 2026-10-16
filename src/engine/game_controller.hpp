#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
namespace sadx {
// One source52-byte PDS-style record. Raw ControllerPointers and engine pads
// are separate owning collections; this record never maps physical key codes.
// Pointer-valued source words (+0x24/+0x28/+0x30) stay opaque bytes, not native pointers.
class GameLogicController {
public:
    static constexpr std::size_t byte_count=52;
    // Source field offsets used by consumers (SDK ControllerData names).
    static constexpr std::size_t held_offset=0x08,pressed_offset=0x10,
        stick_x_offset=0x1c,stick_y_offset=0x1e,old_offset=0x2c;
    std::optional<std::uint8_t> read_byte(std::size_t)const noexcept;
    std::optional<std::uint16_t> read_u16(std::size_t)const noexcept;
    std::optional<std::uint32_t> read_u32(std::size_t)const noexcept;
    bool write_byte(std::size_t,std::optional<std::uint8_t>)noexcept;
    bool write_u16(std::size_t,std::uint16_t)noexcept;
    bool write_u32(std::size_t,std::uint32_t)noexcept;
    bool forget(std::size_t,std::size_t)noexcept;
private:
    std::array<std::optional<std::uint8_t>,byte_count> bytes_;
};
using GameLogicControllers=std::array<std::optional<std::shared_ptr<GameLogicController>>,4>;
struct GameLogicEnginePads {std::array<GameLogicController,8> records;};
struct GameControllerButtonMasks {
    std::optional<std::uint32_t> mask_at_909f94,mask_at_909f98,mask_at_909fa4,mask_at_909fa8,mask_at_909fac;
};
// Other globals cleared by InitControllers 40f3a0 (exact store set).
struct GameControllerInitGlobals {
    std::array<std::optional<std::uint32_t>,4> words_at_3b0e768,words_at_3b0e344,words_at_3b0e9b8;
    std::array<std::optional<std::uint8_t>,4> bytes_at_3b0e9a4;
    std::array<std::optional<std::uint32_t>,5> words_at_3b0e990; // 990,994,998,99c,9a0
};
// Vanilla 40f3a0: copy the template record (3b0eaa8) to ControllersRaw[0],
// replicate it into records 1..3 (overlapping forward 39-dword copy), point
// ControllerPointers[0..3] at ControllersRaw[0..3] and clear the globals above.
// Unknown template bytes stay unknown. Returns false (nothing written beyond
// the first globals) only when a ControllersRaw record is not allocated.
bool init_controllers_40f3a0(const GameLogicController& template_at_3b0eaa8,
    const std::array<std::shared_ptr<GameLogicController>,4>& controllers_raw_at_3b0e9c8,
    GameLogicControllers& controller_pointers_at_3b0e77c,GameControllerInitGlobals&);
} // namespace sadx
