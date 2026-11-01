"""Meaningful compiled animation faults; only private source copies are changed."""
import argparse
import json
import pathlib
import subprocess
import sys

from build import COMMON, FLAGS, ROOT, build, sha

from migration_faults import MOTION_FAULTS as FAULTS, EXTRA_REPLACEMENTS



def unit(output, replacement=None):
    output.mkdir(parents=True, exist_ok=True)
    sources = COMMON.copy()
    overlay = []
    if replacement:
        old, new = replacement
        if pathlib.Path(old).suffix == ".hpp":
            include = pathlib.Path(new)
            for _ in pathlib.Path(old).relative_to("src").parts:
                include = include.parent
            overlay = ["-I" + str(include)]
        else:
            sources[sources.index(old)] = str(new)
    command = FLAGS + overlay + ["-Isrc", "tests/unit/animation_characterization.cpp"] + sources + [
        str(ROOT / "build/animation-judges/softfloat/softfloat.a"), "-o", str(output / "test")]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    (output / "build.log").write_text(result.stdout + result.stderr)
    receipt = dict(command=command, build_exit_status=result.returncode)
    if not result.returncode:
        test = subprocess.run([str(output / "test")], capture_output=True, text=True)
        (output / "cases.jsonl").write_text(test.stdout)
        (output / "judge.log").write_text(test.stderr)
        receipt.update(judge_exit_status=test.returncode, cases=len(test.stdout.splitlines()),
                       stream_sha256=sha(output / "cases.jsonl"), executable_sha256=sha(output / "test"))
    return receipt


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    baseline = unit(args.output / "baseline")
    assert baseline["build_exit_status"] == 0 and baseline["judge_exit_status"] == 0
    receipts = []
    for name, (source, old, new, judge) in FAULTS.items():
        output = args.output / name
        output.mkdir(parents=True, exist_ok=True)
        body = (ROOT / source).read_text()
        assert body.count(old) == 1
        replacement = output / "include" / pathlib.Path(source).relative_to("src")
        replacement.parent.mkdir(parents=True, exist_ok=True)
        body = body.replace(old, new)
        if name in EXTRA_REPLACEMENTS:
            extra_old, extra_new = EXTRA_REPLACEMENTS[name]
            assert body.count(extra_old) == 1
            body = body.replace(extra_old, extra_new)
        replacement.write_text(body)
        if judge == "unit":
            receipt = unit(output, (source, replacement))
            receipt["caught"] = receipt["build_exit_status"] == 0 and receipt.get("judge_exit_status") == 1
        else:
            compiled = build(output, "player_animation_blend", (source, replacement))
            command = [sys.executable, str(pathlib.Path(__file__).with_name("capture.py")),
                       "player_animation_blend", str(args.exe), compiled["library"], str(output / "judge")]
            result = subprocess.run(command, capture_output=True, text=True)
            (output / "judge.log").write_text(result.stdout + result.stderr)
            report_path = output / "judge/report.json"
            report = json.loads(report_path.read_text()) if report_path.exists() else {}
            receipt = dict(build_exit_status=0, judge_exit_status=result.returncode,
                           library_sha256=compiled["library_sha256"], cases=report.get("cases"),
                           stored_mismatches=report.get("stored_mismatches"),
                           status_mismatches=report.get("status_mismatches"),
                           caught=result.returncode == 1 and bool(report.get("stored_mismatches")))
        receipt.update(name=name, source=source, old=old, new=new,
                       private_source_sha256=sha(replacement))
        receipts.append(receipt)
        print(json.dumps({k: v for k, v in receipt.items() if k not in ("command", "old", "new")}), flush=True)
    (args.output / "receipts.json").write_text(json.dumps(dict(baseline=baseline, faults=receipts), indent=2) + "\n")
    return int(not all(receipt["caught"] for receipt in receipts))


if __name__ == "__main__":
    raise SystemExit(main())
