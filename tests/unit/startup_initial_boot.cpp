#include "../probes/startup_initial_state_probe.hpp"
#include "guest/native_game_session.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void same_composition(const sadx::NativeGameReport &expected, const sadx::D3D8StateOwner &graphics,
                      const sadx::NativeGameReport &actual, const sadx::D3D8StateOwner &native) {
    require(expected.startup_completed == actual.startup_completed &&
                expected.frames_completed == actual.frames_completed &&
                expected.exited == actual.exited && expected.failure == actual.failure &&
                expected.assumptions == actual.assumptions &&
                expected.unported.size() == actual.unported.size(),
            "native startup defaults preserve the full composition report");
    for (std::size_t i = 0; i < expected.unported.size(); ++i) {
        const auto &before = expected.unported[i];
        const auto &after = actual.unported[i];
        require(before.callee == after.callee && before.first_site == after.first_site &&
                    before.path == after.path && before.calls == after.calls,
                "native startup defaults preserve every unported boundary and call count");
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
            "native startup defaults preserve recorded graphics values and call counts");
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
                "native startup defaults preserve screen geometry and metadata");
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
std::array<std::optional<std::uint32_t>, 41> image_facts(const sadx::ExecutableImage *image) {
    std::array<std::optional<std::uint32_t>, 41> facts{};
    unsigned next = 0;
    const auto word = [&](std::uint32_t address) {
        return image ? image->u32(address) : std::nullopt;
    };
    facts[next++] = word(0x3b0ef38);
    facts[next++] = word(0x3b0f0e8);
    facts[next++] = word(0x3abdcf0);
    facts[next++] = word(0x909f28);
    facts[next++] = word(0x892950);
    facts[next++] = word(0x3b0ef28);
    facts[next++] = word(0x3b0eae0);
    facts[next++] = word(0x38f6ec0);
    facts[next++] = word(0x3a7cf3c);
    facts[next++] = word(0x3abdfac);
    facts[next++] = word(0x3abdf90);
    for (unsigned i = 0; i < 13; ++i) {
        const auto low = image ? image->byte(0x389d634 + 2 * i) : std::nullopt;
        const auto high = image ? image->byte(0x389d635 + 2 * i) : std::nullopt;
        if (low && high)
            facts[next] = *low | (*high << 8);
        ++next;
    }
    for (unsigned i = 0; i < 17; ++i)
        facts[next++] = word(0x8929d8 + 4 * i);
    require(next == facts.size(), "observer covers all selected fields");
    return facts;
}
void same_resources(const sadx::NativeGame &game, const sadx::NativeGameConfig &config) {
    const auto facts = game.startup_resource_facts();
    const std::uint32_t addresses[] = {0x3ab918c, 0x3ab9190, 0x8929b0, 0x8929c4};
    for (unsigned i = 0; i < 4; ++i)
        require(facts[i] ==
                    (config.executable ? config.executable->u32(addresses[i]) : std::nullopt),
                "native startup values preserve independent resource availability and identity");
}
void same_values(const sadx::NativeGame &game, const sadx::StartupInitialState &values) {
    const auto expected = sadx::test::startup_initial_words(values);
    const auto actual = game.startup_facts();
    for (unsigned i = 0; i < expected.size(); ++i)
        require(actual[i] == expected[i],
                "all native startup fields preserve injected values and widths");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 4, "usage: startup-initial-boot-test EXE DLL FIXTURE_PREFIX");
        auto loaded = boot(argv[1], argv[2]);
        require(bool(loaded) && loaded.config->startup_defaults.has_value(),
                "boot selects native startup defaults");
        const auto canonical = sadx::startup_initial_state();
        const auto varied = sadx::test::varied_startup_initial_state();
        auto historical = *loaded.config;
        historical.startup_defaults.reset();
        const sadx::NativeGame reference_game(historical);
        same_values(reference_game, canonical);
        const auto reference = run_three(historical);
        const std::string prefix = argv[3];
        for (const auto &path : {std::string(argv[1]), prefix + "-poisoned.exe",
                                 prefix + "-varied.exe", prefix + "-partial.exe"}) {
            auto candidate = boot(path.c_str(), argv[2]);
            require(bool(candidate), "reader fixtures remain valid PE inputs");
            // Reader fixtures live under build/, away from the installation's INI/model DLLs.
            // Keep those independent boot inputs identical for the composition comparison.
            candidate.config->sonicdx_ini = loaded.config->sonicdx_ini;
            candidate.config->model_dlls = loaded.config->model_dlls;
            sadx::NativeGame native(*candidate.config);
            same_values(native, canonical);
            same_resources(native, *candidate.config);
            auto fallback = *candidate.config;
            fallback.startup_defaults.reset();
            const sadx::NativeGame from_image(fallback);
            require(from_image.startup_facts() == image_facts(fallback.executable.get()),
                    "fallback preserves independent field availability and original widths");
            same_resources(from_image, fallback);
            if (path == prefix + "-varied.exe") {
                same_values(from_image, varied);
                require(native.startup_resource_facts()[0] == 0x12345010U,
                        "varied fixture changes independent media identity");
            } else if (path == prefix + "-partial.exe") {
                const auto &image = *fallback.executable;
                require(image.byte(0x389d64c).has_value() && !image.byte(0x389d64d) &&
                            !from_image.startup_facts()[23] &&
                            from_image.startup_facts()[22].has_value(),
                        "partial assignment is unknown as a whole uint16");
                for (unsigned i = 24; i < 41; ++i)
                    require(from_image.startup_facts()[i].has_value(),
                            "low configuration table remains mapped");
            } else {
                const auto current = run_three(*candidate.config);
                same_composition(reference.report, reference.graphics, current.report,
                                 current.graphics);
            }
            candidate.config->startup_defaults = varied;
            const sadx::NativeGame injected(*candidate.config);
            same_values(injected, varied);
            same_resources(injected, *candidate.config);
        }
        sadx::NativeGameConfig absent{};
        const sadx::NativeGame unknown(absent);
        require(unknown.startup_facts() == image_facts(nullptr),
                "missing image remains unknown without explicit defaults");
        for (const auto &values : {canonical, varied}) {
            absent.startup_defaults = values;
            const sadx::NativeGame native(absent);
            same_values(native, values);
            same_resources(native, absent);
            require(native.control_facts() == unknown.control_facts() &&
                        native.sound_facts() == unknown.sound_facts(),
                    "startup defaults do not overwrite separate controls or sound owners");
            for (const auto &byte : native.device_caps().bytes)
                require(!byte, "startup defaults do not invent device capability facts");
        }
        std::cout << "Startup boot: canonical/poison/varied/partial, injection, absent image and "
                     "resources passed.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
