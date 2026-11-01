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



def partition_jsonl(rows, byte_limit):
    """Keep every complete record in order while bounding UTF-8 artifact bytes."""
    if byte_limit <= 0:
        raise ValueError("Inventory shard byte limit must be positive")
    chunks, lines, size = [], [], 0
    for row in rows:
        line = json.dumps(row, sort_keys=True, ensure_ascii=False) + "\n"
        length = len(line.encode())
        if length > byte_limit:
            raise ValueError("One inventory record exceeds the shard byte limit")
        if lines and size + length > byte_limit:
            chunks.append("".join(lines)); lines, size = [], 0
        lines.append(line); size += length
    if lines or not chunks:
        chunks.append("".join(lines))
    return chunks


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



def reviewed_stub_binding(stub, review, paths):
    """Record reviewed bindings without treating an unrecovered callee as implemented."""
    if stub["kind"] != "annotation_declaration_candidate":
        raise ValueError("Binding review requires a STUB annotation")
    if (review.get("annotation") != stub["annotation"] or
            review.get("declaration") != stub["following_source"] or
            review.get("source_sha256") != hashlib.sha256((ROOT / stub["path"]).read_bytes()).hexdigest()):
        raise ValueError("STUB declaration/source changed; fresh binding review required")
    required = ("canonical_owner", "native_binding", "reference_binding", "accepted_coverage", "limits")
    if not all(review.get(field) for field in required):
        raise ValueError("STUB binding review requires owner, both bindings, coverage and limits")
    for field in ("canonical_owner", "native_binding", "reference_binding"):
        section = review[field]
        if not isinstance(section, dict) or not section.get("evidence"):
            raise ValueError("STUB binding section requires fingerprinted evidence: " + field)
        evidence = section["evidence"]
        if not isinstance(evidence, list) or not all(
                isinstance(item, dict) and item.get("path") and item.get("sha256") for item in evidence):
            raise ValueError("STUB binding section requires fingerprinted evidence: " + field)
    for section, fields in (("canonical_owner", ("resolution",)),
                            ("native_binding", ("kind", "method")),
                            ("reference_binding", ("kind", "resolution"))):
        for field in fields:
            value = review[section].get(field)
            if not isinstance(value, str) or not value.strip():
                raise ValueError("STUB binding requires explicit semantic description: " + section + "." + field)
    if review["native_binding"]["kind"] == "explicit_failure":
        failure = review["native_binding"].get("failure")
        if not isinstance(failure, str) or not failure.strip():
            raise ValueError("STUB explicit failure requires its failure identifier")
    coverage = review["accepted_coverage"]
    if not isinstance(coverage, list) or not all(
            isinstance(item, dict) and item.get("path") and item.get("sha256") for item in coverage):
        raise ValueError("STUB coverage requires fingerprinted records")
    limits = review["limits"]
    if not isinstance(limits, list) or not all(isinstance(item, str) and item.strip() for item in limits):
        raise ValueError("STUB binding requires explicit nonblank limits")
    if review.get("behavior_status") != "unrecovered_open_debt":
        raise ValueError("Binding review cannot retire unrecovered STUB behavior")

    def validate(value):
        if isinstance(value, dict):
            if "path" in value:
                name = value["path"]
                if name not in paths or not (ROOT / name).is_file():
                    raise ValueError("Missing STUB binding evidence: " + str(name))
                if value.get("sha256") != hashlib.sha256((ROOT / name).read_bytes()).hexdigest():
                    raise ValueError("Changed STUB binding evidence: " + str(name))
            for child in value.values():
                validate(child)
        elif isinstance(value, list):
            for child in value:
                validate(child)

    validate(review)
    return {field: review[field] for field in required} | {
        "behavior_status": "unrecovered_open_debt",
        "source_sha256": review["source_sha256"],
        "status": "bindings_reviewed_behavior_unrecovered"}



def unique_review_json(text):
    """Reject conflicting review identities before JSON can discard their first value."""
    def object_from_pairs(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError("Duplicate STUB review JSON key: " + key)
            result[key] = value
        return result
    return json.loads(text, object_pairs_hook=object_from_pairs)


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


SCOPE_ROOTS = ("src/", "tools/", "tests/", "reference/", "third_party/")


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def canonical_hash(value):
    return sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode())


