#include "../probes/input_mod_constants_probe.hpp"
#include "guest/native_game_session.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
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
void same_composition(const sadx::NativeGameReport &expected, const sadx::D3D8StateOwner &graphics,
                      const sadx::NativeGameReport &actual, const sadx::D3D8StateOwner &native) {
    require(expected.startup_completed == actual.startup_completed &&
                expected.frames_completed == actual.frames_completed &&
                expected.exited == actual.exited && expected.failure == actual.failure &&
                expected.assumptions == actual.assumptions &&
                expected.unported.size() == actual.unported.size(),
            "native coefficients preserve the full composition report");
    for (std::size_t i = 0; i < expected.unported.size(); ++i) {
        const auto &before = expected.unported[i];
        const auto &after = actual.unported[i];
        require(before.callee == after.callee && before.first_site == after.first_site &&
                    before.path == after.path && before.calls == after.calls,
                "native coefficients preserve every unported boundary and call count");
    }
    require(graphics.render_states == native.render_states &&
                graphics.stage0_states == native.stage0_states &&
                graphics.transforms == native.transforms &&
                graphics.render_calls == native.render_calls &&
                graphics.stage_calls == native.stage_calls &&
                graphics.texture_binds == native.texture_binds &&
                graphics.vertex_formats == native.vertex_formats &&
                graphics.viewport == native.viewport &&
                graphics.begin_scenes == native.begin_scenes &&
                graphics.end_scenes == native.end_scenes && graphics.presents == native.presents &&
                graphics.clears == native.clears &&
                graphics.last_clear_flags == native.last_clear_flags &&
                graphics.draws.size() == native.draws.size() &&
                graphics.polygons.size() == native.polygons.size(),
            "native coefficients preserve recorded graphics values and call counts");
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
        require(before.vertex_bytes == after.vertex_bytes &&
                    before.primitive_type == after.primitive_type &&
                    before.primitive_count == after.primitive_count &&
                    before.stride == after.stride,
                "native coefficients preserve screen geometry and metadata");
    }
}
struct Snapshot {
    sadx::NativeGameReport report;
    sadx::D3D8StateOwner graphics;
};
Snapshot run_three(sadx::NativeGameConfig config) {
    std::int64_t now = 0;
    config.clock = {[] { return std::optional<std::int64_t>{10000000}; },
                    [&] {
                        now += 200000;
                        return std::optional<std::int64_t>{now};
                    }};
    sadx::NativeGame game(config);
    const auto report = game.run();
    require(report.startup_completed && report.frames_completed == 3 && report.failure.empty(),
            "three-frame survey completes without missing facts");
    return {report, game.d3d8()};
}
sadx::BootLoadResult boot(const char *exe, const char *dll) {
    return sadx::load_boot_config(exe, dll, 3, sadx::OwnerPolicy::survey_unported_as_noop, {});
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 4, "usage: input-constants-boot-test EXE DLL FIXTURE_PREFIX");
        auto loaded = boot(argv[1], argv[2]);
        require(bool(loaded), "normal reference images load");
        const auto canonical = sadx::test::input_constant_words(sadx::input_mod_261_constants());
        require(sadx::test::input_constant_words(loaded.config->input_constants) == canonical,
                "normal boot selects maintained Input2.6.1 constants");
        auto historical = *loaded.config;
        const auto word = [&](std::uint32_t address) {
            const auto value = historical.input_mod_dll->u32(address);
            require(value.has_value(), "pinned coefficient mapped");
            return sadx::Sse32{*value};
        };
        historical.input_constants = {{word(0x1000bb6c), word(0x1000bbb0), word(0x1000bba0),
                                       word(0x1000bba4), word(0x1000bb7c)},
                                      word(0x1000bba8),
                                      word(0x1000bb78)};
        require(sadx::test::input_constant_words(historical.input_constants) == canonical,
                "compiled coefficients equal historical DLL decoding");
        const auto reference = run_three(historical);
        const std::string prefix = argv[3];
        for (const auto &path : {std::string(argv[2]), prefix + "-poisoned.dll",
                                 prefix + "-varied.dll", prefix + "-partial.dll"}) {
            auto candidate = boot(argv[1], path.c_str());
            require(bool(candidate), "coefficient fixtures retain valid required PE reader inputs");
            require(sadx::test::input_constant_words(candidate.config->input_constants) ==
                        canonical,
                    "boot coefficients are independent of DLL coefficient availability/values");
            const sadx::NativeGame game(*candidate.config);
            require(game.input_constant_facts() == canonical,
                    "native game receives explicit boot coefficients");
            same_input_resources(game, *candidate.config);
            if (path == prefix + "-partial.dll") {
                const auto &image = *candidate.config->input_mod_dll;
                require(image.byte(0x1000bb6e).has_value() && !image.byte(0x1000bb6f) &&
                            !image.u32(0x1000bb6c),
                        "partial mapping removes exactly one coefficient byte");
                for (unsigned i = 1; i < canonical.size(); ++i) {
                    const std::uint32_t addresses[] = {0x1000bb6c, 0x1000bbb0, 0x1000bba0,
                                                       0x1000bba4, 0x1000bb7c, 0x1000bba8,
                                                       0x1000bb78};
                    require(image.u32(addresses[i]) == canonical[i],
                            "other coefficients stay mapped");
                }
                const auto original = loaded.config->input_mod_dll->export_named("OnInput");
                const auto preserved = image.export_named("OnInput");
                require(original && preserved && original->va == preserved->va &&
                            original->forwarder == preserved->forwarder,
                        "partial mapping preserves exports");
            }
            if (path == prefix + "-varied.dll") {
                const auto varied =
                    sadx::test::input_constant_words(sadx::test::varied_input_constants());
                require(candidate.config->input_mod_dll->u32(0x1000bb6c) == varied[0],
                        "varied image actually changes coefficients");
                require(candidate.config->input_mod_dll->u32(0x1000e76c) == 0x12345010U,
                        "varied fixture injects an independent name resource");
            } else {
                const auto current = run_three(*candidate.config);
                same_composition(reference.report, reference.graphics, current.report,
                                 current.graphics);
            }
        }
        sadx::NativeGameConfig injected{};
        for (const auto &values :
             {sadx::InputModPadConstants{}, sadx::test::varied_input_constants()}) {
            injected.input_constants = values;
            const sadx::NativeGame game(injected);
            require(game.input_constant_facts() == sadx::test::input_constant_words(values),
                    "caller-injected zero/IEEE coefficient words must not be replaced");
            same_input_resources(game, injected);
            for (const auto &byte : game.device_caps().bytes)
                require(!byte, "coefficients do not invent device-capability facts");
        }
        require(!boot(argv[1], "") && !boot("", argv[2]),
                "original image requirements remain explicit");
        std::cout << "Boot coefficients: canonical/poison/varied/partial DLLs, injection and "
                     "resources passed.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
