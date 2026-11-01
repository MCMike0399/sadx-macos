"""Narrow imported-table characterization and a compiled unavailable-to-null fault."""
import argparse
import json
import pathlib
import subprocess

from build import FLAGS, ROOT, sha


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("imports", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    receipts = []
    for fault in (False, True):
        output = args.output / ("unavailable_to_null" if fault else "baseline")
        output.mkdir(parents=True, exist_ok=True)
        includes = ["-Isrc"]
        if fault:
            header = output / "include/apps/native_gamma_actor_assets.hpp"
            header.parent.mkdir(parents=True, exist_ok=True)
            body = (ROOT / "src/apps/native_gamma_actor_assets.hpp").read_text()
            old = "record.action.status = recipe.resource.status;"
            new = "record.action.status = recipe.resource.status == ActorResourceStatus::Unavailable ? ActorResourceStatus::KnownNull : recipe.resource.status;"
            assert body.count(old) == 1
            header.write_text(body.replace(old, new))
            includes.insert(0, "-I" + str(output / "include"))
        sources = ["tests/unit/animation_gamma_assets.mm", "src/engine/character_pack.cpp",
                   "src/player/player_state.cpp", "src/cpu/x87_math.cpp"]
        command = FLAGS + ["-fobjc-arc"] + includes + sources + [
            str(ROOT / "build/animation-judges/softfloat/softfloat.a"), "-framework", "Foundation", "-o", str(output / "test")]
        built = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (output / "build.log").write_text(built.stdout + built.stderr)
        receipt = dict(fault=fault, command=command, build_exit_status=built.returncode)
        if not built.returncode:
            result = subprocess.run([str(output / "test"), str(args.imports)], capture_output=True, text=True)
            (output / "rows.txt").write_text(result.stdout)
            (output / "judge.log").write_text(result.stderr)
            receipt.update(judge_exit_status=result.returncode, rows=len(result.stdout.splitlines()),
                           stream_sha256=sha(output / "rows.txt"), executable_sha256=sha(output / "test"))
        dependencies = set(sources)
        for source in sources:
            deps = subprocess.run(FLAGS + includes + ["-fobjc-arc", "-MM", source], cwd=ROOT, capture_output=True, text=True, check=True)
            dependencies.update(deps.stdout.replace("\\\n", " ").split(":", 1)[1].split())
        receipt["source_dependencies"] = {path: sha(ROOT / path) for path in sorted(dependencies)}
        receipts.append(receipt)
        print(json.dumps({key: value for key, value in receipt.items() if key not in ("command", "source_dependencies")}))
    assets = [args.imports / "characters" / name for name in
              ("sonic-selection.actions", "gamma-actor.json", "sonic-actor.json")]
    write = dict(runs=receipts, inputs={str(path.resolve()): dict(bytes=path.stat().st_size, sha256=sha(path)) for path in assets},
                 limits="Imported metadata/status conversion only; no native Gamma binding, PSetMotion, rendering or playback is executed")
    (args.output / "receipts.json").write_text(json.dumps(write, indent=2) + "\n")
    return int(not (receipts[0].get("judge_exit_status") == 0 and receipts[1].get("judge_exit_status") == 1))


if __name__ == "__main__":
    raise SystemExit(main())
