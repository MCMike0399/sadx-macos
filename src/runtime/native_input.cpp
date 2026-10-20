#include "runtime/native_input.hpp"
#include "input/input_mod_constants.hpp"
#include "player/scene_input_producer.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace sadx::native {
namespace {
void set_bit(std::uint32_t &value, std::uint32_t mask, bool down) {
    if (down)
        value |= mask;
    else
        value &= ~mask;
}
// Input mod static map initializer: source virtual keys -> vanilla SADX key indices.
const std::array<std::pair<std::uint32_t, std::uint8_t>, 102> vanilla_key_map{{
    {27, 41},   {112, 58},  {113, 59},  {114, 60},  {115, 61},  {116, 62},  {117, 63}, {118, 64},
    {119, 65},  {120, 66},  {121, 67},  {122, 68},  {123, 69},  {44, 70},   {145, 71}, {19, 72},
    {192, 99},  {49, 30},   {50, 31},   {51, 32},   {52, 33},   {53, 34},   {54, 35},  {55, 36},
    {56, 37},   {57, 38},   {48, 39},   {189, 45},  {187, 46},  {8, 42},    {45, 73},  {36, 74},
    {33, 75},   {144, 83},  {111, 84},  {106, 85},  {109, 86},  {9, 43},    {81, 20},  {87, 26},
    {69, 8},    {82, 21},   {84, 23},   {89, 28},   {85, 24},   {73, 12},   {79, 18},  {80, 19},
    {219, 100}, {221, 101}, {220, 102}, {46, 76},   {35, 77},   {34, 78},   {103, 95}, {104, 96},
    {105, 97},  {107, 87},  {20, 103},  {65, 4},    {83, 22},   {68, 7},    {70, 9},   {71, 10},
    {72, 11},   {74, 13},   {75, 14},   {76, 15},   {186, 51},  {222, 52},  {13, 40},  {100, 92},
    {101, 93},  {102, 94},  {160, 104}, {90, 29},   {88, 27},   {67, 6},    {86, 25},  {66, 5},
    {78, 17},   {77, 16},   {188, 54},  {190, 55},  {191, 56},  {161, 105}, {38, 82},  {97, 89},
    {98, 90},   {99, 91},   {162, 106}, {164, 107}, {32, 44},   {165, 108}, {93, 109}, {163, 110},
    {37, 80},   {40, 81},   {39, 79},   {96, 98},   {110, 111}, {256, 112},
}};
struct KeyboardStick {
    std::int16_t x = 0, y = 0;
    std::uint32_t directions = 0;
    // Input DLL's KeyboardStick::update: signed16 storage, int32 arithmetic.
    static std::int16_t advance(std::int16_t previous, std::uint32_t direction,
                                std::uint32_t negative, std::uint32_t positive) {
        const auto selected = direction & (negative | positive);
        if (selected == negative)
            return static_cast<std::int16_t>(std::max(static_cast<int>(previous) - 8192, -32767));
        if (selected == positive)
            return static_cast<std::int16_t>(std::min(static_cast<int>(previous) + 8192, 32767));
        if (previous < 0)
            return static_cast<std::int16_t>(std::min(static_cast<int>(previous) + 8192, 0));
        return static_cast<std::int16_t>(std::max(static_cast<int>(previous) - 8192, 0));
    }
    // FUNCTION: INPUT 0x10004cb0 KeyboardStick::update
    void update() {
        x = advance(x, directions, 64, 128);
        y = advance(y, directions, 16, 32);
    }
};
} // namespace
struct Input::Impl {
    InputConfiguration configuration;
    SseContext &sse;
    InputModPadConstants constants = input_mod_261_constants();
    InputModKeyboardPad keyboard;
    std::array<std::uint8_t, 6> vanilla_keys{};
    std::array<KeyboardKeyState, 256> keyboard_records{};
    std::uint8_t modifier_input = 0, modifier_held = 0, modifier_previous = 0, modifier_pressed = 0;
    std::array<KeyboardStick, 2> sticks{};
    bool keyboard_half_press = false, keyboard_center_camera = false, demo_active = false;
    std::array<InputModDreamPad, 8> pads{};
    std::array<InputModGamepadState, 8> samples{};
    InputModRawRecords raw_records;
    std::array<std::shared_ptr<GameLogicController>, 4> engine_raw;
    NativeSceneInputState scene;
    std::array<std::uint8_t, 8> enabled{{1, 1, 0, 0, 0, 0, 0, 0}};
    std::uint8_t pad_reading = 1; // Source load-time PadReadOn flag.

