#include "game/menu_dialog_control.hpp"
#include <algorithm>
#include <cstring>
namespace c = sadx::menu_dialog_control;
namespace d = sadx::menu_dialog;
namespace {
float number(std::uint32_t v) { float f; std::memcpy(&f, &v, 4); return f; }
std::uint32_t bits(float f) { std::uint32_t v; std::memcpy(&v, &f, 4); return v; }
struct Definitions final : c::Definitions {
    std::array<d::Parameters, 3> rows{};
    const d::Parameters &parameters(std::int32_t id) override { return rows.at(id); }
};
struct Calls final : d::Calls {
    std::uint32_t *out;
    sadx::TaskHandle &canonical;
    sadx::TaskHandle other;
    bool replace;
    Calls(std::uint32_t *o, sadx::TaskHandle &task, sadx::TaskHandle replacement, bool change)
        : out(o), canonical(task), other(std::move(replacement)), replace(change) {}
    void play_sound(std::int32_t cue) override {
        out[32] = 1;
        out[33] = static_cast<std::uint32_t>(cue);
        if (replace)
            canonical = other;
    }
};
}
extern "C" void sadx_native_menu_dialog_control_probe(const std::uint32_t *i, std::uint32_t *o) {
    std::fill_n(o, 64, 0);
    sadx::TaskScheduler scheduler;
    auto task = scheduler.append_task(3, nullptr, {}).task;
    auto other = scheduler.append_task(3, nullptr, {}).task;
    auto work = std::make_shared<d::Work>();
    auto other_work = std::make_shared<d::Work>();
    task->auxiliary_native_work = work;
    other->auxiliary_native_work = other_work;
    auto canonical = task;
    Definitions defs;
    for (unsigned n = 0; n < 3; ++n) {
        defs.rows[n].choice_count = static_cast<std::int8_t>(i[9]);
        if (i[10])
            defs.rows[n].sounds = std::make_shared<sadx::title_menu::DialogSounds>(
                sadx::title_menu::DialogSounds{static_cast<std::int32_t>(i[11] + n), 0, 0, 0, 0});
    }
    work->phase = static_cast<std::int32_t>(i[1]);
    work->repeat_frames = static_cast<std::int16_t>(i[2]);
    work->opening_frames = static_cast<std::int16_t>(i[3]);
    work->animation_angle = i[4];
    work->fade = number(i[15]);
    work->cursor = static_cast<std::uint8_t>(i[5]);
    work->previous_choice = static_cast<std::int32_t>(i[6]);
    work->parameters = &defs.rows[0];
    std::array<std::int8_t, 16> bytes{};
    for (unsigned n = 0; n < 16; ++n) {
        work->choice_ids[n] = static_cast<std::int8_t>(i[16 + n]);
        bytes[n] = static_cast<std::int8_t>(i[32 + n]);
    }
    d::Choices disabled{};
    if (i[12] == 1)
        disabled = {bytes.data(), bytes.size()};
    else if (i[12] == 2)
        disabled = {work->choice_ids.data(), work->choice_ids.size()};
    else if (i[12] == 3)
        disabled = {work->choice_ids.data() + 1, work->choice_ids.size() - 1};
    Calls calls(o, canonical, other, i[13] != 0);
    d::Owner owner{canonical};
    switch (i[0]) {
    case 0: o[0] = c::close(owner); break;
    case 1: o[0] = static_cast<std::uint16_t>(c::idle_frames(owner)); break;
    case 2: c::reset_idle_frames(static_cast<std::int16_t>(i[14]), owner); break;
    case 3: c::open_advanced(i[7], static_cast<std::int8_t>(i[8]), disabled, defs, owner, calls); break;
    case 4: c::open_advanced_quick(i[7], static_cast<std::int8_t>(i[8]), disabled, defs, owner); break;
    case 5: c::open_quick(defs.rows[i[7]], static_cast<std::int8_t>(i[8]), disabled, owner); break;
    }
    o[1] = static_cast<std::uint32_t>(work->phase);
    o[2] = static_cast<std::uint16_t>(work->repeat_frames);
    o[3] = static_cast<std::uint16_t>(work->opening_frames);
    o[4] = work->animation_angle.value_or(0);
    o[35] = bits(work->fade);
    o[36] = work->animation_angle.has_value();
    o[5] = work->cursor;
    o[6] = static_cast<std::uint32_t>(work->previous_choice);
    for (unsigned n = 0; n < 3; ++n)
        if (work->parameters == &defs.rows[n])
            o[7] = n;
    for (unsigned n = 0; n < 16; ++n)
        o[8 + n] = static_cast<std::uint8_t>(work->choice_ids[n]);
    o[34] = canonical == other;
}
