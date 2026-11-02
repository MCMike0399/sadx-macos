"""Narrow source-only packet builds and compiled faults in private copies.

Never invokes make or consumes a maintained Main archive.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[3]
FLAGS = ["xcrun", "clang++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
         "-ffp-contract=off", "-fno-fast-math", "-arch", "arm64", "-mmacosx-version-min=14.0"]
COMMON = ["src/player/player_animation.cpp", "src/player/player_animation_blend.cpp",
          "src/engine/task_work.cpp", "src/cpu/x87_math.cpp", "src/cpu/crt_integer.cpp",
          "src/guest/player_animation_reference.cpp"]
STARTUP = ["src/player/player_startup.cpp", "src/player/player_state.cpp",
           "src/player/player_task.cpp", "src/guest/player_initialization_reference.cpp",
           "src/engine/task.cpp", "src/engine/task_work.cpp", "src/cpu/x87_math.cpp"]
SONIC = list(dict.fromkeys(COMMON + STARTUP + [
    "src/player/sonic_player.cpp", "src/guest/sonic_player_reference.cpp", "src/player/actor_current.cpp", "src/player/player_hold.cpp",
    "src/player/player_motion.cpp", "src/player/player_input.cpp", "src/player/player_coordinates.cpp",
    "src/player/player_movement.cpp", "src/player/player_position.cpp", "src/player/player_rotation.cpp",
    "src/player/player_projection.cpp", "src/player/player_acceleration_air.cpp",
    "src/guest/player_acceleration_ground.cpp", "src/guest/player_acceleration_friction.cpp",
    "src/player/ground_acceleration.cpp", "src/player/ground_friction.cpp",
    "src/player/floor_angles.cpp", "src/engine/ninja_matrix.cpp", "src/math/ninja_math.cpp",
    "src/cpu/x87_transcendental.cpp", "src/cpu/crt_asin.cpp", "src/engine/game_controller.cpp",
    "src/player/player_heading.cpp", "src/player/player_speed.cpp", "src/cpu/crt_argument.cpp", "src/cpu/crt_acos.cpp"]))
from migration_faults import TABLE_FAULTS



def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    completed = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.append(dict(command=command, exit_status=completed.returncode, output=completed.stdout.decode()))
    if completed.returncode:
        raise RuntimeError(log[-1])


def build(output, slug, source_override=None, fault=None):
    output.mkdir(parents=True, exist_ok=True)
    commands = []
    sources = (["src/player/sonic_animation_table.cpp", "src/guest/sonic_animation_table_reference.cpp"] if "table" in slug else
               STARTUP.copy() if slug == "player_startup" else
               list(dict.fromkeys(STARTUP + ["src/player/gamma_selection_core.cpp"])) if slug == "gamma_selection_core" else
               SONIC.copy() if slug == "sonic_player" else COMMON.copy())
    probe = ("tests/probes/native_animation_table_characterization_probe.cpp" if slug == "table_characterization"
             else "tests/probes/native_" + slug + "_probe.cpp")
    sources.insert(0, probe)
    original_sources = sources.copy()
    overlay = None
    if fault:
        original, old, new = TABLE_FAULTS[fault]
        body = (ROOT / original).read_text()
        assert body.count(old) == 1, (fault, body.count(old))
        replacement = output / "include" / pathlib.Path(original).relative_to("src")
        replacement.parent.mkdir(parents=True, exist_ok=True)
        body = body.replace(old, new)
        if fault == "reordered_publication":
            old_read = "const auto action = recipe.action_export ? sink.load_action(index, recipe.action_index)\n                                                 : sink.null_action();"
            assert body.count(old_read) == 1
            body = body.replace(old_read, "const auto action = prefetched[index];")
        replacement.write_text(body)
        source_override = (original, replacement)
    if source_override:
        original, replacement = source_override
        if pathlib.Path(original).suffix == ".hpp":
            overlay = pathlib.Path(replacement)
            for _ in pathlib.Path(original).relative_to("src").parts:
                overlay = overlay.parent
        else:
            sources[sources.index(original)] = str(replacement)
    library = output / (slug + ".dylib")
    includes = ["-Isrc", "-Ithird_party/bochs_fpatan", "-Ithird_party/bochs_fpatan/source",
                "-Ithird_party/bochs_fpatan/source/softfloat3e/include"]
    if overlay:
        includes.insert(0, "-I" + str(overlay))
    # Match the registered recipe: the frozen reference fixture is injected into
    # the probe TU only, never into maintained production source translation units.
    link_sources = sources
    probe_flags = ["-include", "tests/reference/actor_current_fixture.hpp"] if slug == "sonic_player" else []
    if probe_flags:
        probe_object = output / "sonic-player-frozen-probe.o"
        run(FLAGS + includes + probe_flags + ["-fPIC", "-c", probe, "-o", str(probe_object)], commands)
        link_sources = [str(probe_object)] + sources[1:]
    command = FLAGS + ["-dynamiclib"] + includes + link_sources
    if "table" not in slug:
        command.append(str(ROOT / "build/animation-judges/softfloat/softfloat.a"))
    if slug == "sonic_player":
        command.append(str(ROOT / "build/animation-judges/bochs/libbochs_fpatan.a"))
    command += ["-o", str(library)]
    run(command, commands)
    dependencies = set(original_sources)
    for source in sources:
        result = subprocess.run(FLAGS + includes + (probe_flags if source == probe else []) + ["-MM", source], cwd=ROOT, capture_output=True, text=True, check=True)
        dependencies.update(result.stdout.replace("\\\n", " ").split(":", 1)[1].split())
    receipt = dict(commands=commands, library=str(library), library_sha256=sha(library),
                   source_dependencies={str(path): sha(ROOT / path) for path in sorted(dependencies)},
                   fault=fault, maintained_archives_used=False)
    (output / (slug + "-build.json")).write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("slug", choices=["sonic_animation_table", "player_animation", "player_animation_blend", "table_characterization", "player_startup", "gamma_selection_core", "sonic_player"])
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--fault", choices=TABLE_FAULTS)
    args = parser.parse_args()
    print(json.dumps(build(args.output.resolve(), args.slug, fault=args.fault)))


if __name__ == "__main__":
    main()
