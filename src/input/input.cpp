#include "input/input.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sadx::input {
namespace {
bool valid_action(Action action) noexcept {
    return static_cast<unsigned>(action) < static_cast<unsigned>(Action::count);
}
bool valid_scancode(int scancode) noexcept {
    return scancode > 0 && scancode < scancode_count;
}
bool valid_axis(float axis) noexcept {
    return std::isfinite(axis) && axis >= -1 && axis <= 1;
}
// Explicit native policy for platform-normalized snapshots. It does not apply
// the Input Mod raw-SDL deadzone and is never used for keyboard arithmetic.
float float_word(Sse32 value) noexcept {
    float result;
    std::memcpy(&result, &value.bits, sizeof(result));
    return result;
}
Sse32 binary_word(float value) noexcept {
    Sse32 result;
    std::memcpy(&result.bits, &value, sizeof(value));
    return result;
}
AnalogState normalize_native_controller_snapshot(Stick stick,
    const InputModAxesConstants& constants, SseContext& arithmetic) {
    const auto x = binary_word(stick.x), y = binary_word(stick.y);
    const auto yy = arithmetic.multiply(y, y);
    const auto xx = arithmetic.multiply(x, x);
    const auto sum = arithmetic.add(yy, xx);
    const auto wide = arithmetic.to_double(sum);
    const auto length = arithmetic.to_single(arithmetic.sqrt(wide));
    if (arithmetic.comiss(constants.epsilon, length) == SseComparison::greater)
        return {};
    auto nx = arithmetic.divide(x, length);
    auto ny = arithmetic.divide(y, length);
    nx = arithmetic.multiply(nx, constants.keyboard_scale);
    ny = arithmetic.multiply(ny, constants.keyboard_scale);
    const auto magnitude = arithmetic.minimum(length, constants.magnitude_cap);
    return {static_cast<std::int16_t>(arithmetic.truncate_int32(nx)),
            static_cast<std::int16_t>(arithmetic.truncate_int32(ny)),
            float_word(magnitude)};
}
std::int32_t ramp(std::int32_t previous, int direction,
                  KeyboardProfile profile) noexcept {
    constexpr std::int32_t maximum = 32767;
    constexpr std::int32_t increment = 8192;
    if (profile == KeyboardProfile::immediate) return direction * maximum;
    if (direction < 0) return std::max(previous - increment, -maximum);
    if (direction > 0) return std::min(previous + increment, maximum);
    return previous < 0 ? std::min(previous + increment, 0)
                        : std::max(previous - increment, 0);
}
} // namespace

std::uint32_t action_mask(Action action) noexcept {
    switch (action) {
    case Action::jump: return 0x4;       // Buttons_A
    case Action::action: return 0x2;     // Buttons_B
    case Action::pickup: return 0x400;   // Buttons_X
    case Action::whistle: return 0x200;  // Buttons_Y
    case Action::pause: return 0x8;      // Buttons_Start
    case Action::camera_left: return 0x20000;  // Buttons_L
    case Action::camera_right: return 0x10000; // Buttons_R
    default: return 0; // Stick directions are axes, not invented action bits.
    }
}

std::vector<KeyboardBinding> default_keyboard_bindings() {
    return {{80, Action::move_left}, {79, Action::move_right},
            {82, Action::move_up}, {81, Action::move_down},
            {13, Action::look_left}, {15, Action::look_right},
            {12, Action::look_up}, {16, Action::look_down},
            {27, Action::jump}, {29, Action::action}, {4, Action::pickup},
            {22, Action::whistle}, {40, Action::pause},
            {20, Action::camera_left}, {26, Action::camera_right}};
}

std::vector<ControllerBinding> default_controller_bindings() {
    return {{PhysicalButton::south, Action::jump},
            {PhysicalButton::east, Action::action},
            {PhysicalButton::west, Action::pickup},
            {PhysicalButton::north, Action::whistle},
            {PhysicalButton::start, Action::pause},
            {PhysicalButton::left_trigger, Action::camera_left},
            {PhysicalButton::right_trigger, Action::camera_right}};
}

const char* face_label(ControllerFamily family, PhysicalButton position) noexcept {
    if (family != ControllerFamily::xbox && family != ControllerFamily::switch2)
        return nullptr;
    const bool xbox = family == ControllerFamily::xbox;
    switch (position) {
    case PhysicalButton::south: return xbox ? "A" : "B";
    case PhysicalButton::east: return xbox ? "B" : "A";
    case PhysicalButton::west: return xbox ? "X" : "Y";
    case PhysicalButton::north: return xbox ? "Y" : "X";
    default: return nullptr;
    }
}

InputState::InputState(KeyboardProfile profile) : keyboard_profile_(profile) {
    bind_keyboard(default_keyboard_bindings());
    bind_controller(default_controller_bindings());
}

