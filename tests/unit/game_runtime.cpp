#include "guest/native_game_reference.hpp"
#include "engine/d3d8_caps.hpp"
#include "../probes/session_control_probe.hpp"
#include "../probes/input_initial_state_probe.hpp"
#include "../probes/ninja_sine_table_probe.hpp"
#include "../probes/ninja_fog_initial_state_probe.hpp"
#include "../probes/ninja_screen_initial_state_probe.hpp"
#include "../probes/sound_initial_state_probe.hpp"
#include "../probes/game_view_initial_state_probe.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
// Invariants of the native composition that hold as more owners are ported:
// stop policy halts exactly at the first owner the survey reports; a survey
// completes startup and the requested frames without a non-owner failure.
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::shared_ptr<const sadx::ExecutableImage> load(const char* path){std::ifstream f(path,std::ios::binary);auto image=sadx::ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}});
    require(image.has_value(),"PE image");return std::make_shared<const sadx::ExecutableImage>(std::move(*image));}
void same_control_facts(const sadx::NativeGame &game, const sadx::runtime::SessionControlDefaults &values) {
    const auto actual = game.control_facts();
    const auto expected = sadx::test::control_words(values);
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(actual[i] && *actual[i] == expected[i], "all initialized control facts retain their exact values");
}
void missing_control_facts() {
    sadx::NativeGameConfig config{};
    const sadx::NativeGame missing(config);
    for (const auto &fact : missing.control_facts())
        require(!fact, "absent reference image leaves control facts unknown");
    config.control_defaults = sadx::runtime::session_control_defaults();
    const sadx::NativeGame known(config);
    same_control_facts(known, *config.control_defaults);
    for (const auto &fact : known.device_caps().bytes)
        require(!fact, "native control defaults do not invent device-capability facts");
    config.control_defaults = sadx::test::varied_control_defaults();
    const sadx::NativeGame varied(config);
    same_control_facts(varied, *config.control_defaults);
}
void same_input_facts(const sadx::NativeGame &game, const sadx::input::InputInitialState &values) {
    const auto actual = game.input_data_facts();
    const auto expected = sadx::test::input_initial_bytes(values);
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(actual[i] && *actual[i] == expected[i], "native input facts retain every data byte");
}
void same_input_resources(const sadx::NativeGame &game, const sadx::NativeGameConfig &config) {
    const auto actual = game.input_resource_facts();
    unsigned next = 0;
    const auto record = [&](const sadx::ExecutableImage *image, std::uint32_t base) {
        for (unsigned offset : {36, 40, 48})
            for (unsigned i = 0; i < 4; ++i)
                require(actual[next++] == (image ? image->byte(base + offset + i) : std::nullopt),
                        "native input values preserve independent resource availability and bytes");
    };
    record(config.executable.get(), 0x3b0eaa8);
    for (unsigned i = 0; i < 4; ++i)
        record(config.executable.get(), 0x3b0e9c8 + 52 * i);
    for (unsigned i = 0; i < 8; ++i)
        record(config.input_mod_dll.get(), 0x1000e748 + 52 * i);
    for (unsigned i = 0; i < 8; ++i)
        record(config.input_mod_dll.get(), 0x1000e950 + 96 * i);
    require(next == actual.size(), "resource observer covers every pointer word");
}
void reference_input_facts(const sadx::NativeGame &game, const sadx::NativeGameConfig &config) {
    const auto actual = game.input_data_facts();
    unsigned next = 0;
    const auto region = [&](const sadx::ExecutableImage *image, std::uint32_t base, unsigned size) {
        for (unsigned i = 0; i < size; ++i)
            require(actual[next++] == (image ? image->byte(base + i) : std::nullopt),
                    "reference input values retain byte-granular missing facts");
    };
    for (auto span : {std::pair{0x3b0e3e2U, 768U}, {0x3b0eaf0U, 256U}, {0x3b0e3baU, 1U},
                      {0x3b0eae8U, 1U}, {0x3b0e3e0U, 2U}, {0x3b0e3b9U, 1U}})
        region(config.executable.get(), span.first, span.second);
    const auto record = [&](const sadx::ExecutableImage *image, std::uint32_t base) {
        region(image, base, 36);
        region(image, base + 44, 4);
    };
    record(config.executable.get(), 0x3b0eaa8);
    for (unsigned i = 0; i < 4; ++i)
        record(config.executable.get(), 0x3b0e9c8 + 52 * i);
    for (unsigned i = 0; i < 8; ++i)
        record(config.input_mod_dll.get(), 0x1000e748 + 52 * i);
    for (unsigned i = 0; i < 8; ++i)
        record(config.input_mod_dll.get(), 0x1000e950 + 96 * i);
    require(next == actual.size(), "data observer covers every selected source byte");
    same_input_resources(game, config);
}
void missing_input_facts() {
    sadx::NativeGameConfig config{};
    const sadx::NativeGame missing(config);
    reference_input_facts(missing, config);
    config.input_defaults = sadx::input::input_initial_state();
    const sadx::NativeGame known(config);
    same_input_facts(known, *config.input_defaults);
    same_input_resources(known, config);
    config.input_defaults = sadx::test::varied_input_initial_state();
    const sadx::NativeGame varied(config);
    same_input_facts(varied, *config.input_defaults);
    same_input_resources(varied, config);
}
void same_sine_facts(const sadx::NativeGame &game, const sadx::NinjaQuarterSineTable &table) {
    require(game.sine_facts().has_value(), "complete sine table remains available");
    require(sadx::test::sine_words(*game.sine_facts()) == sadx::test::sine_words(table),
            "every initialized coefficient retains its exact bits");
}
void missing_sine_facts() {
    sadx::NativeGameConfig config{};
    const sadx::NativeGame missing(config);
    require(!missing.sine_facts(), "absent reference image leaves the sine table unknown");
    config.sine_table = sadx::ninja_sine_table();
    const sadx::NativeGame native(config);
    same_sine_facts(native, *config.sine_table);
    config.sine_table = sadx::test::varied_sine_table();
    const sadx::NativeGame varied(config);
    same_sine_facts(varied, *config.sine_table);
}
void same_fog_facts(const sadx::NativeGame &game, const sadx::NinjaFogInitialState &values) {
    const auto actual = game.fog_facts();
    const auto expected = sadx::test::fog_initial_words(values);
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(actual[i] && *actual[i] == expected[i], "every fog initializer fact retains its exact bits");
}
void missing_fog_facts() {
    sadx::NativeGameConfig config{};
    const sadx::NativeGame missing(config);
    for (const auto &fact : missing.fog_facts())
        require(!fact, "absent reference image leaves fog facts unknown");
    config.fog_defaults = sadx::ninja_fog_initial_state();
    config.control_defaults = sadx::test::varied_control_defaults();
    const sadx::NativeGame native(config);
    same_fog_facts(native, *config.fog_defaults);
    same_control_facts(native, *config.control_defaults);
    for (const auto &byte : native.device_caps().bytes)
        require(!byte, "native fog defaults must not invent GPU capabilities");
    config.fog_defaults = sadx::test::varied_fog_initial_state();
    const sadx::NativeGame varied(config);
    same_fog_facts(varied, *config.fog_defaults);
    same_control_facts(varied, *config.control_defaults);
}
void same_sound_facts(const sadx::NativeGame &game, const sadx::SoundInitialState &values) {
    const auto actual = game.sound_facts();
    const auto expected = sadx::test::sound_initial_words(values);
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(actual[i] && *actual[i] == expected[i], "native sound numeric bits survive adapter mapping");
}
void same_sound_resources(const sadx::NativeGame &game, const sadx::NativeGameConfig &config) {
    const auto actual = game.sound_resource_facts();
    const auto read = [&](std::uint32_t address) {
        return config.executable ? config.executable->u32(address) : std::nullopt;
    };
    for (unsigned i = 0; i < 36; ++i) {
        require(actual[i] == read(0x3b29300 + 0x38 * i), "opaque record ID remains reference-backed");
        require(actual[36 + i] == read(0x3b29b90 + 4 * i), "origin task remains reference-backed");
    }
    require(actual[72] == read(0x3b2cbb0), "listener remains reference-backed");
}
void reference_sound_facts(const sadx::NativeGame &game, const sadx::NativeGameConfig &config) {
    const auto actual = game.sound_facts();
    unsigned next = 0;
    const auto read = [&](std::uint32_t address) {
        return config.executable ? config.executable->u32(address) : std::nullopt;
    };
    for (auto address : {0x3b29ce0U, 0x3b29ce4U, 0x3b29b7cU})
        require(actual[next++] == read(address), "reference sound scalar availability");
    require(actual[next++] == (config.executable ? config.executable->byte(0x3b29ce8) : std::nullopt),
            "reference sound frame byte availability");
    require(actual[next++] == read(0x3b0ef2c), "reference audio initialized availability");
    for (unsigned word = 0; word < 18; ++word) {
        const auto packed = read(0x3b292a8 + 4 * word);
        for (unsigned byte = 0; byte < 4; ++byte) {
            const auto expected = packed ? std::optional<std::uint32_t>((*packed >> (8 * byte)) & 255U) : std::nullopt;
            require(actual[next++] == expected, "reference overlap retains whole-word availability");
        }
    }
    for (unsigned i = 0; i < 36; ++i) {
        for (unsigned offset : {4, 12, 16, 20, 24, 32})
            require(actual[next++] == read(0x3b292f8 + 0x38 * i + offset), "reference record numeric availability");
        require(actual[next++] == read(0x3b29c28 + 4 * i), "reference current volume availability");
    }
    require(next == actual.size(), "all sound facts observed");
    same_sound_resources(game, config);
}
void missing_sound_facts() {
    sadx::NativeGameConfig config{};
    const sadx::NativeGame missing(config);
    reference_sound_facts(missing, config);
    config.sound_defaults = sadx::sound_initial_state();
    config.control_defaults = sadx::test::varied_control_defaults();
    const sadx::NativeGame native(config);
    same_sound_facts(native, *config.sound_defaults);
    same_sound_resources(native, config);
    same_control_facts(native, *config.control_defaults);
    for (const auto &byte : native.device_caps().bytes)
        require(!byte, "sound defaults do not invent GPU capabilities");
    config.sound_defaults = sadx::test::varied_sound_initial_state();
    const sadx::NativeGame varied(config);
    same_sound_facts(varied, *config.sound_defaults);
    same_sound_resources(varied, config);
}
void same_screen_facts(const sadx::NativeGame &game, const sadx::NinjaScreenInitialState &values) {
    const auto actual = game.screen_facts();
    const auto expected = sadx::test::screen_initial_words(values);
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(actual[i] && *actual[i] == expected[i], "every numeric screen fact retains its exact bits");
}
void same_screen_resources(const sadx::NativeGame &game, const sadx::NativeGameConfig &config) {
    const auto actual = game.screen_resource_facts();
    const std::uint32_t addresses[] = {0x3ce7164, 0x3ce7168, 0x3ce716c};
    for (unsigned i = 0; i < 3; ++i)
        require(actual[i] == (config.executable ? config.executable->u32(addresses[i]) : std::nullopt),
                "screen numeric defaults preserve point/color/texture resource facts");
}
void missing_screen_facts() {
    sadx::NativeGameConfig config{};
    const sadx::NativeGame missing(config);
    for (const auto &fact : missing.screen_facts())
        require(!fact, "absent image leaves numeric screen facts unknown");
    config.screen_defaults = sadx::ninja_screen_initial_state();
    config.control_defaults = sadx::test::varied_control_defaults();
    config.fog_defaults = sadx::test::varied_fog_initial_state();
    const sadx::NativeGame native(config);
    same_screen_facts(native, *config.screen_defaults);
    same_screen_resources(native, config);
    same_control_facts(native, *config.control_defaults);
    same_fog_facts(native, *config.fog_defaults);
    for (const auto &byte : native.device_caps().bytes)
        require(!byte, "screen defaults do not invent GPU capability bytes");
    config.screen_defaults = sadx::test::varied_screen_initial_state();
    const sadx::NativeGame varied(config);
    same_screen_facts(varied, *config.screen_defaults);
    same_screen_resources(varied, config);
}
void same_view_facts(const sadx::NativeGame &game, const sadx::GameViewInitialState &values) {
    const auto actual = game.view_facts();
    const auto expected = sadx::test::view_initial_words(values);
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(actual[i] && *actual[i] == expected[i], "native view facts retain every bit");
}
void reference_view_facts(const sadx::NativeGame &game, const sadx::NativeGameConfig &config) {
    const auto actual = game.view_facts();
    const auto word = [&](std::uint32_t address) {
        return config.executable ? config.executable->u32(address) : std::nullopt;
    };
    unsigned next = 0;
    for (auto address : {0x8928c0U, 0x8928c4U, 0x3abd9ccU, 0x3b28094U,
                         0x3b28098U, 0x3b2809cU, 0x3b280a0U, 0x3b280a4U})
        require(actual[next++] == word(address), "view scalar availability and raw values");
    require(actual[next++] == (config.executable ? config.executable->byte(0x3b280a8) : std::nullopt),
            "view reset flag byte availability");
    for (auto address : {0x3d0f9ecU, 0x3d0f9f0U, 0x3aad0bcU, 0x3ab98ecU, 0x3b2cbb4U,
                         0x3ce7174U, 0x3ce7178U, 0x3ce717cU, 0x3ce7180U})
        require(actual[next++] == word(address), "view caches and clear colors stay independently available");
    for (auto address : {0x3b280acU, 0x3b18230U, 0x3ab9864U}) {
        bool complete = true;
        for (unsigned i = 0; i < 4; ++i)
            complete = complete && word(address + 4 * i).has_value();
        for (unsigned i = 0; i < 4; ++i)
            require(actual[next++] == (complete ? word(address + 4 * i) : std::nullopt),
                    "view colors retain whole-group availability");
    }
    require(next == actual.size(), "all numeric view facts observed");
}
void missing_view_facts() {
    sadx::NativeGameConfig config{};
    const sadx::NativeGame missing(config);
    reference_view_facts(missing, config);
    config.view_defaults = sadx::game_view_initial_state();
    config.control_defaults = sadx::test::varied_control_defaults();
    config.screen_defaults = sadx::test::varied_screen_initial_state();
    const sadx::NativeGame native(config);
    same_view_facts(native, *config.view_defaults);
    same_control_facts(native, *config.control_defaults);
    same_screen_facts(native, *config.screen_defaults);
    same_screen_resources(native, config);
    for (const auto &byte : native.device_caps().bytes)
        require(!byte, "view values do not invent GPU capabilities");
    config.view_defaults = sadx::test::varied_view_initial_state();
    const sadx::NativeGame varied(config);
    same_view_facts(varied, *config.view_defaults);
    same_control_facts(varied, *config.control_defaults);
    same_screen_facts(varied, *config.screen_defaults);
}
void same_composition(const sadx::NativeGameReport &expected, const sadx::D3D8StateOwner &graphics,
                      const sadx::NativeGameReport &actual, const sadx::D3D8StateOwner &native) {
    require(expected.startup_completed == actual.startup_completed &&
                expected.frames_completed == actual.frames_completed && expected.exited == actual.exited &&
                expected.failure == actual.failure && expected.assumptions == actual.assumptions &&
                expected.unported.size() == actual.unported.size(),
            "native code tables preserve the full composition report");
    for (std::size_t i = 0; i < expected.unported.size(); ++i) {
        const auto &before = expected.unported[i];
        const auto &after = actual.unported[i];
        require(before.callee == after.callee && before.first_site == after.first_site &&
                    before.path == after.path && before.calls == after.calls,
                "native code tables preserve every unported boundary and call count");
    }
    require(graphics.render_states == native.render_states && graphics.stage0_states == native.stage0_states &&
                graphics.transforms == native.transforms && graphics.render_calls == native.render_calls &&
                graphics.stage_calls == native.stage_calls && graphics.texture_binds == native.texture_binds &&
                graphics.vertex_formats == native.vertex_formats && graphics.viewport == native.viewport &&
                graphics.begin_scenes == native.begin_scenes && graphics.end_scenes == native.end_scenes &&
                graphics.presents == native.presents && graphics.clears == native.clears &&
                graphics.last_clear_flags == native.last_clear_flags &&
                graphics.draws.size() == native.draws.size() && graphics.polygons.size() == native.polygons.size(),
            "native code tables preserve recorded graphics values and call counts");
    require(bool(expected.stopped_at) == bool(actual.stopped_at), "same stopped-owner presence");
    if (expected.stopped_at) {
        const auto &before = *expected.stopped_at;
        const auto &after = *actual.stopped_at;
        require(before.callee == after.callee && before.first_site == after.first_site &&
                    before.path == after.path && before.calls == after.calls,
                "same stopped-owner report");
    }
    // This existing three-frame bootstrap does not issue sprite draws. Do not let it
    // silently become an incomplete sprite-command comparator as recovery advances.
    require(graphics.draws.empty() && native.draws.empty(),
            "extend bootstrap observer before claiming sprite-command equivalence");
    for (std::size_t i = 0; i < graphics.polygons.size(); ++i) {
        const auto &before = graphics.polygons[i];
        const auto &after = native.polygons[i];
        require(before.vertex_bytes == after.vertex_bytes && before.primitive_type == after.primitive_type &&
                    before.primitive_count == after.primitive_count && before.stride == after.stride,
                "native code tables preserve screen geometry and metadata");
    }
}
}
int main(int argc,char** argv){try{
    require(argc==3||argc==4||argc==7||argc==13||argc==14||argc==15||argc==16||argc==17||argc==18,"usage: game-runtime-test SONIC.EXE INPUT_MOD.DLL [ENGINE_POISON [CONTROL_POISON CONTROL_VARIED CONTROL_PARTIAL]]");
    missing_control_facts();
    missing_input_facts();
    missing_sine_facts();
    missing_fog_facts();
    missing_screen_facts();
    missing_sound_facts();
    missing_view_facts();
    const auto exe=load(argv[1]),dll=load(argv[2]);
    require(exe->u32(0x7dce58)==0x42720000u&&exe->byte(0x3abdc7c)==0&&!exe->byte(0x3d14000+0x2000),"load image: .rdata 60.5f at 7dce58, zero-fill .data, outside the image");
    sadx::NativeGameConfig config;config.executable=exe;config.input_mod_dll=dll;
    const auto f=[&](std::uint32_t va){return sadx::Sse32{*dll->u32(va)};};
    config.input_constants={{f(0x1000bb6c),f(0x1000bbb0),f(0x1000bba0),f(0x1000bba4),f(0x1000bb7c)},f(0x1000bba8),f(0x1000bb78)};
    for(auto& s:config.pad_settings)s={false,7849,8689,true,false,30};config.pad_settings[0].allow_keyboard=true;
    std::int64_t now=0;config.clock={[]{return std::optional<std::int64_t>{10'000'000};},[&]{now+=200'000;return std::optional<std::int64_t>{now};}};
    config.policy=sadx::OwnerPolicy::survey_unported_as_noop;config.max_frames=3;
    sadx::NativeGame original(config);
    same_control_facts(original, sadx::runtime::session_control_defaults());
    same_input_facts(original, sadx::input::input_initial_state());
    reference_input_facts(original, config);
    same_sine_facts(original, sadx::ninja_sine_table());
    same_fog_facts(original, sadx::ninja_fog_initial_state());
    same_screen_facts(original, sadx::ninja_screen_initial_state());
    same_sound_facts(original, sadx::sound_initial_state());
    reference_sound_facts(original, config);
    same_view_facts(original, sadx::game_view_initial_state());
    reference_view_facts(original, config);
    same_screen_resources(original, config);
    const auto survey=original.run();
    require(survey.failure.empty()&&survey.startup_completed&&survey.frames_completed==3&&!survey.unported.empty(),"survey completes startup and three frames");
    config.engine_tables = sadx::runtime::engine_code_tables();
    config.control_defaults = sadx::runtime::session_control_defaults();
    config.input_defaults = sadx::input::input_initial_state();
    config.sine_table = sadx::ninja_sine_table();
    config.fog_defaults = sadx::ninja_fog_initial_state();
    config.screen_defaults = sadx::ninja_screen_initial_state();
    now = 0;
    config.sound_defaults = sadx::sound_initial_state();
    config.view_defaults = sadx::game_view_initial_state();
    sadx::NativeGame native_tables(config);
    same_control_facts(native_tables, *config.control_defaults);
    const auto native_report = native_tables.run();
    same_composition(survey, original.d3d8(), native_report, native_tables.d3d8());
    if (argc >= 4) {
        config.executable = load(argv[3]);
        require(config.executable->u32(0x88ad64) != exe->u32(0x88ad64),
                "poison fixture actually changes source tables");
        now = 0;
        sadx::NativeGame poisoned(config);
        const auto poisoned_report = poisoned.run();
        same_composition(survey, original.d3d8(), poisoned_report, poisoned.d3d8());
    }
    if (argc >= 7) {
        config.executable = load(argv[4]);
        now = 0;
        sadx::NativeGame poisoned_controls(config);
        same_control_facts(poisoned_controls, *config.control_defaults);
        const auto poisoned_report = poisoned_controls.run();
        same_composition(survey, original.d3d8(), poisoned_report, poisoned_controls.d3d8());
        config.executable = load(argv[5]);
        config.control_defaults.reset();
        const sadx::NativeGame varied_image(config);
        same_control_facts(varied_image, sadx::test::varied_control_defaults());
        config.executable = load(argv[6]);
        const sadx::NativeGame partial_image(config);
        const auto partial = partial_image.control_facts();
        const auto expected = sadx::test::control_words(sadx::runtime::session_control_defaults());
        // Declared probe slots: frequency high, previous-counter low/high.
        // The frequency low word is still present; the pair-only previous counter is absent.
        for (std::size_t i = 0; i < partial.size(); ++i) {
            if (i == 17 || i == 22 || i == 23)
                require(!partial[i], "truncated control facts remain absent at their actual widths");
            else
                require(partial[i] && *partial[i] == expected[i], "other mapped control facts remain known");
        }

    }
    if (argc >= 13) {
        config.control_defaults = sadx::runtime::session_control_defaults();
        config.executable = load(argv[7]);
        config.input_mod_dll = load(argv[8]);
        now = 0;
        sadx::NativeGame poisoned_input(config);
        same_input_facts(poisoned_input, *config.input_defaults);
        same_input_resources(poisoned_input, config);
        const auto report = poisoned_input.run();
        same_composition(survey, original.d3d8(), report, poisoned_input.d3d8());
        config.executable = load(argv[9]);
        config.input_mod_dll = load(argv[10]);
        config.input_defaults.reset();
        const sadx::NativeGame varied_image(config);
        same_input_facts(varied_image, sadx::test::varied_input_initial_state());
        reference_input_facts(varied_image, config);
        config.input_defaults = sadx::input::input_initial_state();
        const sadx::NativeGame native_over_varied(config);
        same_input_facts(native_over_varied, *config.input_defaults);
        same_input_resources(native_over_varied, config);
        require(native_over_varied.input_resource_facts() == varied_image.input_resource_facts(),
                "explicit native data must not overwrite injected resources");
        config.executable = load(argv[11]);
        config.input_mod_dll = load(argv[12]);
        config.input_defaults.reset();
        const sadx::NativeGame partial(config);
        reference_input_facts(partial, config);
        const auto facts = partial.input_data_facts();
        require(facts[1029] && facts[1030] && !facts[1031],
                "partial template preserves two known bytes and the missing third byte");
        require(facts[1229] && facts[1230] && !facts[1231],
                "partial mod record preserves two known bytes and the missing third byte");
        config.input_defaults = sadx::test::varied_input_initial_state();
        const sadx::NativeGame partial_native(config);
        same_input_facts(partial_native, *config.input_defaults);
        same_input_resources(partial_native, config);
    }
    if (argc >= 14) {
        const std::string prefix = argv[13];
        config.input_mod_dll = dll;
        config.input_defaults = sadx::input::input_initial_state();
        config.control_defaults = sadx::runtime::session_control_defaults();
        config.executable = load((prefix + "-poisoned.exe").c_str());
        require(config.executable->u32(0x88af78) != exe->u32(0x88af78),
                "poison fixture changes the coefficient bytes");
        now = 0;
        sadx::NativeGame poisoned_sine(config);
        same_sine_facts(poisoned_sine, *config.sine_table);
        const auto report = poisoned_sine.run();
        same_composition(survey, original.d3d8(), report, poisoned_sine.d3d8());
        config.executable = load((prefix + "-varied.exe").c_str());
        config.sine_table.reset();
        const sadx::NativeGame varied_image(config);
        same_sine_facts(varied_image, sadx::test::varied_sine_table());
        config.sine_table = sadx::ninja_sine_table();
        const sadx::NativeGame native_over_varied(config);
        same_sine_facts(native_over_varied, *config.sine_table);
        config.executable = load((prefix + "-partial.exe").c_str());
        require(config.executable->u32(0x88bf74).has_value() &&
                    config.executable->byte(0x88bf7a).has_value() && !config.executable->byte(0x88bf7b),
                "partial fixture removes only the final knot byte within this table");
        config.sine_table.reset();
        const sadx::NativeGame partial(config);
        require(!partial.sine_facts(), "incomplete final coefficient keeps the entire reference table unknown");
        config.sine_table = sadx::test::varied_sine_table();
        const sadx::NativeGame partial_native(config);
        same_sine_facts(partial_native, *config.sine_table);
    }
    if (argc >= 15) {
        const std::string prefix = argv[14];
        config.input_mod_dll = dll;
        config.sine_table = sadx::ninja_sine_table();
        config.executable = load((prefix + "-poisoned.exe").c_str());
        require(config.executable->u32(0x90a098) != exe->u32(0x90a098) &&
                    config.executable->u32(0x3b10f78) != exe->u32(0x3b10f78),
                "fog poison fixture changes file-backed caches and virtual-zero table data");
        now = 0;
        sadx::NativeGame poisoned_fog(config);
        same_fog_facts(poisoned_fog, *config.fog_defaults);
        const auto report = poisoned_fog.run();
        same_composition(survey, original.d3d8(), report, poisoned_fog.d3d8());
        config.executable = load((prefix + "-varied.exe").c_str());
        config.fog_defaults.reset();
        const sadx::NativeGame varied_image(config);
        same_fog_facts(varied_image, sadx::test::varied_fog_initial_state());
        config.fog_defaults = sadx::ninja_fog_initial_state();
        const sadx::NativeGame native_over_varied(config);
        same_fog_facts(native_over_varied, *config.fog_defaults);
        config.executable = load((prefix + "-partial.exe").c_str());
        require(config.executable->byte(0x3d12dfe).has_value() &&
                    !config.executable->byte(0x3d12dff),
                "partial fixture removes the last reciprocal float byte");
        config.fog_defaults.reset();
        const sadx::NativeGame partial(config);
        const auto facts = partial.fog_facts();
        const auto expected = sadx::test::fog_initial_words(sadx::ninja_fog_initial_state());
        for (std::size_t i = 0; i < facts.size(); ++i) {
            if (i == facts.size() - 1)
                require(!facts[i], "incomplete reciprocal word remains unknown");
            else
                require(facts[i] && *facts[i] == expected[i], "other mapped fog words remain known");
        }
        config.fog_defaults = sadx::test::varied_fog_initial_state();
        const sadx::NativeGame partial_native(config);
        same_fog_facts(partial_native, *config.fog_defaults);
    }
    if (argc >= 16) {
        const std::string prefix = argv[15];
        config.fog_defaults = sadx::ninja_fog_initial_state();
        config.sine_table = sadx::ninja_sine_table();
        config.executable = load((prefix + "-poisoned.exe").c_str());
        require(config.executable->u32(0x389d318) != exe->u32(0x389d318) &&
                    config.executable->u32(0x3d129c0) != exe->u32(0x3d129c0),
                "screen poison fixture changes base and zero-filled projection matrices");
        now = 0;
        sadx::NativeGame poisoned_screen(config);
        same_screen_facts(poisoned_screen, *config.screen_defaults);
        same_screen_resources(poisoned_screen, config);
        const auto report = poisoned_screen.run();
        same_composition(survey, original.d3d8(), report, poisoned_screen.d3d8());
        config.executable = load((prefix + "-varied.exe").c_str());
        config.screen_defaults.reset();
        const sadx::NativeGame varied_image(config);
        same_screen_facts(varied_image, sadx::test::varied_screen_initial_state());
        same_screen_resources(varied_image, config);
        config.screen_defaults = sadx::ninja_screen_initial_state();
        const sadx::NativeGame native_over_varied(config);
        same_screen_facts(native_over_varied, *config.screen_defaults);
        same_screen_resources(native_over_varied, config);
        require(native_over_varied.screen_resource_facts() == varied_image.screen_resource_facts(),
                "native numeric values must not replace injected polygon resources");
        config.executable = load((prefix + "-partial.exe").c_str());
        require(config.executable->byte(0x3d129fe).has_value() &&
                    !config.executable->byte(0x3d129ff),
                "partial screen fixture removes the final projection word byte");
        config.screen_defaults.reset();
        const sadx::NativeGame partial(config);
        const auto facts = partial.screen_facts();
        const auto expected = sadx::test::screen_initial_words(sadx::ninja_screen_initial_state());
        for (std::size_t i = 0; i < facts.size(); ++i) {
            if (i == 57)
                require(!facts[i], "incomplete final projection word remains unknown");
            else
                require(facts[i] && *facts[i] == expected[i], "other screen words remain independently known");
        }
        same_screen_resources(partial, config);
        config.screen_defaults = sadx::test::varied_screen_initial_state();
        const sadx::NativeGame partial_native(config);
        same_screen_facts(partial_native, *config.screen_defaults);
        same_screen_resources(partial_native, config);
    }
    if (argc >= 17) {
        const std::string prefix = argv[16];
        config.screen_defaults = sadx::ninja_screen_initial_state();
        config.sound_defaults = sadx::sound_initial_state();
        config.executable = load((prefix + "-poisoned.exe").c_str());
        require(config.executable->u32(0x3b29308) != exe->u32(0x3b29308) &&
                    config.executable->byte(0x3b292ef) != exe->byte(0x3b292ef),
                "sound fixture poisons record and overlap values");
        now = 0;
        sadx::NativeGame poisoned(config);
        same_sound_facts(poisoned, *config.sound_defaults);
        same_sound_resources(poisoned, config);
        const auto report = poisoned.run();
        same_composition(survey, original.d3d8(), report, poisoned.d3d8());
        config.executable = load((prefix + "-varied.exe").c_str());
        config.sound_defaults.reset();
        const sadx::NativeGame varied_image(config);
        same_sound_facts(varied_image, sadx::test::varied_sound_initial_state());
        reference_sound_facts(varied_image, config);
        config.sound_defaults = sadx::sound_initial_state();
        const sadx::NativeGame native_over_varied(config);
        same_sound_facts(native_over_varied, *config.sound_defaults);
        same_sound_resources(native_over_varied, config);
        require(native_over_varied.sound_resource_facts() == varied_image.sound_resource_facts(),
                "native sound defaults preserve injected opaque identities");
        config.executable = load((prefix + "-partial.exe").c_str());
        require(config.executable->byte(0x3b292ee).has_value() && !config.executable->byte(0x3b292ef),
                "partial fixture cuts the final overlap word");
        config.sound_defaults.reset();
        const sadx::NativeGame partial(config);
        reference_sound_facts(partial, config);
        const auto facts = partial.sound_facts();
        for (unsigned i = 5 + 68; i < 5 + 72; ++i)
            require(!facts[i], "all bytes of incomplete reference overlap word remain unknown");
        config.sound_defaults = sadx::test::varied_sound_initial_state();
        const sadx::NativeGame partial_native(config);
        same_sound_facts(partial_native, *config.sound_defaults);
        same_sound_resources(partial_native, config);
    }
    if (argc >= 18) {
        const std::string prefix = argv[17];
        config.sound_defaults = sadx::sound_initial_state();
        config.view_defaults = sadx::game_view_initial_state();
        config.executable = load((prefix + "-poisoned.exe").c_str());
        require(config.executable->u32(0x8928c0) != exe->u32(0x8928c0) &&
                    config.executable->u32(0x3b280b8) != exe->u32(0x3b280b8),
                "view poison changes stretches and zero-filled color components");
        now = 0;
        sadx::NativeGame poisoned(config);
        same_view_facts(poisoned, *config.view_defaults);
        same_screen_resources(poisoned, config);
        const auto report = poisoned.run();
        same_composition(survey, original.d3d8(), report, poisoned.d3d8());
        config.executable = load((prefix + "-varied.exe").c_str());
        config.view_defaults.reset();
        const sadx::NativeGame varied_image(config);
        same_view_facts(varied_image, sadx::test::varied_view_initial_state());
        reference_view_facts(varied_image, config);
        config.view_defaults = sadx::game_view_initial_state();
        const sadx::NativeGame native_over_varied(config);
        same_view_facts(native_over_varied, *config.view_defaults);
        same_screen_resources(native_over_varied, config);
        config.executable = load((prefix + "-partial.exe").c_str());
        require(config.executable->byte(0x3b280ba).has_value() && !config.executable->byte(0x3b280bb),
                "partial fixture cuts the fourth view-color float");
        config.view_defaults.reset();
        const sadx::NativeGame partial(config);
        reference_view_facts(partial, config);
        const auto facts = partial.view_facts();
        for (unsigned i = 18; i < 22; ++i)
            require(!facts[i], "all four view-color components remain unknown when one is incomplete");
        config.view_defaults = sadx::test::varied_view_initial_state();
        const sadx::NativeGame partial_native(config);
        same_view_facts(partial_native, *config.view_defaults);
        same_screen_resources(partial_native, config);
    }
    config.view_defaults.reset();
    config.sound_defaults.reset();
    config.screen_defaults.reset();
    config.fog_defaults.reset();
    config.sine_table.reset();
    config.input_defaults.reset();
    config.input_mod_dll = load(argv[2]);
    config.control_defaults.reset();
    config.executable = exe;
    config.engine_tables.reset();
    config.policy=sadx::OwnerPolicy::stop_at_unported;
    sadx::NativeGame game(config);const auto stopped=game.run();
    require(stopped.stopped_at&&stopped.stopped_at->callee==survey.unported.front().callee&&stopped.failure.empty(),"stop policy halts at the first surveyed owner");
    const auto caps=sadx::assumed_d3d8_caps();
    for(std::size_t i=0;i<caps.size();++i)require(game.device_caps().bytes[i]==caps[i],"native startup stores the shared caps bytes");
    require(std::find(stopped.assumptions.begin(),stopped.assumptions.end(),sadx::d3d8_caps_assumption())!=stopped.assumptions.end(),"native caps assumption reported");
    require(game.device_caps().read(0x3d127c0,4)==1,"device caps declare HAL");
    require(game.device_caps().read(0x3d127e4,4)==0x100000,"device caps retain WFOG");
    require(game.device_caps().read(0x3d127fc,4)==0x1ec05,"device caps retain mipmap and cube-map converters");
    // 794ec0 then 40bf40>411af0>4119b0's update: zeroed fog table, colour 0,
    // linear table fog with the 65534..65535 range, fog switched off.
    const auto& rs=game.d3d8().render_states;const auto state=[&](std::uint32_t s){const auto i=rs.find(s);return i==rs.end()?0xdeadbeefU:i->second;};
    require(state(28)==0&&state(34)==0&&state(35)==3&&state(36)==0x477ffe00&&state(37)==0x477fff00,"fog init and the reset fog update reach the D3D8 state owner");
    std::cout<<"Game runtime: survey ran startup + 3 frames ("<<survey.unported.size()<<" unported owners); native execution stops at "<<stopped.stopped_at->path<<".\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