def strict_keys(value, keys, label):
    if not isinstance(value, dict) or set(value) != set(keys):
        raise ValueError("Invalid " + label + " fields")


def nonblank(value):
    return isinstance(value, str) and bool(value.strip())


def safe_path(name):
    if (not isinstance(name, str) or not name or Path(name).is_absolute() or
            Path(name).as_posix() != name or any(x in {"..", ".git"} for x in Path(name).parts)):
        raise ValueError("Invalid lineage path")
    return name


def fingerprinted_file(anchor, paths):
    strict_keys(anchor, ("path", "sha256"), "lineage anchor")
    name = safe_path(anchor["path"])
    target = ROOT / name
    if (name not in paths or not target.is_file() or
            any(p.is_symlink() for p in [target, *target.parents] if p.is_relative_to(ROOT))):
        raise ValueError("Missing/untracked lineage evidence: " + name)
    if target.stat().st_size > 2 * 1024 * 1024:
        raise ValueError("Lineage evidence exceeds 2 MiB")
    data = target.read_bytes()
    if anchor["sha256"] != sha256(data):
        raise ValueError("Stale lineage evidence: " + name)
    return data


def git_blob_hashes(objects):
    """One bounded Git metadata stream rather than a subprocess per scope member."""
    process = subprocess.run(["git", "-C", str(ROOT), "cat-file", "--batch"],
                             input=("\n".join(objects.values()) + "\n").encode(),
                             stdout=subprocess.PIPE, check=True)
    data, offset, result = process.stdout, 0, {}
    for name in objects:
        end = data.index(b"\n", offset)
        header = data[offset:end].split()
        if len(header) != 3 or header[1] != b"blob":
            raise ValueError("Missing immutable source object: " + name)
        size = int(header[2])
        start = end + 1
        result[name] = sha256(data[start:start + size])
        offset = start + size + 1
    return result


def baseline_fingerprints(commit):
    objects, actual = {}, {}
    for row in git("ls-tree", "-r", "-z", commit).split("\0"):
        if not row:
            continue
        meta, name = row.split("\t", 1)
        mode, kind, oid = meta.split()
        if name.startswith(SCOPE_ROOTS):
            if mode == "160000":
                actual[name] = "gitlink:" + oid
            else:
                objects[name] = oid
    actual.update(git_blob_hashes(objects))
    return actual


def lineage_anchors(value):
    """Only explicit fingerprint anchors; source snapshot paths are not live proofs."""
    result = {}
    def visit(item):
        if isinstance(item, dict):
            if set(item) == {"path", "sha256"}:
                name = safe_path(item["path"])
                if name in result and result[name] != item["sha256"]:
                    raise ValueError("Conflicting lineage proof fingerprints")
                result[name] = item["sha256"]
            for child in item.values():
                visit(child)
        elif isinstance(item, list):
            for child in item:
                visit(child)
    visit(value)
    return result


def frozen_lineage_proofs(ledger, receipt, paths, has_git):
    expected = lineage_anchors(ledger)
    if receipt["proofs"] != expected:
        raise ValueError("Incomplete/changed committed lineage proof set")
    for name, digest in expected.items():
        fingerprinted_file({"path": name, "sha256": digest}, paths)
    if has_git:
        modes = {}
        for row in git("ls-tree", "-r", "-z", receipt["freeze_commit"]).split("\0"):
            if row:
                metadata, name = row.split("\t", 1)
                modes[name] = metadata.split()[0]
        if any(modes.get(name) not in {"100644", "100755"} for name in expected):
            raise ValueError("Lineage proof must be regular in committed freeze snapshot")
        if git_blob_hashes({p: receipt["freeze_commit"] + ":" + p for p in expected}) != expected:
            raise ValueError("Lineage proof differs from committed freeze snapshot")


