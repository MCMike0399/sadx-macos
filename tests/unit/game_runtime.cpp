#include "guest/native_game_reference.hpp"
#include "engine/d3d8_caps.hpp"
#include "../probes/session_control_probe.hpp"
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
    require(argc==3||argc==4||argc==7,"usage: game-runtime-test SONIC.EXE INPUT_MOD.DLL [ENGINE_POISON [CONTROL_POISON CONTROL_VARIED CONTROL_PARTIAL]]");
    missing_control_facts();
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
    const auto survey=original.run();
    require(survey.failure.empty()&&survey.startup_completed&&survey.frames_completed==3&&!survey.unported.empty(),"survey completes startup and three frames");
    config.engine_tables = sadx::runtime::engine_code_tables();
    config.control_defaults = sadx::runtime::session_control_defaults();
    now = 0;
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
    if (argc == 7) {
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
