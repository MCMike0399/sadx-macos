#include "game/menu_dialog.hpp"

namespace sadx::menu_dialog {
namespace {
// NOTSA: preserves genuine missing task/work boundaries without caching another owner.
std::shared_ptr<Work> current_work(Owner owner) {
    if (!owner.task)
        throw Error(Boundary::MissingTask, "dialog request has no canonical task");
    auto work = std::static_pointer_cast<Work>(owner.task->auxiliary_native_work);
    if (!work)
        throw Error(Boundary::MissingWork, "dialog request has no canonical auxiliary work");
    return work;
}
// NOTSA: scans only resource bytes known to belong to the source disabled-choice array.
bool disabled_choice(const Work &work, std::int8_t choice) {
    bool found = false;
    for (const auto entry : work.choice_ids) {
        if (entry < 0)
            return found;
        if (entry == choice)
            found = true;
    }
    throw Error(Boundary::DisabledTerminator,
                "dialog disabled-choice list has no known terminator");
}
} // namespace
void set_cursor(std::int8_t cursor, Owner owner) {
    const auto work = current_work(owner);
    if (!work->parameters)
        throw Error(Boundary::MissingWork, "dialog cursor has no parameters");
    const auto count = work->parameters->choice_count;
    if (count > 0) {
        for (unsigned remaining = static_cast<std::uint8_t>(count); remaining != 0; --remaining) {
            const auto disabled = disabled_choice(*work, cursor);
            if (static_cast<std::int32_t>(cursor) != work->previous_choice && disabled) {
                cursor = static_cast<std::int8_t>(static_cast<std::uint8_t>(cursor) + 1U);
                if (cursor >= count)
                    cursor = 0;
            }
        }
    }
    work->cursor = static_cast<std::uint8_t>(cursor);
}
void open_with_cursor(const Parameters &parameters, std::int8_t cursor, Choices disabled,
                      Owner owner, Calls &calls) {
    const auto work = current_work(owner);
    work->parameters = &parameters;
    work->phase = 1;
    if (disabled.data) {
        for (std::size_t index = 0;; ++index) {
            if (index >= disabled.size)
                throw Error(Boundary::DisabledTerminator,
                            "dialog choice resource ends before its terminator");
            if (index >= work->choice_ids.size())
                throw Error(Boundary::DisabledCapacity,
                            "dialog choice copy exceeds recovered work storage");
            work->choice_ids[index] = disabled.data[index];
            // The source reads again after its store; the input may alias the destination.
            if (disabled.data[index] < 0)
                break;
        }
    } else {
        work->choice_ids.fill(-1);
    }
    work->previous_choice = -1;
    set_cursor(cursor, owner);
    work->repeat_frames = 0;
    if (parameters.sounds)
        calls.play_sound(parameters.sounds->open);
}
void open(const Parameters &parameters, Owner owner, Calls &calls) {
    open_with_cursor(parameters, 0, {}, owner, calls);
}
void request(std::int32_t dialog, const Catalog &catalog, Owner owner, Calls &calls) {
    if (dialog < 0 || static_cast<std::size_t>(dialog) >= catalog.size())
        throw Error(Boundary::DialogIndex, "requested dialog has no imported source definition");
    open(catalog[static_cast<std::size_t>(dialog)], owner, calls);
}
std::int32_t result(Owner owner) {
    if (owner.task) {
        const auto work = std::static_pointer_cast<Work>(owner.task->auxiliary_native_work);
        if (work) {
            if (work->phase > 4)
                return static_cast<std::int8_t>(work->cursor);
            return -1;
        }
    }
    return 255;
}
} // namespace sadx::menu_dialog

