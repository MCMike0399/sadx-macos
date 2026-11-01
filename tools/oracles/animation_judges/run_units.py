"""Narrow source-only builds of the seven inherited animation consumer units."""
import argparse
import json
import pathlib
import subprocess

from build import FLAGS, ROOT, COMMON, STARTUP, SONIC, sha


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dll", type=pathlib.Path)
    parser.add_argument("assets", type=pathlib.Path)
    parser.add_argument("constants", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    table = ["src/player/sonic_animation_table.cpp", "src/guest/sonic_animation_table_reference.cpp"]
    decode = ["src/guest/player_animation_decode_reference.cpp"]
    groups = {
        "player_animation_decode": (decode, [str(args.dll)]),
        "player_animation": (list(dict.fromkeys(COMMON + decode + table)), [str(args.dll), str(args.constants)]),
        "player_initialization": (STARTUP, []),
        "player_startup": (STARTUP, [str(args.assets / "player-parameters.bin")]),
        "sonic_player": (SONIC, [str(args.assets / name) for name in ["sonic-player-constants.bin", "sonic-action-dispatch.bin", "ninja-trig.f32", "ninja-unit.f32"]]),
        "sonic_animation_table": (table, []),
        "gamma_selection_core": (list(dict.fromkeys(STARTUP + ["src/player/gamma_selection_core.cpp"])), []),
    }
    groups["sonic_player_native"] = groups["sonic_player"]
    receipts = []
    for slug, (sources, inputs) in groups.items():
        source = "tests/unit/" + ("sonic_player" if slug == "sonic_player_native" else slug) + ".cpp"
        includes = ["-Isrc", "-Ithird_party/bochs_fpatan", "-Ithird_party/bochs_fpatan/source", "-Ithird_party/bochs_fpatan/source/softfloat3e/include"]
        commands = []
        unit_source = source
        if slug == "sonic_player_native":
            body = (ROOT / source).read_text()
            replacements = {
                "PlayerAnimationResources animation_resources;": "animation::Resources animation_resources;",
                "        PlayerAnimationConstants animation_constants{0,1,4294967296.0F};\n": "",
                "PlayerAnimationRuntime animation_runtime": "animation::Runtime animation_runtime",
                ",animation_resources,animation_constants}": ",animation_resources}",
                "rt.animation_runtime=&animation_runtime;": "rt.native_animation_runtime=&animation_runtime;",
                "std::make_shared<PlayerAnimationTable>()": "std::make_shared<animation::Table>()",
                "row.action=AnimationResourceReference{0}": "row.action={animation::ResourceStatus::Loaded,0,{}}",
                "animation_resources.actions[0].motion=AnimationResourceReference{0}": "animation_resources.actions[0].motion=animation::ResourceReference{0}",
            }
            for old, new in replacements.items():
                assert body.count(old) == 1, old
                body = body.replace(old, new)
            generated = args.output / "frozen-sonic-native-contract.cpp"
            generated.write_text(body)
            unit_source = str(generated)
        if slug in ("sonic_player", "sonic_player_native"):
            test_source = unit_source
            unit_source = str(args.output / (slug + "-frozen-unit.o"))
            command = FLAGS + includes + ["-include", "tests/reference/actor_current_fixture.hpp", "-c", test_source, "-o", unit_source]
            result = subprocess.run(command, capture_output=True, text=True)
            commands.append(dict(command=command, exit_status=result.returncode, output=result.stdout + result.stderr))
            assert result.returncode == 0, commands[-1]
        executable = args.output / slug
        command = FLAGS + includes + [unit_source] + sources
        if slug not in ("sonic_animation_table", "player_animation_decode"):
            command += [str(ROOT / "build/animation-judges/softfloat/softfloat.a")]
        if slug in ("sonic_player", "sonic_player_native"):
            command += [str(ROOT / "build/animation-judges/bochs/libbochs_fpatan.a")]
        command += ["-o", str(executable)]
        result = subprocess.run(command, capture_output=True, text=True)
        commands.append(dict(command=command, exit_status=result.returncode, output=result.stdout + result.stderr))
        assert result.returncode == 0, commands[-1]
        command = [str(executable)] + inputs
        result = subprocess.run(command, capture_output=True, text=True)
        commands.append(dict(command=command, exit_status=result.returncode, output=result.stdout + result.stderr))
        receipt = dict(slug=slug, commands=commands, executable_sha256=sha(executable),
                       input_hashes={path: sha(pathlib.Path(path)) for path in inputs},
                       unit_source_sha256=sha(ROOT / source), exit_status=result.returncode)
        receipts.append(receipt)
        (args.output / "receipts.json").write_text(json.dumps(receipts, indent=2) + "\n")
        print(slug, result.returncode, flush=True)
        assert result.returncode == 0, receipt
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
