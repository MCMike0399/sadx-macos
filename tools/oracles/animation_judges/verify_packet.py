"""Read-only integrity check for the bounded packet; not the repository gate."""
import argparse
import gzip
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[3]
PACKET = ROOT / "docs/refactor-inventory/packets/animation-judges"
BASE = "99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(name):
    return json.loads((PACKET / name).read_text())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    closure = load("source-closure.json")
    for path, expected in closure["files"].items():
        assert sha(ROOT / path) == expected, "Source closure drift: " + path
    artifacts = load("local-artifacts.json")
    for path, expected in artifacts.items():
        assert sha(ROOT / path) == expected["sha256"], "Local artifact drift: " + path
    manifest = load("inherited-manifest.json")
    assert sha(ROOT / manifest["path"]) == manifest["sha256"]
    assert len(json.loads((ROOT / manifest["path"]).read_text())["files"]) == 946
    changed = subprocess.check_output(["git", "diff", "--name-only", BASE, "--", "src"], cwd=ROOT)
    assert not changed, "Maintained Source changed"
    counts = {"sonic_animation_table": (103, 0), "player_animation": (16440, 4140),
              "player_animation_blend": (7944, 1668), "sonic_player": (28800, 2840),
              "player_startup": (2004, 924), "gamma_selection_core": (1422, 498)}
    identity_counts = {}
    for slug, frozen in load("current-baseline.json").items():
        stream = pathlib.Path(frozen["stream_local_path"])
        assert sha(stream) == frozen["stream_file_sha256"]
        assert sha(pathlib.Path(frozen["library_local_path"])) == frozen["library_sha256"]
        raw = stream.read_bytes()
        if stream.suffix == ".gz":
            raw = gzip.decompress(raw)
        assert len(raw.splitlines()) == counts[slug][0]
        identity_counts[slug] = {}
        for category, index in frozen["identity_index"].items():
            total = 0
            for part in index["parts"]:
                path = ROOT / part["path"]
                assert sha(path) == part["sha256"]
                assert len(json.loads(path.read_text())) == part["records"]
                total += part["records"]
            assert total == index["count"]
            identity_counts[slug][category] = total
        assert identity_counts[slug]["fp_partials"] == counts[slug][1]
    assert all(row["exit_status"] == 0 for row in load("existing-units.json"))
    assert len(load("existing-units.json")) == 7
    table = load("table-missingness.json")
    assert table["passed"] and table["cases"] == 8727 and table["incomplete_new_rows_at_read"] == 0
    assert all(row["caught"] for row in load("table-mutations.json"))
    motion = load("motion-mutations.json")
    assert motion["baseline"]["judge_exit_status"] == 0 and motion["baseline"]["cases"] == 114
    assert len(motion["faults"]) == 8 and all(row["caught"] for row in motion["faults"])
    gamma = load("gamma-assets.json")
    assert [row["judge_exit_status"] for row in gamma["runs"]] == [0, 1]
    assert gamma["runs"][0]["rows"] == 91
    gamma_rows = load("gamma-assets-rows.json")
    assert hashlib.sha256(("\n".join(gamma_rows["lines"]) + "\n").encode()).hexdigest() == gamma_rows["raw_sha256"]
    assert gamma_rows["raw_sha256"] == gamma["runs"][0]["stream_sha256"]
    assert load("comparator-fault.json")["caught"]
    assert load("reserved-holdouts.json")["generation_or_execution_performed"] is False
    receipt = dict(source_files=len(closure["files"]), local_artifacts=len(artifacts),
                   existing_units=7, interface_cases=114, table_cases=8727, gamma_rows=91,
                   compiled_algorithm_faults_caught=13, identities=identity_counts,
                   maintained_source_identical=True, repository_gate_run=False,
                   global_manifest_refreshed=False, holdouts_run=False)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({key: value for key, value in receipt.items() if key != "identities"}))


if __name__ == "__main__":
    main()
