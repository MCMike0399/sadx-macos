"""Compile six isolated Sonic faults; never modify maintained source or judges."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

from observe import sha

ROOT = Path(__file__).resolve().parents[3]
FLAGS = ["xcrun", "clang++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
         "-ffp-contract=off", "-fno-fast-math", "-arch", "arm64", "-mmacosx-version-min=14.0"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("support", type=Path, help="Private source build with both support archives")
    parser.add_argument("baseline", type=Path, help="Frozen complete development capture directory")
    parser.add_argument("output", type=Path)
    parser.add_argument("--fault", action="append", help="Rerun a named fault; default runs all six")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    support = args.support.resolve()
    numeric = [str(ROOT / "build/animation-judges/softfloat/softfloat.a"),
               str(ROOT / "build/animation-judges/bochs/libbochs_fpatan.a")]
    faults = json.loads(Path(__file__).with_name("mutants.json").read_text())
    if args.fault:
        unknown = set(args.fault) - {fault["name"] for fault in faults}
        if unknown:
            raise ValueError("Unknown faults: " + str(sorted(unknown)))
        faults = [fault for fault in faults if fault["name"] in args.fault]
    receipts = []
    for fault in faults:
        output = args.output / fault["name"]
        replacement = output / "include" / Path(fault["source"]).relative_to("src")
        replacement.parent.mkdir(parents=True, exist_ok=True)
        original = (ROOT / fault["source"]).read_text()
        if original.count(fault["old"]) != 1:
            raise ValueError("Mutation anchor is not unique: " + fault["name"])
        replacement.write_text(original.replace(fault["old"], fault["new"]))
        flags = FLAGS + ["-I" + str((output / "include").resolve()), "-Isrc"]
        if fault["judge"] == "native_unit":
            command = flags + ["tests/unit/sonic_controller_native.cpp", "src/player/sonic_player.cpp",
                               "src/runtime/native_sonic_binding.cpp", "src/math/ninja_length.cpp",
                               str(support / "native-support.a")] + numeric + ["-o", str(output / "test")]
        else:
            unit = fault["judge"] == "reference_unit"
            if unit:
                witness = output / "witness.o"
                unit_command = flags + ["-include", "tests/reference/actor_current_fixture.hpp",
                                        "-c", "tests/unit/sonic_actions.cpp", "-o", str(witness)]
                unit_build = subprocess.run(unit_command, cwd=ROOT, capture_output=True, text=True)
                (output / "witness-build.json").write_text(json.dumps(dict(
                    command=unit_command, exit_status=unit_build.returncode,
                    output=unit_build.stdout + unit_build.stderr), indent=2) + "\n")
                if unit_build.returncode:
                    raise RuntimeError("Reference witness did not compile")
                command = flags + [str(witness)]
            else:
                probe = "sonic-probe.o" if fault["judge"] == "sonic_player" else "action-probe.o"
                command = flags + ["-dynamiclib", str(support / probe)]
            command += ["src/guest/sonic_player_reference.cpp", "src/player/sonic_player.cpp",
                        str(support / "reference-support.a")] + numeric
            command += ["-o", str(output / ("test" if unit else "probe.dylib"))]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (output / "build.log").write_text(build.stdout + build.stderr)
        receipt = dict(name=fault["name"], build_command=command, build_exit_status=build.returncode,
                       original_sha256=sha(original.encode()), replacement_sha256=sha(replacement.read_bytes()),
                       caught=False)
        if build.returncode == 0:
            if fault["judge"] in ("native_unit", "reference_unit"):
                command = [str((output / "test").resolve())]
                if fault["judge"] == "reference_unit":
                    command.append(str((args.baseline / "player-observations/report.constants.bin").resolve()))
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                (output / "judge.log").write_text(result.stdout + result.stderr)
                receipt.update(judge_command=command, judge_exit_status=result.returncode,
                               caught=result.returncode != 0)
            else:
                folder = "player-observations" if fault["judge"] == "sonic_player" else "action-observations"
                report = output / "observations.json"
                command = [sys.executable, str(Path(__file__).with_name("observe.py")),
                           str(args.baseline / folder), str(output / "probe.dylib"), str(report)]
                judged = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                (output / "judge.log").write_text(judged.stdout + judged.stderr)
                if report.exists():
                    result = json.loads(report.read_text())
                    receipt.update(records=result["records"], differences=len(result["differences"]),
                                   supplement=result["supplement"],
                                   judge_exit_status=judged.returncode,
                                   caught=judged.returncode == 1 and not result["exact"])
        receipts.append(receipt)
        print(json.dumps({key: receipt[key] for key in ("name", "build_exit_status", "caught")}), flush=True)
        (args.output / "receipts.json").write_text(json.dumps(receipts, indent=2) + "\n")
    return int(not all(receipt["caught"] for receipt in receipts))


if __name__ == "__main__":
    raise SystemExit(main())