def previous_receipt_at(name, commit):
    """Find the last committed version even if a receipt was subsequently deleted."""
    changes = git("log", "--first-parent", "--format=%H", commit, "--", name).splitlines()
    if len(changes) > 64:
        raise ValueError("Lineage receipt history exceeds 64 versions")
    for changed in changes:
        result = subprocess.run(["git", "-C", str(ROOT), "show", changed + ":" + name],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode == 0:
            return result.stdout, changed
    return None


def committed_receipt_continuity(name, receipt, paths):
    """A restored old tip cannot hide a later committed receipt generation."""
    chain = set()
    current = receipt
    for _ in range(65):
        chain.add(canonical_hash(current))
        previous = current["previous"]
        if previous is None:
            break
        current = unique_review_json(fingerprinted_file(previous["receipt"], paths).decode())
    else:
        raise ValueError("Lineage proof history exceeds 64 receipts")
    versions = git("log", "--first-parent", "--format=%H", "HEAD", "--", name).splitlines()
    if len(versions) > 64:
        raise ValueError("Lineage receipt history exceeds 64 versions")
    for commit in versions:
        result = subprocess.run(["git", "-C", str(ROOT), "show", commit + ":" + name],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode:  # A deletion does not erase the prior generation.
            continue
        historical = unique_review_json(result.stdout.decode())
        if canonical_hash(historical) not in chain:
            raise ValueError("Committed lineage receipt history cannot be rolled back or skipped")


def immutable_receipt_identity(name, commit):
    policy_path = (OUT / "review.json").relative_to(ROOT).as_posix()
    versions = git("log", "--first-parent", "--format=%H", commit, "--", policy_path).splitlines()
    if len(versions) > 64:
        raise ValueError("Lineage policy history exceeds 64 versions")
    checked = set()
    for version in versions:
        old = subprocess.run(["git", "-C", str(ROOT), "show", version + ":" + policy_path],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if old.returncode:
            continue
        old_name = unique_review_json(old.stdout.decode()).get("source_lineage_receipt")
        if old_name and old_name != name and old_name not in checked:
            safe_path(old_name)
            checked.add(old_name)
            if previous_receipt_at(old_name, commit):
                raise ValueError("Accepted lineage receipt path cannot be replaced")


def integrated_replacement(snapshot, frozen, paths, has_git):
    """Bind a historical snapshot to the integrated tree, never to today's bytes."""
    effective = snapshot.get("integrated", snapshot)
    if "integrated" in effective:
        raise ValueError("Nested source integration anchors are unsupported")
    if (effective["path"] != snapshot["path"] or effective["sha256"] != snapshot["sha256"]):
        raise ValueError("Source integration import is not content-equivalent")
    lineage_snapshot(effective, paths, has_git)
    if has_git:
        if effective["commit"] not in set(git("rev-list", "--first-parent", frozen).splitlines()):
            raise ValueError("Replacement source snapshot was never integrated on the first-parent source history")
    return effective["commit"]


def prepare_lineage_receipt(name, ledger_name):
    """Seal only committed inputs. Activation requires committing this receipt too."""
    safe_path(name)
    safe_path(ledger_name)
    if not git_available():
        raise ValueError("Receipt preparation requires Git; archives only verify supplied proofs")
    if git("status", "--porcelain", "--untracked-files=all", "--", "src/").strip():
        raise ValueError("Receipt preparation requires source identical to immutable HEAD")
    policy_name = (OUT / "review.json").relative_to(ROOT).as_posix()
    for path in (policy_name, ledger_name):
        if git("show", "HEAD:" + path).encode() != (ROOT / path).read_bytes():
            raise ValueError("Receipt preparation requires committed policy: " + path)
    policy = unique_review_json(read(policy_name))
    if policy.get("source_lineage_receipt") != name:
        raise ValueError("Receipt path must be explicitly authored in review policy")
    paths = set(filter(None, git("ls-files", "-z").split("\0")))
    source = {p: "HEAD:" + p for p in sorted(paths) if p.startswith("src/")}
    ledger = unique_review_json(read(ledger_name))
    receipt = dict(schema_version=3, base_commit=policy["base_commit"],
                   freeze_commit=git("rev-parse", "HEAD").strip(),
                   policy={"path": ledger_name, "sha256": sha256((ROOT / ledger_name).read_bytes())},
                   baseline_members=baseline_fingerprints(policy["base_commit"]),
                   archive_paths=sorted(paths | {name}), source_freeze=git_blob_hashes(source),
                   proofs=lineage_anchors(ledger), previous=ledger["previous"])
    reconcile_lineage(policy, set(receipt["baseline_members"]), paths | {name},
                      _receipt=receipt, _preparing=True)
    if len(json.dumps(receipt, sort_keys=True, indent=2).encode()) > 2 * 1024 * 1024:
        raise ValueError("Lineage receipt exceeds 2 MiB")
    target = ROOT / name
    if (any(p.is_symlink() for p in [target, *target.parents] if p.is_relative_to(ROOT)) or
            not name.startswith("docs/refactor-inventory/")):
        raise ValueError("Receipt output must be a regular inventory document")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(receipt, sort_keys=True, indent=2) + "\n")
    return receipt


def git_available():
    # A source archive nested inside another repository is still an archive.
    return (ROOT / ".git").exists()


def lineage_snapshot(snapshot, paths, has_git, *, allow_integration=False):
    strict_keys(snapshot, {"path", "sha256", "commit", "proof"} |
                ({"integrated"} if allow_integration and "integrated" in snapshot else set()), "lineage snapshot")
    safe_path(snapshot["path"])
    if not re.fullmatch(r"[0-9a-f]{40}", snapshot["commit"]):
        raise ValueError("Snapshot requires immutable commit")
    data = fingerprinted_file(snapshot["proof"], paths)
    if sha256(data) != snapshot["sha256"]:
        raise ValueError("Wrong source snapshot fingerprint")
    if has_git:
        original = subprocess.check_output(["git", "-C", str(ROOT), "show",
                                            snapshot["commit"] + ":" + snapshot["path"]],
                                           stderr=subprocess.PIPE)
        if original != data:
            raise ValueError("Snapshot does not match immutable Git source")
    return snapshot["path"]


def reconcile_lineage(policy, base_paths, paths, *, _receipt=None, _ledger=None, _historical=False,
                      _preparing=False, _depth=0):
    """Whole-file historical accounting only; never grants a live source exception."""
    if _depth > 64:
        raise ValueError("Lineage proof history exceeds 64 receipts")
    removed = {p for p in base_paths - paths if p.startswith(SCOPE_ROOTS)}
    receipt_name = policy.get("source_lineage_receipt")
    if not receipt_name:
        if removed:
            raise ValueError("Baseline paths removed; add an explicit reviewed lineage policy first: " + repr(sorted(removed)))
        return [], None
    receipt_name = safe_path(receipt_name)
    if receipt_name not in paths:
        raise ValueError("Untracked lineage migration receipt")
    if _receipt is None and (ROOT / receipt_name).stat().st_size > 2 * 1024 * 1024:
        raise ValueError("Lineage receipt exceeds 2 MiB")
    receipt = _receipt if _receipt is not None else unique_review_json(read(receipt_name))
    if len(json.dumps(receipt, sort_keys=True, indent=2).encode()) > 2 * 1024 * 1024:
        raise ValueError("Lineage receipt exceeds 2 MiB")
    strict_keys(receipt, ("schema_version", "base_commit", "freeze_commit", "policy",
                          "baseline_members", "archive_paths", "source_freeze", "proofs", "previous"), "lineage receipt")
    if receipt["schema_version"] != 3 or receipt["base_commit"] != policy["base_commit"]:
        raise ValueError("Unknown lineage metric version/baseline")
    archive_paths = receipt["archive_paths"]
    if (not isinstance(archive_paths, list) or len(set(archive_paths)) != len(archive_paths) or
            any(safe_path(p) != p for p in archive_paths)):
        raise ValueError("Duplicate/invalid archive membership")
    if (not isinstance(receipt["source_freeze"], dict) or
            not all(p.startswith("src/") and safe_path(p) == p and
                    isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value)
                    for p, value in receipt["source_freeze"].items())):
        raise ValueError("Invalid source freeze fingerprints")
    has_git = git_available()
    frozen = receipt["freeze_commit"]
    if not isinstance(frozen, str) or not re.fullmatch(r"[0-9a-f]{40}", frozen):
        raise ValueError("Receipt requires immutable freeze commit")
    proof_paths = paths if has_git else paths & set(archive_paths)
    document = _ledger if _ledger is not None else fingerprinted_file(receipt["policy"], proof_paths)
    if sha256(document) != receipt["policy"]["sha256"]:
        raise ValueError("Changed historical lineage ledger proof")
    membership = set(archive_paths) if _historical else paths
    removed = {p for p in base_paths - membership if p.startswith(SCOPE_ROOTS)}
    members = receipt["baseline_members"]
    if not isinstance(members, dict) or set(members) != {p for p in base_paths if p.startswith(SCOPE_ROOTS)}:
        raise ValueError("Lineage baseline membership mismatch")
    if not all(safe_path(p) == p and isinstance(value, str) and
               re.fullmatch(r"(?:[0-9a-f]{64}|gitlink:[0-9a-f]{40})", value)
               for p, value in members.items()):
        raise ValueError("Invalid baseline fingerprints")
    if has_git:
        if not (_historical or _preparing) and git("show", "HEAD:" + receipt_name).encode() != (ROOT / receipt_name).read_bytes():
            raise ValueError("Migration receipt must be versioned in immutable HEAD")
        subprocess.check_call(["git", "-C", str(ROOT), "merge-base", "--is-ancestor", frozen, "HEAD"])
        if git("show", frozen + ":" + receipt["policy"]["path"]).encode() != document:
            raise ValueError("Lineage policy is not the frozen Git policy")
        # Compare the entire original denominator, not just the retired member.
        actual = baseline_fingerprints(policy["base_commit"])
        if members != actual:
            raise ValueError("Wrong immutable baseline fingerprints")
        frozen_paths = git("ls-tree", "-r", "--name-only", "-z", frozen).split("\0")
        source_objects = {p: frozen + ":" + p for p in frozen_paths if p.startswith("src/")}
        source_freeze = git_blob_hashes(source_objects)
        if source_freeze != receipt["source_freeze"]:
            raise ValueError("Wrong immutable source freeze")
    elif not _historical:
        scoped = lambda values: {p for p in values if p.startswith(SCOPE_ROOTS)}
        if scoped(receipt["archive_paths"]) != scoped(paths):
            raise ValueError("Archive membership differs from supplied lineage receipt")
    ledger = unique_review_json(document.decode())
    strict_keys(ledger, ("schema_version", "edges", "previous"), "lineage ledger")
    if ledger["schema_version"] != 2 or not isinstance(ledger["edges"], list):
        raise ValueError("Unknown lineage ledger schema")
    if ledger["previous"] != receipt["previous"]:
        raise ValueError("Receipt/ledger predecessor mismatch")
    frozen_lineage_proofs(ledger, receipt, proof_paths, has_git)
    edges, targets = {}, set()
    for edge in ledger["edges"]:
        strict_keys(edge, ("before", "replacements", "member", "disposition", "rationale",
                           "consumer_evidence", "behavior_evidence", "review", "prior_member"), "lineage edge")
        old = lineage_snapshot(edge["before"], proof_paths, has_git)
        if old in edges or old in membership or not old.startswith(SCOPE_ROOTS):
            raise ValueError("Duplicate/stale lineage source")
        if edge["disposition"] not in {"reviewed_migration", "reviewed_retirement"} or not nonblank(edge["rationale"]):
            raise ValueError("Lineage requires explicit semantic disposition/rationale")
        member = edge["member"]
        member_keys = {"module", "prior_disposition", "remaining_debt", "observed_counts"}
        strict_keys(member, member_keys | ({"baseline_snapshot"} if "baseline_snapshot" in member else set()),
                    "historical member")
        if "baseline_snapshot" in member:
            if lineage_snapshot(member["baseline_snapshot"], proof_paths, has_git) != old or old not in removed:
                raise ValueError("Unknown historical baseline snapshot")
        if (member["module"] != module(old) or member["prior_disposition"] not in DISPOSITIONS or
                not isinstance(member["remaining_debt"], list) or not member["remaining_debt"] or
                not all(nonblank(x) for x in member["remaining_debt"]) or
                not isinstance(member["observed_counts"], dict) or
                not all(isinstance(n, int) and not isinstance(n, bool) and n >= 0
                        for n in member["observed_counts"].values())):
            raise ValueError("Historical member must retain identity, disposition and open debt")
        for field in ("consumer_evidence", "behavior_evidence"):
            if not isinstance(edge[field], list) or not edge[field]:
                raise ValueError("Missing lineage consumer/behavior preservation evidence")
            for anchor in edge[field]:
                if not fingerprinted_file(anchor, proof_paths).strip():
                    raise ValueError("Empty lineage consumer/behavior evidence")
        review = unique_review_json(fingerprinted_file(edge["review"], proof_paths).decode())
        strict_keys(review, ("edge_sha256", "reviewer", "independent", "verdict", "limits"), "independent lineage review")
        if (review["edge_sha256"] != canonical_hash({k: v for k, v in edge.items() if k != "review"}) or
                not nonblank(review["reviewer"]) or review["independent"] is not True or
                review["verdict"] != "accept" or not nonblank(review["limits"])):
            raise ValueError("Missing/stale independent lineage review")
        successors = edge["replacements"]
        if not isinstance(successors, list) or not successors:
            raise ValueError("Lineage requires real replacement paths")
        for successor in successors:
            name = lineage_snapshot(successor, proof_paths, has_git, allow_integration=True)
            if name in targets or not name.startswith(SCOPE_ROOTS):
                raise ValueError("Duplicate/overlapping lineage replacement")
            integrated = integrated_replacement(successor, frozen, proof_paths, has_git)
            if has_git:
                subprocess.check_call(["git", "-C", str(ROOT), "merge-base", "--is-ancestor",
                                       edge["before"]["commit"], integrated])
            targets.add(name)
        edges[old] = edge
    visited, active = set(), set()

    def visit(name):
        if name in active:
            raise ValueError("Cyclic lineage")
        if name in visited:
            return
        if name not in edges:
            target = ROOT / name
            if (name not in membership or (not _historical and (not target.is_file() or
                    any(p.is_symlink() for p in [target, *target.parents] if p.is_relative_to(ROOT))))):
                raise ValueError("Missing/untracked lineage successor: " + name)
            return
        active.add(name)
        for successor in edges[name]["replacements"]:
            child = successor["path"]
            if has_git and child in edges:
                subprocess.check_call(["git", "-C", str(ROOT), "merge-base", "--is-ancestor",
                                       successor.get("integrated", successor)["commit"], edges[child]["before"]["commit"]])
            visit(child)
        active.remove(name)
        visited.add(name)

    for name in removed:
        if name not in edges:
            raise ValueError("Unexplained baseline omission: " + name)
        baseline = edges[name]["member"].get("baseline_snapshot", edges[name]["before"])
        if baseline["commit"] != policy["base_commit"] or baseline["sha256"] != members[name]:
            raise ValueError("Unexplained baseline omission/fingerprint: " + name)
        visit(name)
    if set(edges) != visited:
        raise ValueError("Unknown/unreachable lineage records")
    for name, edge in edges.items():
        origin = edge["prior_member"]
        lineage_snapshot(origin, proof_paths, has_git)
        prior = unique_review_json(fingerprinted_file(origin["proof"], proof_paths).decode())
        expected = {"historical_path": name, "source_sha256":
                    edge["member"].get("baseline_snapshot", edge["before"])["sha256"], "member": edge["member"]}
        if prior != expected:
            raise ValueError("Historical member differs from anchored prior review metadata")
        if has_git:
            if origin["commit"] == frozen:
                raise ValueError("Prior review metadata must precede the ledger freeze")
            subprocess.check_call(["git", "-C", str(ROOT), "merge-base", "--is-ancestor", origin["commit"], frozen])
    previous = receipt["previous"]
    if has_git:
        immutable_receipt_identity(receipt_name, frozen)
        expected_previous = previous_receipt_at(receipt_name, frozen)
        if bool(previous) != bool(expected_previous):
            raise ValueError("Missing prior accepted lineage receipt")
    if previous is not None:
        strict_keys(previous, ("receipt", "ledger", "commit", "path"), "lineage predecessor")
        if previous["path"] != receipt_name or not re.fullmatch(r"[0-9a-f]{40}", previous["commit"]):
            raise ValueError("Changed predecessor receipt identity")
        old_bytes = fingerprinted_file(previous["receipt"], proof_paths)
        old_ledger = fingerprinted_file(previous["ledger"], proof_paths)
        old_receipt = unique_review_json(old_bytes.decode())
        if has_git:
            if old_bytes != expected_previous[0] or git("show", previous["commit"] + ":" + receipt_name).encode() != old_bytes:
                raise ValueError("Predecessor is not the prior committed accepted receipt")
            subprocess.check_call(["git", "-C", str(ROOT), "merge-base", "--is-ancestor", previous["commit"], frozen])
        if sha256(old_ledger) != old_receipt["policy"]["sha256"]:
            raise ValueError("Changed historical lineage ledger proof")
        old_edges = unique_review_json(old_ledger.decode())["edges"]
        for old in old_edges:
            if edges.get(old["before"]["path"]) != old:
                raise ValueError("Prior accepted historical identity/debt cannot be removed or rewritten")
        reconcile_lineage(policy, base_paths, paths, _receipt=old_receipt, _ledger=old_ledger,
                          _historical=True, _depth=_depth + 1)
    if has_git and not _historical:
        committed_receipt_continuity(receipt_name, receipt, proof_paths)
    if not has_git and not (_historical or _preparing):
        export = unique_review_json((OUT / "summary.json").read_text())
        if export.get("lineage_receipt", {}).get("sha256") != sha256((ROOT / receipt_name).read_bytes()):
            raise ValueError("Archive receipt differs from supplied export fingerprint")
    return [{"historical_path": name, "baseline_member": name in removed,
             "source_sha256": edge["member"].get("baseline_snapshot", edge["before"])["sha256"],
             "source_lines": len(fingerprinted_file(
                 edge["member"].get("baseline_snapshot", edge["before"])["proof"], proof_paths).splitlines()), **edge,
             "status": "historical_scope_retained_debt_open"}
            for name, edge in sorted(edges.items())], receipt


def render():
    policy = unique_review_json((OUT / "review.json").read_text())
    base = policy["base_commit"]
    if git_available():
        paths = sorted(filter(None, git("ls-files", "-z").split("\0")))
        path_set = set(paths)
        base_paths = set(filter(None, git("ls-tree", "-r", "--name-only", "-z", base).split("\0")))
        index = {}
        for row in git("ls-files", "--stage", "-z").split("\0"):
            if row:
                meta, path = row.split("\t", 1)
                mode, blob, stage = meta.split()
                if stage != "0":
                    raise ValueError("Unmerged tracked path: " + path)
                index[path] = {"git_mode": mode, "index_blob": blob}
    else:
        # git archive supplies regular tracked files; caches/build products are not scope.
        paths = sorted(p.relative_to(ROOT).as_posix() for p in ROOT.rglob("*")
                       if p.is_file() and not any(x in {".git", "build", "__pycache__"}
                                                 for x in p.relative_to(ROOT).parts))
        path_set = set(paths)
        boundary_file = OUT / "boundaries.jsonl"
        index = {r["path"]: {k: r[k] for k in ("git_mode", "index_blob")}
                 for r in map(json.loads, boundary_file.read_text().splitlines())}
        gitlinks = {p for p, metadata in index.items() if metadata["git_mode"] == "160000"}
        paths = sorted(({p for p in paths if not any(p.startswith(link + "/") for link in gitlinks)} |
                        gitlinks))
        path_set = set(paths)
        if policy.get("source_lineage_receipt"):
            receipt_path = safe_path(policy["source_lineage_receipt"])
            if (ROOT / receipt_path).stat().st_size > 2 * 1024 * 1024:
                raise ValueError("Lineage receipt exceeds 2 MiB")
            receipt = unique_review_json(read(receipt_path))
            base_paths = set(receipt["baseline_members"])
        else:
            base_paths = set(index)
            for shard in (OUT / "files").glob("*.jsonl"):
                base_paths.update(r["path"] for r in map(json.loads, shard.read_text().splitlines())
                                  if r["present_at_base"])
        if any(p not in index for p in paths if p.startswith(("reference/", "third_party/"))
               and not (p.startswith("reference/") and Path(p).suffix in SOURCE_SUFFIXES
                        and p not in policy["external_reference_sources"])):
            raise ValueError("Archive boundary membership requires supplied inventory metadata")
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
    binding_reviews = {}
    for name in policy.get("stub_binding_review_files", []):
        if name not in path_set:
            raise ValueError("Missing STUB binding review file: " + str(name))
        fragment = unique_review_json(read(name))
        for identity, review in fragment["stubs"].items():
            if identity in binding_reviews:
                raise ValueError("Duplicate STUB binding review: " + identity)
            binding_reviews[identity] = review
    unknown = set(binding_reviews) - {stub["id"] for stub in stubs}
    if unknown:
        raise ValueError("Stale STUB binding review: " + repr(sorted(unknown)))
    for stub in stubs:
        stub["canonical_owner_status"] = "open_review"
        stub["behavior_status"] = "unrecovered_open_debt"
        if stub["id"] in binding_reviews:
            semantic = reviewed_stub_binding(stub, binding_reviews[stub["id"]], path_set)
            stub["binding_review"] = semantic
            stub["review_status"] = semantic["status"]
            stub["canonical_owner_status"] = "binding_owner_reviewed_behavior_unrecovered"
            stub["native_binding"] = semantic["native_binding"]
            stub["reference_binding"] = semantic["reference_binding"]
        stub["judge_candidates"] = by_path[stub["path"]]["judge_candidates"]
        stub["consumers"] = by_path[stub["path"]]["consumers"]
    total = set(maintained) | {b["path"] for b in boundaries}
    expected = {p for p in paths if p.startswith(("src/", "tools/", "tests/", "reference/", "third_party/"))}
    if total != expected:
        raise ValueError("Coverage mismatch: " + repr(sorted(total ^ expected)))
    removed = sorted(p for p in base_paths - path_set
                     if p.startswith(("src/", "tools/", "tests/", "reference/", "third_party/")))
    history, receipt = reconcile_lineage(policy, base_paths, path_set)
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
               "reviewed_stub_bindings": len(binding_reviews),
               "coverage": "exact tracked path equality; no semantic completeness or behavior verdict",
               "checks_not_run": ["builds", "gate", "oracles", "mutants", "journeys", "providers"],
               "baseline_removed_paths": removed}

    if receipt:
        summary.update(schema_version=3, current_tracked_denominator=len(total),
                       historical_retained_scope=len(history),
                       historical_baseline_members=sum(r["baseline_member"] for r in history),
                       accounted_scope=len(total) + len(history),
                       historical_open_debt=len(history),
                       current_open_entries=sum(r["disposition"] != "already_conforming" for r in records),
                       accounted_open_entries=sum(r["disposition"] != "already_conforming" for r in records) + len(history),
                       lineage_receipt={"path": policy["source_lineage_receipt"],
                                        "sha256": sha256((ROOT / policy["source_lineage_receipt"]).read_bytes()),
                                        "freeze_commit": receipt["freeze_commit"], "policy": receipt["policy"]})

    def jsonl(rows):
        return "".join(json.dumps(r, sort_keys=True, ensure_ascii=False) + "\n" for r in rows)

    module_chunks = partition_jsonl(modules, 2 * 1024 * 1024)
    module_outputs = {("modules.jsonl" if index == 0 else f"modules-{index + 1:03d}.jsonl"): content
                      for index, content in enumerate(module_chunks)}
    summary["module_shards"] = [{"path": name, "records": content.count("\n"),
                                  "sha256": hashlib.sha256(content.encode()).hexdigest()}
                                 for name, content in module_outputs.items()]
    outputs = {**module_outputs, "boundaries.jsonl": jsonl(boundaries),
               "stubs.jsonl": jsonl(stubs),
               "summary.json": json.dumps(summary, indent=2, sort_keys=True) + "\n"}
    if receipt:
        for n, content in enumerate(partition_jsonl(history, 2 * 1024 * 1024), 1):
            outputs[f"lineage-{n:03d}.jsonl"] = content
        summary["lineage_shards"] = [{"path": name, "records": content.count("\n"),
                                      "sha256": sha256(content.encode())}
                                     for name, content in outputs.items() if name.startswith("lineage-")]
        outputs["summary.json"] = json.dumps(summary, indent=2, sort_keys=True) + "\n"
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
    mode.add_argument("--prepare-lineage-receipt", metavar="PATH", help="write an unactivated receipt from immutable HEAD")
    parser.add_argument("--lineage-policy", metavar="PATH", help="committed reviewed lineage ledger to seal")
    args = parser.parse_args()
    if args.prepare_lineage_receipt:
        if not args.lineage_policy:
            parser.error("--prepare-lineage-receipt requires --lineage-policy")
        prepare_lineage_receipt(args.prepare_lineage_receipt, args.lineage_policy)
        print("Receipt prepared; commit and independently review before inventory activation.")
        return 0
    if args.lineage_policy:
        parser.error("--lineage-policy requires --prepare-lineage-receipt")
    outputs = render()
    existing_shards = {p.relative_to(OUT).as_posix() for p in (OUT / "files").glob("*.jsonl")}
    existing_shards.update(p.relative_to(OUT).as_posix() for p in OUT.glob("modules*.jsonl"))
    existing_shards.update(p.relative_to(OUT).as_posix() for p in OUT.glob("lineage-*.jsonl"))
    expected_shards = {name for name in outputs if name.startswith("files/") or
                       re.fullmatch(r"(?:modules(?:-\d{3,})?|lineage-\d{3,})\.jsonl", name)}
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
