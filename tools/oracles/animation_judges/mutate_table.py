"""Compile table faults against the shared initializer and run the full new judge."""
import argparse
import json
import pathlib
import subprocess
import sys

from build import TABLE_FAULTS, build, sha


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    receipts = []
    for name in TABLE_FAULTS:
        output = args.output.resolve() / name
        compiled = build(output, "table_characterization", fault=name)
        command = [sys.executable, str(pathlib.Path(__file__).with_name("table.py")),
                   str(args.exe), compiled["library"], str(output / "judge")]
        result = subprocess.run(command, capture_output=True, text=True)
        (output / "judge.log").write_text(result.stdout + result.stderr)
        report_path = output / "judge/report.json"
        report = json.loads(report_path.read_text()) if report_path.exists() else {}
        receipts.append(dict(name=name, build_exit_status=0, judge_exit_status=result.returncode,
                             cases=report.get("cases"), mismatches=len(report.get("mismatches", [])),
                             caught=result.returncode == 1 and bool(report.get("mismatches")),
                             library_sha256=compiled["library_sha256"],
                             report_sha256=sha(report_path) if report_path.exists() else None,
                             first_mismatch=next(iter(report.get("mismatches", [])), None)))
        print(json.dumps(receipts[-1]), flush=True)
    (args.output / "receipts.json").write_text(json.dumps(receipts, indent=2) + "\n")
    return int(not all(receipt["caught"] for receipt in receipts))


if __name__ == "__main__":
    raise SystemExit(main())
