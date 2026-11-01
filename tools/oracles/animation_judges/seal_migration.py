"""Validate/copy bounded migration receipts without changing the global manifest."""
import argparse
import gzip
import hashlib
import json
import pathlib
import subprocess

from build import ROOT, sha

PACKET = ROOT / "docs/refactor-inventory/packets/animation-contract"
BEFORE = ROOT / "docs/refactor-inventory/packets/animation-judges"
RAW = ROOT / "build/animation-contract"


def read(path):
    data = path.read_bytes()
    return gzip.decompress(data) if path.suffix == ".gz" else data


def load(path):
    return json.loads(path.read_text())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=pathlib.Path, default=PACKET)
    args = parser.parse_args()
    output = args.output
    output.mkdir(parents=True, exist_ok=True)
    source = load(PACKET / "source-hashes.json")
    assert all(sha(ROOT / path) == value for path, value in source["files"].items())
    baseline = load(BEFORE / "current-baseline.json")
    comparisons = load(RAW / "review-candidate/full-stream-comparison.json")
    assert len(comparisons) == 6 and all(row["full_stream_equal"] for row in comparisons)
    builds = {}
    for slug, frozen in baseline.items():
        before_path = pathlib.Path(frozen["stream_local_path"])
        after_path = RAW / "review-candidate" / slug / before_path.name
        assert sha(before_path) == frozen["stream_file_sha256"]
        assert read(before_path) == read(after_path)
        builds[slug] = load(RAW / "review-candidate" / (slug + "-build.json"))
        assert sha(pathlib.Path(builds[slug]["library"])) == builds[slug]["library_sha256"]
        for path, expected in builds[slug]["source_dependencies"].items():
            assert sha(ROOT / path) == expected, path
    table = load(RAW / "table-canonical/report.json")
    assert table["passed"] and table["cases"] == 8727
    assert sha(RAW / "table-canonical/cases.jsonl") == load(BEFORE / "table-missingness.json")["stream_sha256"]
    motion = load(RAW / "motion-faults-canonical/receipts.json")
    assert motion["baseline"]["judge_exit_status"] == 0 and motion["baseline"]["cases"] == 114
    assert (RAW / "motion-faults-canonical/baseline/cases.jsonl").read_bytes() == (BEFORE / "missingness.jsonl").read_bytes()
    assert len(motion["faults"]) == 8 and all(row["caught"] for row in motion["faults"])
    table_faults = load(RAW / "table-faults-canonical/receipts.json")
    assert len(table_faults) == 4 and all(row["caught"] for row in table_faults)
    gamma = load(RAW / "gamma-assets/receipts.json")
    assert [row["judge_exit_status"] for row in gamma["runs"]] == [0, 1]
    assert gamma["runs"][0]["stream_sha256"] == load(BEFORE / "gamma-assets.json")["runs"][0]["stream_sha256"]
    units = load(RAW / "units-canonical/receipts.json")
    assert len(units) == 8 and all(row["exit_status"] == 0 for row in units)
    plain = {slug: load(RAW / "plain" / slug / "replay.json") for slug in
             ("sonic_animation_table", "player_animation", "player_animation_blend")}
    assert all(row["mismatch_count"] == 0 for row in plain.values())
    assert all(row["current_stream_sha256"] == row["frozen_stream_sha256"] for row in plain.values())
    publication = load(RAW / "publication-fault-final/receipt.json")
    assert [row["judge_exit_status"] for row in publication["runs"]] == [0, 1]
    manifest = load(ROOT / "reference/judge-manifest.json")["files"]
    assert all((ROOT / path).is_file() for path in manifest)
    raw_inputs = load(PACKET / "raw-input-preservation.json")
    assert len(raw_inputs) == 53 and all(sha(ROOT / row["path"]) == row["sha256"] for row in raw_inputs)
    for path in ["tests/unit/sonic_player.cpp", "tests/probes/native_sonic_player_probe.cpp"]:
        assert (ROOT / path).read_bytes() == subprocess.check_output(["git", "show", "99bdaf4:" + path], cwd=ROOT)
    assert load(BEFORE / "reserved-holdouts.json")["generation_or_execution_performed"] is False
    proof = dict(source_commit=source["source_commit"], source_files=len(source["files"]),
                 stock_full_streams=comparisons, plain_native_streams=plain,
                 table_characterization=table, interface_cases=114, gamma_rows=91,
                 inherited_faults_caught=13, additional_publication_fault_caught=True,
                 affected_units=8, inherited_manifest_paths_present=len(manifest), raw_inputs_exact=53,
                 global_manifest_refreshed=False, full_gate_run=False, journeys_run=False,
                 holdouts_run=False, independent_source_review_run=False,
                 actual_product_closure_run=False)
    copies = {"qualification.json": proof, "stock-builds.json": builds,
              "table-faults.json": table_faults, "motion-faults.json": motion,
              "gamma-assets.json": gamma, "units.json": units,
              "publication.json": publication}
    for filename, value in copies.items():
        (output / filename).write_text(json.dumps(value, indent=2) + "\n")
    print(json.dumps({key: value for key, value in proof.items() if key not in
                      ("stock_full_streams", "plain_native_streams", "table_characterization")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
