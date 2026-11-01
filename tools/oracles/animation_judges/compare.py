"""Compare all six future captures with this frozen before-Source packet.

Full streams include native failures, FP flags and controls. Aggregate equality
does not suffice. Missing artifacts are errors, never an implicit pass.
"""
import argparse
import gzip
import hashlib
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[3]


def raw(path):
    content = path.read_bytes()
    return gzip.decompress(content) if path.suffix == ".gz" else content


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("current", type=pathlib.Path, help="Directory containing all six <slug>/ captures")
    parser.add_argument("--packet", type=pathlib.Path,
                        default=ROOT / "docs/refactor-inventory/packets/animation-judges")
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    baseline = json.loads((args.packet / "current-baseline.json").read_text())
    receipts = []
    for slug, frozen in baseline.items():
        before_path = pathlib.Path(frozen["stream_local_path"])
        before_file = before_path.read_bytes()
        assert hashlib.sha256(before_file).hexdigest() == frozen["stream_file_sha256"], "Frozen stream drift: " + slug
        after_path = args.current / slug / before_path.name
        before, after = raw(before_path), raw(after_path)
        row = dict(slug=slug, full_stream_equal=before == after,
                   before_sha256=hashlib.sha256(before).hexdigest(),
                   current_sha256=hashlib.sha256(after).hexdigest(),
                   before_cases=len(before.splitlines()), current_cases=len(after.splitlines()),
                   inherited_verdict=frozen["verdict"])
        if before != after:
            first = next((index for index, pair in enumerate(zip(before.splitlines(), after.splitlines()))
                          if pair[0] != pair[1]), min(row["before_cases"], row["current_cases"]))
            row["first_differing_case"] = first
        receipts.append(row)
    args.output.write_text(json.dumps(receipts, indent=2) + "\n")
    print(json.dumps({row["slug"]: row["full_stream_equal"] for row in receipts}))
    return int(not all(row["full_stream_equal"] for row in receipts))


if __name__ == "__main__":
    raise SystemExit(main())
