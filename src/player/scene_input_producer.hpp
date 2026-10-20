#pragma once
#include "engine/game_controller.hpp"
#include "player/player_input_producer.hpp"
#include <functional>
namespace sadx {
struct SceneDirectionalKeyState {
    std::array<std::optional<std::uint32_t>,4> held_work_at_3b0e3a8,pressed_work_at_3b0e354;
    std::array<std::optional<std::int32_t>,4> repeat_at_3b0e3bc;
    std::array<std::optional<std::uint32_t>,4> held_out_at_3b0e7e0,pressed_out_at_3b0e3cc;
};
enum class SceneInputFailure {none,unknown_controller_reference,null_controller,
    unknown_controller_word,unknown_held_work,unknown_pressed_work,unknown_repeat,
    unknown_control_enabled,unknown_stage,unknown_controller_enabled,
    controller_index_unmapped,analog_producer_failed,missing_analog_owner,directional_failed};
struct SceneInputResult {
    SceneInputFailure failure=SceneInputFailure::none;std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::size_t slot=0,word_index=0,copied_words=0,cleared_records=0;
    std::optional<PlayerInputProducerResult> analogs;
    std::optional<SceneInputFailure> directional_failure;
    bool completed=false;
    explicit operator bool()const noexcept{return failure==SceneInputFailure::none&&completed;}
};
// Exact4146b0 outcome for the caller's existing stage owner. This classifies
// source stages only and supplies no guessed startup stage.
bool scene_input_hub_stage(std::uint16_t)noexcept;
// Borrowed40fdc0 owners. Raw ControllerPointers[0..3] and the eight engine pads
// at3b0e7f0 are distinct storage; slots0/1 copy raw bytes (including unknown
// bytes) only when the slot's controller-enabled byte (see below) is exactly 1.
struct SceneInputProducerState {
    const GameLogicControllers& raw_controllers;              // 3b0e77c
    GameLogicEnginePads& engine_pads;                         // 3b0e7f0
    GameControllerButtonMasks& masks;                         // 909f94..909fac
    SceneDirectionalKeyState& directional;                    // 40f070 work/output
    const std::optional<std::uint16_t>& stage_at_3b22dcc;     // read by4146b0
    const std::optional<std::uint8_t>& control_enabled_at_909fb0;
    // The byte 40fe0b/40fe2d compare with 1 for slots 0/1: originally 909fb4/909fb5; under the
    // pinned Input mod (its Init WriteData at 40fe0d/40fe2f) input::controller_enabled[0/1]
    // at 1000e73c/1000e73d, the table its EnableController/DisableController maintain.
    std::function<std::optional<std::uint8_t>(std::size_t slot)> controller_enabled;
};
struct SceneInputProducerRuntime {
    SceneInputProducerState& state;
    // Actual40f170 owner selected by the profile (original WriteAnalogs or the
    // Input2.6.1 return hook). Missing owner is a required boundary.
    std::function<PlayerInputProducerResult(X87Context&)> analog_owner;
};
SceneInputResult update_scene_directional_keys(std::uint32_t,const GameLogicControllers&,
    SceneDirectionalKeyState&,const std::optional<std::uint8_t>& control_enabled); //40f070
// Full local40fdc0: classifier and ordered mask stores, engine pad copy/clear,
// then40f170 and40f070 slots0..3. Earlier stores survive later failures.
SceneInputResult produce_scene_input(SceneInputProducerRuntime&,X87Context&);
// NOTSA: complete native owners. The older optional record API remains the
// reference adapter; its conversion is confined to this controller boundary.
struct NativeAnalogInput {
    std::int32_t angle = 0;
    float magnitude = 0;
};
struct NativeSceneInputState {
    std::array<ControllerInputValues, 8> engine_pads{};
    std::array<std::uint32_t, 4> held{}, pressed{}, held_output{}, pressed_output{};
    std::array<std::int32_t, 4> repeat{};
    std::array<NativeAnalogInput, 8> raw_analogs{}, normalized_analogs{};
    // Source load-image values, in GameControllerButtonMasks declaration order.
    std::array<std::uint32_t, 5> masks{{1026, 4, 1026, 512, 512}};
};
struct NativeScenePad {
    bool connected, keyboard_allowed;
    ControllerInputValues input;
    float magnitude;
};
SceneInputResult produce_native_scene_input(
    const std::array<std::shared_ptr<GameLogicController>, 8>& raw,
    const std::array<NativeScenePad, 8>& pads,
    const std::array<std::uint8_t, 8>& enabled, std::uint8_t pad_reading,
    std::uint16_t stage, const PlayerInputCamera&, const PlayerInputProducerConstants&,
    NativeSceneInputState&, X87Context&);
} // namespace sadx
