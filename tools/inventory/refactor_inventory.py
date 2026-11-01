#!/usr/bin/env python3
"""Reconcile tracked refactor scope; static observations are never semantic verdicts.

This tool reads local files and Git only. It does not import project tools, compile,
execute scripts, run judges, refresh baselines, or start analysis providers.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/refactor-inventory"
SOURCE_SUFFIXES = {".cpp", ".hpp", ".h", ".c", ".mm", ".m", ".inc", ".py", ".js",
                   ".mjs", ".ts", ".sh", ".ps1", ".java"}
DISPOSITIONS = {"open_review", "needs_migration", "already_conforming",
                "reference_infrastructure", "obsolete"}
PROVENANCE = re.compile(r"//\s*(FUNCTION|GLOBAL|DATA|PATCH|STUB|LIBRARY):\s*(.*)")


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args]).decode()


def read(path):
    return (ROOT / path).read_text(errors="replace")


def location(path, line):
    return {"path": path, "line": line}


def reviewed_scope(review, source_sha256):
    """Project authored semantic closure, never infer conformity from static counts."""
    conforming = review.get("disposition") == "already_conforming"
    required = ("semantic_justification", "resolved_owners", "resolved_consumers", "accepted_judges")
    if conforming and not all(review.get(field) for field in required):
        raise ValueError("Conformity requires semantic, owner, consumer and judge evidence")
    if conforming and review.get("reviewed_source_sha256") != source_sha256:
        raise ValueError("Conforming source changed; fresh semantic review required")
    return {"status": "reviewed_conforming" if conforming else "closure_open",
            "source_sha256": review.get("reviewed_source_sha256"),
            **{field: review.get(field, []) for field in required},
            "limits": review.get("classification_limits", [])}


def reviewed_module(review, members):
    """Require explicit whole-module review; file approval alone never closes it."""
    disposition = review.get("disposition", "open_review")
    if disposition not in DISPOSITIONS:
        raise ValueError("Invalid module disposition")
    actual = {member["path"]: member["sha256"] for member in members}
    if disposition != "open_review":
        if review.get("reviewed_members") != actual:
            raise ValueError("Module members changed; fresh whole-module review required")
        if not review.get("reason") or not review.get("evidence"):
            raise ValueError("Module disposition requires rationale and evidence")
    required = ("semantic_justification", "resolved_owners", "resolved_consumers", "accepted_judges")
    if disposition == "already_conforming":
        if not all(review.get(field) for field in required):
            raise ValueError("Module conformity requires semantic, owner, consumer and judge evidence")
        limits = review.get("classification_limits")
        if not ((isinstance(limits, str) and limits.strip()) or
                (isinstance(limits, list) and limits and
                 all(isinstance(limit, str) and limit.strip() for limit in limits))):
            raise ValueError("Module conformity requires explicit nonempty classification limits")
        if not members or not all(member["review_status"] == "reviewed_conforming" for member in members):
            raise ValueError("Module conformity requires every member's current semantic review")
    if disposition == "obsolete" and not review.get("obsolescence_proof"):
        raise ValueError("Module obsolescence requires maintained-consumer and behavior evidence")
    if disposition in {"reference_infrastructure", "obsolete"} and not all(
            member["disposition"] == disposition for member in members):
        raise ValueError("Module purpose/removal classification requires every member disposition")
    return {"disposition": disposition,
            "status": "reviewed_conforming" if disposition == "already_conforming" else "module_closure_open",
            "reviewed_members": review.get("reviewed_members", {}),
            "reason": review.get("reason", "Whole-module semantic/ownership review remains open."),
            "evidence": review.get("evidence", []),
            **{field: review.get(field, []) for field in required},
            "limits": review.get("classification_limits", []),
            "obsolescence_proof": review.get("obsolescence_proof", []),
            "findings": review.get("findings", []),
            "qualification_gaps": review.get("qualification_gaps", []),
            "destination": review.get("destination", [])}


def validate_module_evidence(value, paths):
    """Validate every projected evidence link, not only the top-level anchors."""
    if isinstance(value, dict):
        if "path" in value and value["path"] not in paths:
            raise ValueError("Missing module review evidence: " + str(value["path"]))
        for child in value.values():
            validate_module_evidence(child, paths)
    elif isinstance(value, list):
        for child in value:
            validate_module_evidence(child, paths)


def module(path):
    p = Path(path)
    if path.startswith("src/"):
        # Shared basename pairs form a module; no semantic merge by fuzzy name.
        return str(p.with_suffix(""))
    if path.startswith("tests/"):
        return str(p.with_suffix(""))
    if path.startswith("tools/"):
        return str(p.parent)
    return str(p.with_suffix(""))


def render():
    policy = json.loads((OUT / "review.json").read_text())
    paths = sorted(filter(None, git("ls-files", "-z").split("\0")))
    path_set = set(paths)
    base = policy["base_commit"]
    base_paths = set(filter(None, git("ls-tree", "-r", "--name-only", "-z", base).split("\0")))
    index = {}
    for row in git("ls-files", "--stage", "-z").split("\0"):
        if row:
            meta, path = row.split("\t", 1)
            mode, blob, stage = meta.split()
            if stage != "0":
                raise ValueError("Unmerged tracked path: " + path)
            index[path] = {"git_mode": mode, "index_blob": blob}
    primary = [p for p in paths if p.startswith(("src/", "tools/", "tests/"))]
    ref_code = [p for p in paths if p.startswith("reference/")
                and Path(p).suffix in SOURCE_SUFFIXES]
    # Historical scripts remain individually inventoried; not silently excluded.
    maintained = sorted(set(primary + [p for p in ref_code
                                      if p not in policy["external_reference_sources"]]))
    for path, review in policy["files"].items():
        if path not in maintained:
            raise ValueError("Stale reviewed path: " + path)
        if review["disposition"] not in DISPOSITIONS or not review["reason"]:
            raise ValueError("Invalid reviewed disposition: " + path)
        if review["disposition"] != "open_review" and not review.get("evidence"):
            raise ValueError("Disposition without evidence: " + path)
        if review["disposition"] == "already_conforming" and not all(
                review.get(key) for key in ("semantic_justification", "resolved_owners",
                                            "resolved_consumers", "accepted_judges")):
            raise ValueError("Conformity requires semantic/owner/consumer/judge justification: " + path)
        reviewed_scope(review, hashlib.sha256((ROOT / path).read_bytes()).hexdigest())
        if review["disposition"] == "obsolete" and not review.get("obsolescence_proof"):
            raise ValueError("Obsolescence requires maintained-consumer and behavior evidence: " + path)
        for evidence in review.get("evidence", []):
            if evidence["path"] not in path_set:
                raise ValueError("Missing review evidence: " + str(evidence))
    texts = {p: read(p) for p in maintained}
    # Also index build declarations and current profile/analysis configurations as
    # textual consumers; dynamic callbacks and generated links remain open review.
    extra = ["Makefile", "native-sources.txt", ".gitmodules"]
    consumer_texts = dict(texts)
    for p in extra:
        consumer_texts[p] = read(p)
    edges = collections.defaultdict(list)
    uses = collections.defaultdict(list)
    for p, body in consumer_texts.items():
        for number, line in enumerate(body.splitlines(), 1):
            for match in re.finditer(r'^\s*#\s*include\s*"([^"\n]+)"', line):
                name = match[1]
                candidates = [str(Path(p).parent / name), "src/" + name,
                              "tests/" + name, name]
                target = next((x for x in candidates if x in path_set), None)
                if target:
                    edge = {"from": p, "to": target, "line": number, "kind": "quoted_include"}
                    edges[p].append(edge)
                    uses[target].append({"path": p, "line": number, "kind": "quoted_include"})
            for target in re.findall(r'(?<![\w/])(?:src|tools|tests|reference)/[\w./-]+', line):
                if target in path_set and target != p:
                    uses[target].append({"path": p, "line": number, "kind": "literal_path"})
    # A test including a public header is a candidate judge for its basename .cpp.
    # This does not prove it exercises any particular function or all code paths.
    reverse = collections.defaultdict(set)
    for p, values in edges.items():
        for edge in values:
            reverse[edge["to"]].add(p)
    native = {x.strip() for x in read("native-sources.txt").splitlines()
              if x.strip() and not x.lstrip().startswith("#")}
    make = read("Makefile").replace("\\\n", " ")
    native_recipe = next(line for line in make.splitlines() if line.startswith("NATIVE_SOURCES :="))
    wildcard_match = re.search(r'\$\(wildcard ([^)]+)\)', native_recipe)
    if not wildcard_match:
        raise ValueError("Native recipe changed; review inventory build-membership extraction")
    native_patterns = wildcard_match[1].split()
    native_wildcard = {p.as_posix().removeprefix(ROOT.as_posix() + "/")
                       for pattern in native_patterns for p in ROOT.glob(pattern)} & path_set
    platform_recipes = "\n".join(line for line in make.splitlines()
                                 if line.startswith("NATIVE_PLATFORM_OBJECTS"))
    platform_objects = re.findall(r'build/obj/([\w/]+)\.o', platform_recipes)
    platform_paths = set(re.findall(r'src/[\w/]+\.(?:cpp|mm)', platform_recipes))
    platform_paths.update("src/" + obj + suffix for obj in platform_objects
                          for suffix in (".cpp", ".mm") if "src/" + obj + suffix in path_set)
    systems = policy["systems"]

    def system_for(p):
        for rule in policy["system_rules"]:
            if re.search(rule["pattern"], p):
                return rule["system"]
        raise ValueError("No system assignment: " + p)

    def judge_candidates(p):
        pending = [p]
        if p.startswith("src/"):
            pending += [str(Path(p).with_suffix(s)) for s in (".hpp", ".cpp")
                        if str(Path(p).with_suffix(s)) in path_set]
        found = set(pending)
        while pending:
            for parent in reverse[pending.pop()]:
                if parent not in found:
                    found.add(parent)
                    pending.append(parent)
        tests = sorted(x for x in found if x.startswith("tests/") and x != p)
        links = set(tests)
        stem = Path(p).stem.removeprefix("native_").removesuffix("_probe")
        for slug in {stem, stem.removesuffix("_native"), stem.removesuffix("_reference")}:
            for candidate in [f"tests/unit/{slug}.cpp", f"tests/probes/native_{slug}_probe.cpp",
                              f"tools/oracles/verify_original_{slug}.py",
                              f"reference/oracle-reports/{slug}.json"]:
                if candidate in path_set and candidate != p:
                    links.add(candidate)
        for t in tests:
            if t.startswith("tests/probes/native_"):
                slug = Path(t).stem.removeprefix("native_").removesuffix("_probe")
                for candidate in [f"tools/oracles/verify_original_{slug}.py",
                                  f"reference/oracle-reports/{slug}.json"]:
                    if candidate in path_set:
                        links.add(candidate)
        return sorted(links)

    records, stubs = [], []
    for p in maintained:
        body = texts[p]
        lines = body.splitlines()
        declarations = []
        owner_candidates = []
        for number, line in enumerate(lines, 1):
            match = PROVENANCE.search(line)
            if match:
                declarations.append({"line": number, "kind": match[1], "text": match[2]})
            # Merely a navigation index, not a C++ parser or ownership inference.
            for match in re.finditer(r'\b(?:struct|class)\s+([A-Za-z_]\w*)', line):
                owner_candidates.append({"line": number, "name": match[1],
                                         "status": "type_candidate_not_verified_owner"})
            if p.startswith("src/") and re.search(r'\bSTUB\b', line):
                annotation = re.search(r'//\s*STUB:\s*(.*)', line)
                following = []
                if annotation:
                    for next_line in lines[number:]:
                        if next_line.strip():
                            following.append(next_line.strip())
                            if ";" in next_line or "{" in next_line or len(following) >= 8:
                                break
                stubs.append({"id": f"{p}:{number}", "path": p, "line": number,
                              "module": module(p), "system": system_for(p),
                              "kind": "annotation_declaration_candidate" if annotation else "other_stub_mention",
                              "annotation": annotation[1] if annotation else line.strip(),
                              "following_source": following,
                              "review_status": "open_review",
                              "native_binding": "open_review_not_resolved_from_annotation",
                              "reference_binding": "open_review_not_resolved_from_annotation",
                              "acceptance": systems[system_for(p)]["acceptance"]})
        review = policy["files"].get(p, {})
        disposition = review.get("disposition", "open_review")
        role = ("reference_composition_or_adapter" if p.startswith(("src/guest/", "src/hle/"))
                else "product_or_development_source" if p.startswith("src/")
                else "test_or_judge" if p.startswith("tests/") else "tooling")
        if p.startswith("reference/"):
            role = ("historical_script_preserve_pending_usage_review" if "/production-integration-" in p
                    else "repository_owned_reference_script")
        system = system_for(p)
        semantic = reviewed_scope(review, hashlib.sha256((ROOT / p).read_bytes()).hexdigest())
        records.append({"path": p, "module": module(p), "system": system,
                        "sha256": hashlib.sha256((ROOT / p).read_bytes()).hexdigest(),
                        "present_at_base": p in base_paths,
                        "source_kind": "source" if Path(p).suffix in SOURCE_SUFFIXES else "support_or_data",
                        "role": role, "disposition": disposition,
                        "review_status": ("open_review" if disposition == "open_review" else
                                          semantic["status"] if disposition == "already_conforming" else
                                          "finding_recorded_closure_open"),
                        "semantic_review": semantic,
                        "reason": review.get("reason", "Not semantically reviewed; path, include and declaration indexing only."),
                        "evidence": review.get("evidence", []),
                        "native_build": ("listed_translation_unit" if p in native else
                                         "not_listed_translation_unit" if p.startswith("src/") and Path(p).suffix in {".cpp", ".mm", ".c"}
                                         else "not_a_translation_unit_in_native_list"),
                        "production_recipe_membership": (
                            "native_source_list" if p in native else
                            "native_source_wildcard" if p in native_wildcard else
                            "native_platform_object" if p in platform_paths else
                            "production_entrypoint" if p == "src/apps/sadx_native.mm" else
                            "not_in_observed_production_recipes" if p.startswith("src/") and Path(p).suffix in {".cpp", ".mm", ".c"} else
                            "not_a_production_translation_unit"),
                        "native_build_evidence": ["native-sources.txt", "Makefile:40", "Makefile:42", "Makefile:112"],
                        "canonical_ownership": {"status": "reviewed" if disposition == "already_conforming" else "open_review", "declaration_candidates": owner_candidates,
                                                "resolved": semantic["resolved_owners"],
                                                "reviewed_notes": review.get("ownership_notes", []),
                                                "annotated_globals": [d for d in declarations if d["kind"] == "GLOBAL"]},
                        "consumers": [{"path": path, "line": line, "kind": kind} for path, line, kind in
                                      sorted({(v["path"], v["line"], v["kind"]) for v in uses[p]})],
                        "consumer_status": "reviewed_with_recorded_limits" if disposition == "already_conforming" else "static_candidates_only_dynamic_and_link_consumers_open",
                        "resolved_consumers": semantic["resolved_consumers"],
                        "destination": review.get("destination", systems[system]["destination"]),
                        "judge_candidates": judge_candidates(p),
                        "judge_status": "reviewed_with_recorded_limits" if disposition == "already_conforming" else "candidate_links_only_coverage_and_case_verdict_review_open",
                        "accepted_judges": semantic["accepted_judges"],
                        "acceptance": systems[system]["acceptance"],
                        "provenance": declarations})
    # All other tracked reference paths and third-party trees get explicit rows.
    boundaries = []
    for p in paths:
        if p in maintained or not p.startswith(("reference/", "third_party/")):
            continue
        if index[p]["git_mode"] == "160000":
            kind, reason = "third_party_gitlink", "Pinned submodule; descendants belong to upstream, not the parent tracked denominator."
        elif p.startswith("third_party/") or p in policy["external_reference_sources"]:
            kind, reason = "third_party", "Preserve upstream license/provenance; no campaign rewrite without separate evidence."
        else:
            kind, reason = "reference_evidence_or_metadata", "Preserve evidence/control data; not executable maintained source. Historical versus live use remains open."
        boundaries.append({"path": p, **index[p], "boundary": kind, "reason": reason,
                           "review_status": "boundary_only_no_obsolescence_claim"})
    modules = []
    module_names = {r["module"] for r in records}
    module_reviews = policy.get("modules", {})
    if not isinstance(module_reviews, dict) or set(module_reviews) - module_names:
        raise ValueError("Unknown/stale module review identity")
    for name in sorted(module_names):
        members = [r for r in records if r["module"] == name]
        semantic = reviewed_module(module_reviews.get(name, {}), members)
        validate_module_evidence(semantic, path_set)
        modules.append({"module": name, "system": sorted({r["system"] for r in members}),
                        "files": [r["path"] for r in members],
                        "dispositions": dict(collections.Counter(r["disposition"] for r in members)),
                        "disposition": semantic["disposition"],
                        "semantic_review": semantic,
                        "canonical_owners_status": ("reviewed_with_recorded_limits" if semantic["status"] == "reviewed_conforming"
                                                    else "open_review_see_per_file_candidates"),
                        "judge_candidates": sorted({j for r in members for j in r["judge_candidates"]}),
                        "acceptance": sorted({a for r in members for a in r["acceptance"]}),
                        "destination": sorted({r["destination"] for r in members})})
    by_path = {r["path"]: r for r in records}
    for values in (records, stubs):
        for r in values:
            for link in r["acceptance"]:
                if link.split("#")[0] not in path_set:
                    raise ValueError("Missing acceptance link: " + link)
    for stub in stubs:
        stub["canonical_owner_status"] = "open_review"
        stub["judge_candidates"] = by_path[stub["path"]]["judge_candidates"]
        stub["consumers"] = by_path[stub["path"]]["consumers"]
    total = set(maintained) | {b["path"] for b in boundaries}
    expected = {p for p in paths if p.startswith(("src/", "tools/", "tests/", "reference/", "third_party/"))}
    if total != expected:
        raise ValueError("Coverage mismatch: " + repr(sorted(total ^ expected)))
    removed = sorted(p for p in base_paths - path_set
                     if p.startswith(("src/", "tools/", "tests/", "reference/", "third_party/")))
    if removed:
        raise ValueError("Baseline paths removed; add an explicit reviewed lineage policy first: " + repr(removed))
    summary = {"schema_version": 1, "base_commit": base,
               "tracked_scope": len(total), "maintained_and_script_entries": len(records),
               "modules": len(modules), "boundary_entries": len(boundaries),
               "module_dispositions": dict(sorted(collections.Counter(m["disposition"] for m in modules).items())),
               "roots": dict(collections.Counter(p.split('/')[0] for p in sorted(total))),
               "dispositions": dict(sorted(collections.Counter(r["disposition"] for r in records).items())),
               "systems": dict(sorted(collections.Counter(r["system"] for r in records).items())),
               "native_translation_units_listed": sum(r["native_build"] == "listed_translation_unit" for r in records),
               "native_translation_units_not_listed": sum(r["native_build"] == "not_listed_translation_unit" for r in records),
               "production_recipe_membership": dict(sorted(collections.Counter(r["production_recipe_membership"] for r in records if r["path"].startswith("src/")).items())),
               "stub_annotations": sum(s["kind"] == "annotation_declaration_candidate" for s in stubs),
               "other_stub_mentions": sum(s["kind"] == "other_stub_mention" for s in stubs),
               "coverage": "exact tracked path equality; no semantic completeness or behavior verdict",
               "checks_not_run": ["builds", "gate", "oracles", "mutants", "journeys", "providers"],
               "baseline_removed_paths": removed}

    def jsonl(rows):
        return "".join(json.dumps(r, sort_keys=True, ensure_ascii=False) + "\n" for r in rows)

    outputs = {"modules.jsonl": jsonl(modules), "boundaries.jsonl": jsonl(boundaries),
               "stubs.jsonl": jsonl(stubs),
               "summary.json": json.dumps(summary, indent=2, sort_keys=True) + "\n"}
    shards = collections.defaultdict(list)
    for record in records:
        parts = record["path"].split("/")
        shard = "-".join(parts[:2]) if parts[0] != "reference" else "reference-scripts"
        shards[shard].append(record)
    outputs.update({"files/" + name + ".jsonl": jsonl(rows) for name, rows in shards.items()})
    if any(len(value.encode()) > 2 * 1024 * 1024 for value in outputs.values()):
        raise ValueError("Inventory shard exceeds the repository 2 MiB limit")
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="regenerate inventory artifacts only")
    mode.add_argument("--check", action="store_true", help="compare artifacts with tracked local scope")
    args = parser.parse_args()
    outputs = render()
    existing_shards = {p.relative_to(OUT).as_posix() for p in (OUT / "files").glob("*.jsonl")}
    expected_shards = {name for name in outputs if name.startswith("files/")}
    if existing_shards - expected_shards:
        raise ValueError("Unexpected inventory shards require explicit reconciliation: " +
                         repr(sorted(existing_shards - expected_shards)))
    stale = []
    for name, value in outputs.items():
        target = OUT / name
        if args.write:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(value)
        elif not target.exists() or target.read_text() != value:
            stale.append(name)
    if stale:
        print("Inventory drift: " + ", ".join(stale), file=sys.stderr)
        return 1
    print(outputs["summary.json"], end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