bool InputState::bind_keyboard(const std::vector<KeyboardBinding>& bindings) {
    decltype(keyboard_bindings_) candidate{};
    for (const auto& binding : bindings) {
        if (!valid_scancode(binding.scancode) || !valid_action(binding.action) ||
            candidate[binding.scancode].has_value()) return false;
        candidate[binding.scancode] = binding.action;
    }
    keyboard_bindings_ = candidate;
    return true;
}

bool InputState::bind_controller(const std::vector<ControllerBinding>& bindings) {
    decltype(controller_bindings_) candidate{};
    for (const auto& binding : bindings) {
        const auto index = static_cast<unsigned>(binding.button);
        if (index >= candidate.size() || !valid_action(binding.action) || action_mask(binding.action) == 0 ||
            candidate[index].has_value()) return false;
        candidate[index] = binding.action;
    }
    controller_bindings_ = candidate;
    return true;
}

bool InputState::keyboard_event(int scancode, bool down, bool autorepeat) noexcept {
    if (!valid_scancode(scancode)) return false;
    if (!autorepeat) keys_[scancode] = down;
    return true;
}

bool InputState::normalized_controller_snapshot(const NormalizedControllerSnapshot& sample) noexcept {
    if (!valid_axis(sample.left.x) || !valid_axis(sample.left.y) ||
        !valid_axis(sample.right.x) || !valid_axis(sample.right.y)) return false;
    controller_ = sample;
    return true;
}

void InputState::focus_lost() noexcept {
    keys_.fill(false);
    // Input Mod's WM_KILLFOCUS clears keyboard output/directions, not its
    // KeyboardStick x/y ramp or an independent SDL controller's state.
    if (keyboard_profile_ == KeyboardProfile::immediate) {
        controller_ = {};
        keyboard_axes_.fill(0);
    }
}

KeyboardRawSticks InputState::keyboard_raw_sticks() const noexcept {
    return {{static_cast<std::int16_t>(keyboard_axes_[0]), static_cast<std::int16_t>(keyboard_axes_[1])},
            {static_cast<std::int16_t>(keyboard_axes_[2]), static_cast<std::int16_t>(keyboard_axes_[3])}};
}

TickInput InputState::sample_tick(SseContext& arithmetic, const InputModAxesConstants& constants) {
    (void)arithmetic.mxcsr(); // Validate thread ownership before raw-state mutation.
    std::array<bool, static_cast<unsigned>(Action::count)> active{};
    for (unsigned i = 0; i < keys_.size(); ++i) {
        if (keys_[i] && keyboard_bindings_[i])
            active[static_cast<unsigned>(*keyboard_bindings_[i])] = true;
    }
    const auto on = [&](Action action) { return active[static_cast<unsigned>(action)]; };
    const std::array<int, 4> directions{
        on(Action::move_right) - on(Action::move_left),
        on(Action::move_down) - on(Action::move_up),
        on(Action::look_right) - on(Action::look_left),
        on(Action::look_down) - on(Action::look_up)};
    for (unsigned i = 0; i < keyboard_axes_.size(); ++i)
        keyboard_axes_[i] = ramp(keyboard_axes_[i], directions[i], keyboard_profile_);
    const auto raw = keyboard_raw_sticks();
    InputModAxesOutput normalized_left, normalized_right;
    normalize_input_mod_261_keyboard(raw.left, raw.right, constants, arithmetic,
                                    normalized_left, normalized_right);
    const auto convert = [](const InputModAxesOutput& output) {
        return AnalogState{*output.x, *output.y, float_word(*output.magnitude)};
    };
    const auto keyboard_left = convert(normalized_left);
    const auto keyboard_right = convert(normalized_right);
    TickInput result;
    // Existing native arbitration: nonzero keyboard axes take priority per
    // stick. Full original poll controller/demo/half-press phases are excluded.
    const auto keyboard_source = keyboard_profile_ == KeyboardProfile::input_mod_261
        ? AnalogSource::input_mod_261_keyboard : AnalogSource::immediate_keyboard;
    if (keyboard_left.x || keyboard_left.y) {
        result.left = keyboard_left;
        result.left_source = keyboard_source;
    } else result.left = normalize_native_controller_snapshot(controller_.left, constants, arithmetic);
    if (keyboard_right.x || keyboard_right.y) {
        result.right = keyboard_right;
        result.right_source = keyboard_source;
    } else result.right = normalize_native_controller_snapshot(controller_.right, constants, arithmetic);
    for (unsigned i = 0; i < controller_.buttons.size(); ++i) {
        if (controller_.buttons[i] && controller_bindings_[i])
            active[static_cast<unsigned>(*controller_bindings_[i])] = true;
    }
    for (unsigned i = 0; i < active.size(); ++i) {
        if (active[i]) result.held |= action_mask(static_cast<Action>(i));
    }
    result.pressed = result.held & ~previous_held_;
    result.released = previous_held_ & ~result.held;
    previous_held_ = result.held;
    result.mxcsr_after_sampling = arithmetic.mxcsr();
    return result;
}

} // namespace sadx::input