    Impl(const InputConfiguration &c, SseContext &context) : configuration(c), sse(context) {
        const auto initial = input::input_initial_state();
        for (std::size_t i = 0; i < pads.size(); ++i) {
            pads[i].record = GameLogicController::from_input_values(initial.dream_pads[i]);
            pads[i].normalized_l = Sse32{0};
            pads[i].normalized_r = Sse32{0};
            pads[i].settings = c.pads[i];
            pads[i].settings.allow_keyboard = i == c.keyboard_player;
        }
        for (auto &record : raw_records) {
            record = std::make_shared<GameLogicController>();
            *record = GameLogicController::from_input_values(initial.mod_raw[0]);
        }
        for (std::size_t i = 0; i < engine_raw.size(); ++i) {
            engine_raw[i] = std::make_shared<GameLogicController>();
            *engine_raw[i] = GameLogicController::from_input_values(initial.engine_raw[i]);
        }
        // Input mod Init explicitly enables entries0/1 after static BSS initialization.
    }
    // FUNCTION: SONIC 0x40ee10 KeyboardUpdate
    void update_vanilla_keyboard() {
        for (auto &key : keyboard_records) {
            key.previous = key.held;
            key.held = 0;
        }
        for (const auto index : vanilla_keys)
            keyboard_records[index].held = 1; // Zero slots mark key0 too.
        for (auto &key : keyboard_records)
            key.pressed = (key.previous ^ key.held) & key.held;
        modifier_previous = modifier_held;
        modifier_pressed = (modifier_held ^ modifier_input) & modifier_input;
        modifier_held = modifier_input;
    }
    bool poll_keyboard(std::string &error) {
        sticks[0].update();
        sticks[1].update();
        InputModAxesOutput left, right;
        normalize_input_mod_261_keyboard({sticks[0].x, sticks[0].y}, {sticks[1].x, sticks[1].y},
                                         constants.axes, sse, left, right);
        if (!left.x || !left.y || !left.magnitude || !right.x || !right.y || !right.magnitude) {
            error = "Input KeyboardMouse normalization did not produce complete axes";
            return false;
        }
        keyboard.lx = *left.x;
        keyboard.ly = *left.y;
        keyboard.rx = *right.x;
        keyboard.ry = *right.y;
        keyboard.normalized_l = *left.magnitude;
        keyboard.normalized_r = *right.magnitude;
        if (keyboard_half_press) {
            keyboard.lx /= 2;
            keyboard.ly /= 2;
            keyboard.rx /= 2;
            keyboard.ry /= 2;
            keyboard.normalized_l = sse.multiply(keyboard.normalized_l, constants.half_press_scale);
            keyboard.normalized_r = sse.multiply(keyboard.normalized_r, constants.half_press_scale);
        }
        keyboard.ltrigger = keyboard.held & 131072 ? 255 : 0;
        keyboard.rtrigger = keyboard.held & 65536 ? 255 : 0;
        update_vanilla_keyboard();
        return true;
    }
};
// FUNCTION: SONIC 0x40efe0 ConsumeSoftReset
std::uint32_t consume_soft_reset(SoftResetState &state) {
    const auto result = state.requested & 1U;
    if (result != 0) {
        state.requested = 0;
        state.neighbor = 0;
        state.metal_sonic = 0;
    }
    return result;
}
InputConfiguration normalize_configuration(const RawInputConfiguration &raw) {
    InputConfiguration result{};
    result.disable_mouse = raw.disable_mouse;
    result.keyboard_player = static_cast<std::uint8_t>(std::clamp(raw.keyboard_player, 0, 7));
    // Init stores WORD; KeyboardMouse then MOVSX-loads it before comparing a VK.
    const auto signed_word = [](std::int32_t value) {
        const auto low = static_cast<std::uint16_t>(value);
        return low < 32768 ? static_cast<std::int32_t>(low)
                           : static_cast<std::int32_t>(low) - 65536;
    };
    for (std::size_t index = 0; index < result.keys.size(); ++index)
        result.keys[index] = static_cast<std::uint32_t>(signed_word(raw.keys[index]));
    for (std::size_t index = 0; index < result.pads.size(); ++index) {
        const auto &source = raw.pads[index];
        result.pads[index] = {
            index == result.keyboard_player,
            static_cast<std::int16_t>(std::max(signed_word(source.deadzone_left), 0)),
            static_cast<std::int16_t>(std::max(signed_word(source.deadzone_right), 0)),
            source.radial_left,
            source.radial_right,
            static_cast<std::uint8_t>(source.trigger_threshold)};
    }
    return result;
}
PlayerInputProducerConstants controller_constants() {
    // SONIC scalar data:3072/-3072,magnitude scale/cap,angle units,+/-1/(2pi).
    // Exact bytes verified against the pinned original by the native-input oracle.
    return {3072.0f,
            -3072.0f,
            3.918702723968749e-14f,
            1.0f,
            65536.0,
            0.15915497620314795,
            -0.15915497620314795};
}
Input::Input(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
Input::~Input() = default;
std::unique_ptr<Input> Input::create(const InputConfiguration &config, SseContext &sse,
                                     std::string &error) {
    error.clear();
    if (!config.disable_mouse) {
        error = "Input KeyboardMouse mouse-enabled polling is not ported";
        return nullptr;
    }
    if (config.keyboard_player >= 8) {
        error = "Input configuration keyboard player is outside 0..7";
        return nullptr;
    }
    for (const auto &pad : config.pads)
        if (pad.deadzone_l < 0 || pad.deadzone_r < 0) {
            error = "Input configuration requires source-clamped controller deadzones";
            return nullptr;
        }
    return std::unique_ptr<Input>(new Input(std::make_unique<Impl>(config, sse)));
}
void Input::key_event(std::uint32_t key, bool down) {
    auto &state = *impl_;
    const auto mapped = std::find_if(vanilla_key_map.begin(), vanilla_key_map.end(),
                                     [&](const auto &entry) { return entry.first == key; });
    if (mapped != vanilla_key_map.end()) {
        for (auto &slot : state.vanilla_keys) {
            if (down && slot == 0) {
                slot = mapped->second;
                break;
            }
            if (!down && slot == mapped->second)
                slot = 0;
        }
    }
    const auto found =
        std::find(state.configuration.keys.begin(), state.configuration.keys.end(), key);
    if (found == state.configuration.keys.end())
        return;
    const auto binding = static_cast<KeyBinding>(found - state.configuration.keys.begin());
    if (binding == KeyBinding::HalfPress) {
        state.keyboard_half_press = down;
        return;
    }
    if (binding == KeyBinding::CenterCamera) {
        state.keyboard_center_camera = down;
        return;
    }
    static constexpr std::array<std::uint32_t, 14> masks{
        {4, 2, 1024, 512, 256, 1, 2048, 8, 131072, 65536, 16, 32, 64, 128}};
    const auto index = static_cast<std::size_t>(binding);
    if (index < static_cast<std::size_t>(KeyBinding::LeftUp)) {
        set_bit(state.keyboard.held, masks[index - 2], down);
        return;
    }
    const auto direction = index - static_cast<std::size_t>(KeyBinding::LeftUp);
    static constexpr std::array<std::uint32_t, 4> directions{{16, 32, 64, 128}};
    set_bit(state.sticks[direction / 4].directions, directions[direction % 4], down);
}
void Input::focus_lost() {
    auto &state = *impl_;
    state.sticks[0].directions = 0;
    state.sticks[1].directions = 0;
    state.vanilla_keys.fill(0);
    state.keyboard.held = 0;
    state.keyboard.lx = 0;
    state.keyboard.ly = 0;
    state.keyboard.rx = 0;
    state.keyboard.ry = 0;
    // Source WM_KILLFOCUS preserves ramp positions and half-press/e-held booleans.
}
void Input::gamepad(std::size_t player, const std::optional<InputModGamepadState> &sample) {
    auto &state = *impl_;
    auto &pad = state.pads.at(player);
    if (sample) {
        if (!pad.connected)
            input_mod_connect(pad);
        state.samples[player] = *sample;
    } else
        pad.connected = false; // Source removal retains its record and per-pad flags.
}
bool Input::poll(std::string &error) {
    error.clear();
    auto &state = *impl_;
    try {
        if (!state.poll_keyboard(error))
            return false;
        for (std::size_t i = 0; i < state.pads.size(); ++i) {
            const auto result = input_mod_poll(state.pads[i], state.keyboard, state.samples[i],
                                               false, state.constants, state.sse);
            if (!result) {
                error = "Input DreamPad poll failed at source boundary " +
                        std::to_string(result.source_boundary);
                return false;
            }
        }
        const auto published = input_mod_publish(state.pads, state.engine_raw, state.raw_records);
        if (!published) {
            error = "Input controller publication failed at source boundary " +
                    std::to_string(published.source_boundary);
            return false;
        }
        return true;
    } catch (const SseError &exception) {
        error = std::string("Input KeyboardMouse arithmetic failed: ") + exception.what();
        return false;
    }
}
bool Input::poll_frame(const InputFrameContext &context, SoftResetState &reset,
                       std::string &error) {
    error.clear();
    auto &state = *impl_;
    try {
        if (!state.poll_keyboard(error))
            return false;
        NativeInputModFrame frame{context.game_mode,     context.demo_playing, context.demo_enabled,
                                  context.demo_cutscene, state.demo_active,    reset.requested};
        NativeInputModOwners owners{state.pads, state.engine_raw, state.raw_records};
        const auto result = input_mod_on_input(owners, state.keyboard, state.samples, frame,
                                               state.constants, state.sse);
        state.demo_active = frame.demo_active;
        reset.requested = frame.soft_reset;
        if (!result) {
            error =
                "Input OnInput failed at source boundary " + std::to_string(result.source_boundary);
            return false;
        }
        return true;
    } catch (const SseError &exception) {
        error = std::string("Input OnInput arithmetic failed: ") + exception.what();
        return false;
    }
}
PadSnapshot Input::snapshot(std::size_t player) const {
    const auto &state = *impl_;
    const auto &pad = state.pads.at(player);
    return {pad.record.input_values().value(),
            pad.connected,
            pad.e_held ||
                (player == state.configuration.keyboard_player && state.keyboard_center_camera),
            pad.half_press,
            *pad.normalized_l,
            *pad.normalized_r};
}
std::array<std::uint8_t, 6> Input::keyboard_keys() const {
    return impl_->vanilla_keys;
}
KeyboardKeyState Input::key_state(std::size_t index) const {
    return impl_->keyboard_records.at(index);
}
// FUNCTION: SONIC 0x40ef20 KeyGetOn
std::uint8_t Input::check_key(std::size_t index) const {
    return impl_->keyboard_records.at(index).held;
}
// FUNCTION: SONIC 0x40ef30 KeyCtrlGetOn
std::uint8_t Input::switch_data() const {
    return impl_->modifier_held;
}
void Input::keyboard_modifiers(std::uint8_t bits) {
    impl_->modifier_input = bits;
}
// NOTSA: bind the existing PadReadOn service to its source flag owner.
void Input::enable_pad_reading() {
    impl_->pad_reading = 1;
}
// FUNCTION: SONIC 0x40ef50 PadReadOff
void Input::disable_pad_reading() {
    impl_->pad_reading = 0;
}
bool Input::update_controllers(std::uint16_t stage, const PlayerInputCamera &camera,
                               const PlayerInputProducerConstants &constants, X87Context &fp,
                               std::string &error) {
    error.clear();
    auto &state = *impl_;
    std::array<NativeScenePad, 8> pads;
    for (std::size_t index = 0; index < state.pads.size(); ++index) {
        const auto &pad = state.pads[index];
        float magnitude;
        const auto bits = pad.normalized_l->bits;
        std::memcpy(&magnitude, &bits, sizeof magnitude);
        pads[index] = {pad.connected, pad.settings.allow_keyboard,
                       pad.record.input_values().value(), magnitude};
    }
    try {
        const auto result =
            produce_native_scene_input(state.raw_records, pads, state.enabled, state.pad_reading,
                                       stage, camera, constants, state.scene, fp);
        if (!result) {
            error = "GetControllers failed at source boundary " +
                    std::to_string(result.source_boundary) + " nested " +
                    std::to_string(result.nested_source_boundary);
            return false;
        }
        return true;
    } catch (const X87Error &exception) {
        error = std::string("GetControllers arithmetic failed: ") + exception.what();
        return false;
    }
}

std::uint32_t Input::held(std::size_t player) const {
    return impl_->scene.held.at(player);
}
std::uint32_t Input::pressed(std::size_t player) const {
    return impl_->scene.pressed.at(player);
}
ControllerInputValues Input::engine_snapshot(std::size_t player) const {
    return impl_->scene.engine_pads.at(player);
}
std::uint32_t Input::published_held(std::size_t player) const {
    return impl_->scene.held_output.at(player);
}
AnalogSnapshot Input::analog_snapshot(std::size_t player) const {
    const auto &value = impl_->scene.normalized_analogs.at(player);
    return {value.angle, value.magnitude};
}
std::array<std::uint32_t, 5> Input::button_masks() const {
    return impl_->scene.masks;
}
} // namespace sadx::native
