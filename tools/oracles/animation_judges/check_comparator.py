"""Prove that unchanged aggregate FP-partial counts cannot hide identity drift."""
import argparse
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    packet = ROOT / "docs/refactor-inventory/packets/animation-judges"
    manifest = json.loads((packet / "current-baseline.json").read_text())
    for slug, record in manifest.items():
        target = args.output / slug
        target.mkdir(exist_ok=True)
        source = pathlib.Path(record["stream_local_path"])
        destination = target / source.name
        if slug == "player_animation":
            records = [json.loads(line) for line in source.read_text().splitlines()]
            partials_before = sum(row["native"][9] != row["source"][0][9] for row in records)
            changed = next(row for row in records if row["native"][9] != row["source"][0][9])
            changed["native"][9] ^= 32
            partials_after = sum(row["native"][9] != row["source"][0][9] for row in records)
            assert partials_before == partials_after == 4140
            destination.write_text("".join(json.dumps(row, separators=(",", ":")) + "\n" for row in records))
        elif not destination.exists():
            destination.symlink_to(source)
    command = [sys.executable, str(pathlib.Path(__file__).with_name("compare.py")),
               str(args.output), "--output", str(args.output / "comparison.json")]
    result = subprocess.run(command, capture_output=True, text=True)
    receipt = dict(command=command, exit_status=result.returncode, expected_exit_status=1,
                   changed_case=changed["case"], partials_before=partials_before,
                   partials_after=partials_after, caught=result.returncode == 1,
                   comparison=json.loads((args.output / "comparison.json").read_text()))
    (args.output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({key: receipt[key] for key in ("exit_status", "changed_case", "partials_before", "partials_after", "caught")}))
    return int(not receipt["caught"])


if __name__ == "__main__":
    raise SystemExit(main())
