"""Freeze packet receipts, complete identity indexes and actual source closure.

Does not update the repository judge manifest or copy any asset into Git.
"""
import argparse
import ast
import hashlib
import json
import pathlib
import shutil
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[3]
BASE = "99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3"
PACKET = ROOT / "docs/refactor-inventory/packets/animation-judges"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("dll", type=pathlib.Path)
    args = parser.parse_args()
    assert not subprocess.check_output(["git", "diff", "--name-only", BASE, "--", "src"], cwd=ROOT), "Before-Source seal refuses maintained Source changes"
    assert not subprocess.check_output(["git", "ls-files", "--others", "--exclude-standard", "src"], cwd=ROOT), "Before-Source seal refuses untracked Source"
    build = ROOT / "build/animation-judges"
    slugs = ["sonic_animation_table", "player_animation", "player_animation_blend",
             "sonic_player", "player_startup", "gamma_selection_core"]
    summary = {}
    dependencies = set()
    # Preserve the old protected universe verbatim; no incomplete worktree refresh.
    manifest = json.loads((ROOT / "reference/judge-manifest.json").read_text())
    write(PACKET / "inherited-manifest.json", dict(path="reference/judge-manifest.json",
          sha256=sha(ROOT / "reference/judge-manifest.json"), top_level_keys=list(manifest),
          refreshed=False, instruction="Root must preserve every inherited path and all 53 raw corpora when sealing integration"))
    for slug in slugs:
        directory = build / "baseline" / slug
        if not directory.exists():
            directory = build / slug
        report = json.loads((directory / "report.json").read_text())
        ledger = json.loads((directory / "identities.json").read_text())
        build_receipt = json.loads((build / "baseline" / (slug + "-build.json")).read_text())
        library = pathlib.Path(build_receipt["library"])
        assert report["native_library_sha256"] == sha(library)
        dependencies.update(build_receipt["source_dependencies"])
        if slug in ("sonic_animation_table", "player_animation", "player_animation_blend"):
            old = json.loads((ROOT / "reference/oracle-reports" / (slug + ".json")).read_text())
            assert report["case_sha256"] == old["case_sha256"]
            for key in ("cases", "byte_mismatches", "stored_mismatches", "boundary_mismatches", "status_mismatches", "fp_mismatches", "passed"):
                if key in old:
                    assert report[key] == old[key]
        parts = {}
        for category, rows in ledger["identities"].items():
            paths = []
            for start in range(0, len(rows), 500):
                path = PACKET / "identities" / f"{slug}-{category}-{start // 500:03}.json"
                write(path, rows[start:start + 500])
                paths.append(dict(path=str(path.relative_to(ROOT)), sha256=sha(path), records=min(500, len(rows) - start)))
            parts[category] = dict(count=len(rows), parts=paths)
        stream = directory / ("cases.jsonl" if slug in slugs[:3] else "cases.jsonl.gz")
        summary[slug] = dict(report=report, identity_index=parts, stream_local_path=str(stream),
                             stream_file_sha256=sha(stream), library_local_path=str(library),
                             library_sha256=sha(library), build_receipt=build_receipt,
                             verdict="pass" if report.get("passed", report.get("full_emulator_match", False)) else "partial")
    write(PACKET / "current-baseline.json", summary)
    context = json.loads((ROOT / "docs/refactor-inventory/context-animation.json").read_text())
    gamma_assets = json.loads((build / "gamma-assets/receipts.json").read_text())
    dependencies.update(gamma_assets["runs"][0]["source_dependencies"])
    dependencies.update(context["currentSourceHashes"])
    dependencies.update(context["tracked_evidence_hashes"])
    dependencies.update(str(path.relative_to(ROOT)) for path in (ROOT / "third_party").rglob("*")
                        if path.is_file() and path.suffix in (".c", ".cpp", ".cc", ".h", ".txt", ".sh"))
    dependencies.update(str(path.relative_to(ROOT)) for path in (ROOT / "tools/oracles/animation_judges").glob("*.py"))
    dependencies.update(["tests/unit/animation_characterization.cpp", "tests/probes/native_animation_table_characterization_probe.cpp"])
    # Local Python imports also form part of the actual source execution closure.
    pending = ["tools/oracles/verify_original_" + slug + ".py" for slug in slugs]
    seen = set()
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        dependencies.add(path)
        tree = ast.parse((ROOT / path).read_text())
        for node in ast.walk(tree):
            modules = [node.module] if isinstance(node, ast.ImportFrom) else [item.name for item in node.names] if isinstance(node, ast.Import) else []
            for module in modules:
                if module:
                    for folder in ("tools/oracles", "tools/lib"):
                        candidate = folder + "/" + module.replace(".", "/") + ".py"
                        if (ROOT / candidate).is_file():
                            pending.append(candidate)
    hashes = {path: sha(ROOT / path) for path in sorted(dependencies)}
    write(PACKET / "source-closure.json", dict(base_commit=BASE, files=hashes,
          method="Clang -MM for every compiled translation unit, recursive local Python AST imports, plus explicit emitters, operation metadata, backing/type owners and cached evidence",
          existing_verifier_native_sources_changed=False))
    source_before = build / "source-before"
    for path in hashes:
        # Clang may list absolute SDK metadata even with -MM. Pin a local copy;
        # an absolute operand must never escape the packet directory.
        target = source_before / ("external" + path if pathlib.Path(path).is_absolute() else path)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / path, target)
    ast_proof = {}
    for slug in slugs:
        path = "tools/oracles/verify_original_" + slug + ".py"
        old = subprocess.check_output(["git", "show", BASE + ":" + path], cwd=ROOT).decode()
        current = (ROOT / path).read_text()
        assert old == current
        ast_proof[path] = dict(byte_identical=True, ast_sha256=hashlib.sha256(
            ast.dump(ast.parse(current), include_attributes=False).encode()).hexdigest())
    write(PACKET / "verifier-ast-proof.json", ast_proof)
    artifacts = {}
    for path in build.rglob("*"):
        if path.name == "seal.log":
            continue  # This command's active redirection is not a frozen artifact.
        if path.is_file() and "source-before" not in path.parts and (path.suffix in (".json", ".jsonl", ".gz", ".dylib", ".a", ".bin", ".f32", ".log", ".txt") or path.name == "test" or path.name.endswith("-test")):
            artifacts[str(path.relative_to(ROOT))] = dict(bytes=path.stat().st_size, sha256=sha(path))
    artifacts[str(args.exe.resolve())] = dict(bytes=args.exe.stat().st_size, sha256=sha(args.exe))
    artifacts[str(args.dll.resolve())] = dict(bytes=args.dll.stat().st_size, sha256=sha(args.dll))
    artifacts.update(gamma_assets["inputs"])
    write(PACKET / "local-artifacts.json", artifacts)
    for source, destination in [("source-order.json", "source-order.json"),
                                ("existing-units.json", "existing-units.json"),
                                ("table-characterization-final/report.json", "table-missingness.json"),
                                ("table-faults-final/receipts.json", "table-mutations.json"),
                                ("motion-faults-sealed/receipts.json", "motion-mutations.json"),
                                ("gamma-assets/receipts.json", "gamma-assets.json"),
                                ("full-stream-replay.json", "full-stream-replay.json"),
                                ("comparator-fault-final/receipt.json", "comparator-fault.json")]:
        shutil.copy2(build / source, PACKET / destination)
    shutil.copy2(build / "motion-faults-sealed/baseline/cases.jsonl", PACKET / "missingness.jsonl")
    gamma_rows = build / "gamma-assets/baseline/rows.txt"
    write(PACKET / "gamma-assets-rows.json", dict(raw_sha256=sha(gamma_rows),
          lines=gamma_rows.read_text().splitlines()))
    print(json.dumps(dict(baselines=len(summary), source_files=len(hashes), local_artifacts=len(artifacts))))


if __name__ == "__main__":
    main()
