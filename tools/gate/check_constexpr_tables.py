#!/usr/bin/env python3
"""Require an explicit review of named constexpr aggregates hidden by the literal ratchet.

This is a bounded lexical census, not C++ type resolution or an address detector.
Review classifications describe consumer semantics. Raw numeric tokens never prove
that a value is an executable address. Existing open migrations remain visible debt.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from check_readability import CONSTEXPR, HEX, LAYERS, NON_CODE, NUMBER, SOURCE_SUFFIXES

ROOT = Path(__file__).resolve().parents[2]
REGISTRY = "reference/constexpr-table-review.json"
CLASSIFICATIONS = {"numeric_encoding", "typed_content", "named_structure",
                   "reference_metadata", "open_migration", "open_review"}
SCALAR = re.compile(r"(?:(?:(?:std::)?(?:u?int(?:_fast|_least)?(?:8|16|32|64)_t|size_t)"
                    r"|bool|char|short|int|long|float|double|unsigned|signed|const|volatile)\b\s*)+")


def declarations(source: str) -> list[dict]:
    """Find array and other braced aggregate candidates, retaining exact source spans.

    Builtin numeric scalar initializers stay outside this census. Custom value types
    with braces are candidates because a lexical tool cannot prove their structure.
    """
    code = NON_CODE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), source)
    found = []
    for match in CONSTEXPR.finditer(code):
        if not re.search(r"[A-Za-z_]\w*", match[1]):
            continue
        if match[3] == "(" and not re.match(
                r"\s*(?:\(\s*)*(?:\{|[\w:]+(?:\s*<[^(){};]*>)?\s*\{)", code[match.end():]):
            if not re.search(r"\b(?:array|to_array)\b", match[1]):
                continue
            arguments = code[match.end():].split(")", 1)[0].strip()
            if not arguments or re.match(r"[\w:]+\s+(?:[&*]\s*)?[A-Za-z_]\w*\s*(?:[,=]|$)", arguments):
                continue  # Supported function declaration, not an object initializer.
            raise ValueError(f"ambiguous parenthesized array declaration requires review: {match[2]}")
        depth = 0
        end = None
        for index in range(match.end() - 1, len(code)):
            char = code[index]
            if char in "({[":
                depth += 1
            elif char in ")}]":
                depth -= 1
            elif char == ";" and depth == 0:
                end = index + 1
                break
        if end is None:
            raise ValueError(f"unterminated constexpr candidate: {match[2]}")
        prefix = code[match.start():match.end()]
        body = code[match.end():end]
        array = bool(re.search(r"\b(?:array|to_array)\b|\[", prefix + body))
        scalar = bool(SCALAR.fullmatch(match[1].strip()))
        if not array and ("{" not in prefix + body or scalar):
            continue
        exact = source[match.start():end]
        found.append({"declaration": match[2], "line": code.count("\n", 0, match.start()) + 1,
                      "sha256": hashlib.sha256(exact.encode()).hexdigest(),
                      "raw_hex_tokens": sum(bool(re.fullmatch(HEX, token.group()))
                                            for token in NUMBER.finditer(code[match.start():end]))})
    return found


def census(root: Path) -> dict[str, dict]:
    result = {}
    for layer in LAYERS:
        for path in sorted((root / "src" / layer).rglob("*")):
            if path.suffix not in SOURCE_SUFFIXES or not path.is_file():
                continue
            for entry in declarations(path.read_text()):
                key = f"{path.relative_to(root).as_posix()}::{entry['declaration']}"
                if key in result:
                    raise ValueError(f"ambiguous declaration identity; review scopes explicitly: {key}")
                result[key] = entry
    return result


def validate_registry(data: dict) -> dict:
    if data.get("schema") != 1 or not isinstance(data.get("tables"), dict):
        raise ValueError("expected schema 1 table review registry")
    for key, entry in data["tables"].items():
        if not isinstance(entry, dict) or entry.get("classification") not in CLASSIFICATIONS:
            raise ValueError(f"invalid classification: {key}")
        if not re.fullmatch(r"[0-9a-f]{64}", entry.get("sha256", "")):
            raise ValueError(f"missing declaration fingerprint: {key}")
        if (not isinstance(entry.get("purpose"), str) or not entry["purpose"] or
                not all(isinstance(entry.get(field), list) and entry[field]
                        for field in ("evidence", "verification"))):
            raise ValueError(f"review needs purpose, evidence and verification: {key}")
    if not isinstance(data.get("retired", {}), dict):
        raise ValueError("retired reviews must be an object")
    return data["tables"]


def reference_problems(root: Path, data: dict) -> list[str]:
    problems = []
    entries = dict(data["tables"])
    entries.update({key: item for key, item in data.get("retired", {}).items()})
    for key, entry in entries.items():
        for item in entry["evidence"]:
            if not isinstance(item, dict) or not isinstance(item.get("path"), str):
                problems.append(f"invalid source evidence: {key}")
                continue
            path = Path(item["path"])
            if path.is_absolute() or ".." in path.parts or not (root / path).is_file():
                problems.append(f"missing source evidence: {key}: {path}")
                continue
            line = item.get("line")
            if type(line) is not int or not 1 <= line <= len((root / path).read_text().splitlines()):
                problems.append(f"invalid source evidence line: {key}: {path}")
            expected = item.get("sha256")
            if not isinstance(expected, str) or hashlib.sha256((root / path).read_bytes()).hexdigest() != expected:
                problems.append(f"changed source/consumer evidence requires review: {key}: {path}")
        for name in entry["verification"]:
            if not isinstance(name, str):
                problems.append(f"invalid verification path: {key}")
                continue
            path = Path(name)
            if path.is_absolute() or ".." in path.parts or not (root / path).is_file():
                problems.append(f"missing verification path: {key}: {path}")
        if key in data.get("retired", {}) and entry.get("outcome") == "migrated":
            successor = entry.get("successor", {})
            path = Path(successor.get("path", ""))
            if (path.is_absolute() or ".." in path.parts or not (root / path).is_file()):
                problems.append(f"missing retired aggregate successor: {key}")
            else:
                content = (root / path).read_text()
                symbol = successor.get("symbol", "")
                code = NON_CODE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), content)
                if (hashlib.sha256((root / path).read_bytes()).hexdigest() != successor.get("sha256") or
                        not re.fullmatch(r"[A-Za-z_]\w*", symbol) or
                        not re.search(r"\b" + re.escape(symbol) + r"\b", code)):
                    problems.append(f"changed/invalid retired aggregate successor: {key}")
    return problems


def review_problems(current: dict, data: dict, previous: dict | None = None) -> list[str]:
    tables = validate_registry(data)
    problems = []
    for key, observed in current.items():
        if key not in tables:
            problems.append(f"unreviewed aggregate: {key}")
        elif tables[key]["sha256"] != observed["sha256"]:
            problems.append(f"changed aggregate requires review: {key}")
    for key in tables.keys() - current.keys():
        problems.append(f"stale aggregate review: {key}")
    retired = data.get("retired", {})
    for key, item in retired.items():
        if (key in tables or not isinstance(item, dict) or not all(
                item.get(field) for field in ("review", "reason", "evidence", "verification")) or
                item.get("outcome") not in {"migrated", "justified_removal"}):
            problems.append(f"invalid retired aggregate lineage: {key}")
            continue
        validate_registry({"schema": 1, "tables": {key: item["review"]}})
        if not isinstance(item.get("history", []), list) or any(
                not isinstance(proof, dict) or proof.get("review") != item["review"]
                for proof in item.get("history", [])):
            problems.append(f"invalid retirement proof history: {key}")
        if item["outcome"] == "migrated" and not isinstance(item.get("successor"), dict):
            problems.append(f"missing retired aggregate successor: {key}")
        if item["outcome"] == "justified_removal" and (
                not isinstance(item.get("removal_proof"), dict) or
                item["removal_proof"] not in item["evidence"]):
            problems.append(f"missing justified removal proof: {key}")
    if previous is not None:
        old = validate_registry(previous)
        for key in retired.keys() - previous.get("retired", {}).keys():
            if key not in old or retired[key].get("history"):
                problems.append(f"retirement requires a prior active review without invented history: {key}")
        for key in old.keys() - tables.keys():
            item = retired.get(key, {})
            if item.get("review") != old[key]:
                problems.append(f"removed review requires preserved lineage: {key}")
        for key, item in previous.get("retired", {}).items():
            updated = retired.get(key)
            if updated == item:
                continue
            prior_proof = {field: value for field, value in item.items() if field != "history"}
            history = [*item.get("history", []), prior_proof]
            if (not isinstance(updated, dict) or updated.get("review") != item.get("review") or
                    updated.get("history") != history):
                problems.append(f"retired review history changed: {key}")
    return problems


def committed_registry(root: Path) -> dict | None:
    process = subprocess.run(["git", "show", f"HEAD:{REGISTRY}"], cwd=root,
                             capture_output=True, text=True)
    if process.returncode == 0:
        return json.loads(process.stdout)
    tracked = subprocess.run(["git", "ls-tree", "HEAD", "--", REGISTRY], cwd=root,
                             capture_output=True, text=True)
    if tracked.returncode == 0 and tracked.stdout.strip():
        raise ValueError("cannot read immutable HEAD registry")
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--candidates", action="store_true",
                        help="print observed declarations without assigning semantic reviews")
    args = parser.parse_args()
    try:
        root = args.root.resolve()
        current = census(root)
        if args.candidates:
            print(json.dumps(current, indent=2))
            return 0
        data = json.loads((root / REGISTRY).read_text())
        problems = review_problems(current, data, committed_registry(root))
        problems += reference_problems(root, data)
        for problem in problems:
            print("CONSTEXPR TABLE: " + problem)
        counts = {kind: sum(entry["classification"] == kind for entry in data["tables"].values())
                  for kind in sorted(CLASSIFICATIONS)}
        retired = data.get("retired", {})
        outcomes = {kind: sum(item.get("outcome") == kind for item in retired.values())
                    for kind in ("migrated", "justified_removal")}
        unresolved = sum(item.get("outcome") not in outcomes for item in retired.values())
        retired_dispositions = {kind: sum(item.get("review", {}).get("classification") == kind
                                         for item in retired.values())
                                for kind in sorted(CLASSIFICATIONS)}
        print(f"Constexpr table reviews: {len(current)} active, {len(current) + len(retired)} historical, "
              f"{counts}, retired {outcomes}, prior dispositions {retired_dispositions}, "
              f"unresolved retirements {unresolved}, {len(problems)} problems")
        return int(bool(problems))
    except (OSError, ValueError) as error:
        print(f"CONSTEXPR TABLE: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