#include "cpu/fidelity_float.hpp"
namespace sadx::menu_dialog {
bool is_disabled(std::int8_t cursor, const Work &work, Owner owner) {
    const auto found = disabled_choice(work, cursor);
    return static_cast<std::int32_t>(cursor) == current_work(owner)->previous_choice ? false
                                                                                     : found;
}
std::int8_t next_choice(std::int8_t cursor, std::int8_t count, const Work &work, Owner owner) {
    if (static_cast<int>(cursor) == static_cast<int>(count) - 1)
        return cursor;
    const auto original = cursor;
    const auto override_choice = current_work(owner)->previous_choice;
    for (;;) {
        cursor = static_cast<std::int8_t>(static_cast<std::uint8_t>(cursor) + 1U);
        if (static_cast<std::int32_t>(cursor) == override_choice || !disabled_choice(work, cursor))
            return cursor;
        if (static_cast<int>(cursor) >= static_cast<int>(count) - 1)
            return original;
    }
}
std::int8_t previous_choice(std::int8_t cursor, const Work &work, Owner owner) {
    if (cursor <= 0)
        return cursor;
    const auto original = cursor;
    const auto override_choice = current_work(owner)->previous_choice;
    for (;;) {
        cursor = static_cast<std::int8_t>(static_cast<std::uint8_t>(cursor) - 1U);
        if (static_cast<std::int32_t>(cursor) == override_choice || !disabled_choice(work, cursor))
            return cursor;
        if (cursor <= 0)
            return original;
    }
}
namespace {
// NOTSA: named repeated counter step of opening/closing; signed division precedes16-bit wrap.
void advance_animation(Work &work, std::uint32_t ticks) {
    const auto step = ticks == 1 ? 1 : static_cast<std::int32_t>(ticks) / 2;
    work.opening_frames = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(work.opening_frames) + static_cast<std::uint32_t>(step));
}
// NOTSA: bounds the actual imported16-row animation resource, not the game state.
const AnimationScale &animation_scale(const std::array<AnimationScale, 16> &table,
                                      std::int16_t frame) {
    if (frame < 0 || static_cast<unsigned>(frame) >= table.size())
        throw Error(Boundary::AnimationIndex,
                    "dialog animation reads outside the imported source table");
    return table[static_cast<std::size_t>(frame)];
}
// NOTSA: genuine absent parameters fail at the active source read, not at inactive task entry.
const Parameters &require_parameters(const Parameters *parameters) {
    if (!parameters)
        throw Error(Boundary::MissingParameters, "active dialog has no parameters");
    return *parameters;
}
// NOTSA: named source selection step; all action paths share its close/sound stores.
void decide(Work &work, const Parameters &parameters, Owner owner, std::uint32_t pressed,
            UpdateCalls &calls) {
    if ((pressed & 12U) != 0) {
        const auto choice = static_cast<std::int8_t>(work.cursor);
        if (!is_disabled(choice, work, owner)) {
            work.opening_frames = 0;
            work.phase = 3;
            if (parameters.sounds)
                calls.play_sound(choice == parameters.cancel_choice ? parameters.sounds->cancel
                                                                    : parameters.sounds->decide);
        }
    } else if ((pressed & 1026U) != 0) {
        work.cursor = static_cast<std::uint8_t>(parameters.cancel_choice);
        work.opening_frames = 0;
        work.phase = 3;
        if (parameters.sounds)
            calls.play_sound(parameters.sounds->cancel);
    }
}
// NOTSA: geometric three-way and nearest-point layouts are distinct original navigation cases.
bool navigate(Work &work, const Parameters &parameters, const Tables &tables, Owner owner,
              std::uint32_t pressed) {
    auto cursor = static_cast<std::int8_t>(work.cursor);
    const auto style = static_cast<std::int32_t>(parameters.style);
    if (style == 2) {
        if ((pressed & 240U) != 0)
            throw Error(Boundary::NavigationStyle,
                        "dialog nearest-point navigation is not recovered");
        return false;
    }
    if (style == 3) {
        if ((pressed & 128U) != 0 && cursor != 1)
            cursor = 1;
        else if ((pressed & 64U) != 0 && cursor != 0)
            cursor = 0;
        else if ((pressed & 16U) != 0 && cursor == 2)
            cursor = 0;
        else if ((pressed & 32U) != 0 && cursor != 2)
            cursor = 2;
        else
            return false;
    } else {
        if (style < 0 || static_cast<std::size_t>(style) >= tables.directions.size())
            throw Error(Boundary::NavigationStyle, "dialog style has no imported direction masks");
        const auto &direction = tables.directions[static_cast<std::size_t>(style)];
        if ((pressed & direction.previous) != 0 && cursor > 0)
            cursor = previous_choice(cursor, work, owner);
        else if ((pressed & direction.next) != 0 &&
                 static_cast<int>(cursor) < static_cast<int>(parameters.choice_count) - 1)
            cursor = next_choice(cursor, parameters.choice_count, work, owner);
        else
            return false;
    }
    work.cursor = static_cast<std::uint8_t>(cursor);
    return true;
}
// NOTSA: named long-function phase, retaining the source's captured parameters and live cursor.
void control(Work &work, const Parameters *captured, Owner owner, const Tables &tables,
             UpdateCalls &calls, const NinjaMath &math, X87Context &fp) {
    work.color = 4294967295U;
    work.scale_y = 1.0f;
    work.scale_x = 1.0f;
    const auto &parameters = require_parameters(captured);
    const auto style = static_cast<std::int32_t>(parameters.style);
    if (style >= 0 && style <= 5) {
        if (!work.animation_angle)
            throw Error(Boundary::AnimationCounter,
                        "dialog pulse reads indeterminate animation counter");
        const auto angle = static_cast<std::int32_t>(work.blink_step * *work.animation_angle);
        const auto cosine = fidelity::ExactFloat::retained(fp, math.cos_extended(angle));
        const auto progress = ((cosine + 1.0f) * 0.5f).rounded();
        const auto first = style == 1 || style == 4 ? 4279416063U : 4278190335U;
        work.upper_color = title_menu::interpolate_colors(first, 4294901760U, progress, fp);
    }
    const auto previous = work.cursor;
    const auto pressed = calls.pressed_buttons();
    if (!navigate(work, parameters, tables, owner, pressed))
        decide(work, parameters, owner, pressed, calls);
    if (previous == work.cursor) {
        if (work.repeat_frames < 32767)
            ++work.repeat_frames;
    } else {
        work.repeat_frames = 0;
        work.fade = 1.5f;
        if (parameters.sounds)
            calls.play_sound(parameters.sounds->move);
    }
}
} // namespace
void update(Work &work, Owner owner, const Tables &tables, const std::uint32_t &logic_ticks,
            UpdateCalls &calls, const NinjaMath &math, X87Context &fp) {
    const auto captured = work.parameters;
    const auto canonical = current_work(owner);
    const auto state = title_menu::update_dialog(work, *canonical, logic_ticks, fp);
    using title_menu::DialogUpdateResult;
    switch (state) {
    case DialogUpdateResult::Complete:
        return;
    case DialogUpdateResult::NeedsOpening: {
        work.color = 4294967295U;
        const auto &scale = animation_scale(tables.opening, work.opening_frames);
        work.scale_x = scale.x;
        work.scale_y = scale.y;
        advance_animation(work, logic_ticks);
        if (work.opening_frames > 15) {
            work.animation_angle = 0;
            work.phase = 2;
        }
        return;
    }
    case DialogUpdateResult::NeedsControl:
        control(work, captured, owner, tables, calls, math, fp);
        return;
    case DialogUpdateResult::NeedsClosePause:
        work.opening_frames =
            static_cast<std::int16_t>(static_cast<std::uint16_t>(work.opening_frames) + 1U);
        if (work.opening_frames > 3) {
            work.opening_frames = 0;
            work.phase = 4;
            const auto &parameters = require_parameters(captured);
            if (parameters.sounds)
                calls.play_sound(parameters.sounds->close);
        }
        return;
    case DialogUpdateResult::NeedsClosing: {
        work.color = 4294967295U;
        const auto &scale = animation_scale(tables.closing, work.opening_frames);
        work.scale_x = scale.x;
        work.scale_y = scale.y;
        advance_animation(work, logic_ticks);
        if (work.opening_frames > 15) {
            title_menu::reset_dialog_work(work, *current_work(owner));
            work.phase = 5;
        }
        return;
    }
    case DialogUpdateResult::NeedsCleanup:
        calls.clear_screen_fade();
        calls.set_back_colors(4278190080U, 4278190080U, 4278190080U);
        calls.set_material(4294967295U);
        calls.set_base_color(4294967295U);
        calls.reset_material();
        calls.default_blending();
        return;
    }
}
} // namespace sadx::menu_dialog
