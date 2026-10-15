#!/usr/bin/env python3
"""Ratchet game-tier source debt; this is a lexical metric, not a C++ parser.

Comments and strings are masked before counting code constructs. Long lines count
all source text. Multi-statement lines contain two or more semicolons outside
parentheses (so for-loop headers do not count). Address calls count call/ret/invoke
dispatch helpers with a hex first argument; hex literals count independently.
Refresh with --write-baseline after a migration; increases against HEAD fail even
when the working baseline was edited. New files have a zero allowance.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
BASELINE = "reference/readability-baseline.json"
LAYERS = ("cpu", "math", "engine", "input", "player", "game", "runtime", "enhancements")
METRICS = (
    "hex_literals", "address_identifiers", "address_calls", "goto", "address_labels",
    "long_lines", "multi_statement_lines",
)
# Raw strings must precede ordinary strings. Masking keeps offsets and newlines.
NON_CODE = re.compile(
    r'R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"'
    r'|//(?:\\\n|[^\n])*|/\*.*?\*/'
    r'|"(?:\\.|[^"\\])*"|(?<!\w)(?:u8|u|U|L)?\'(?:\\.|[^\'\\])*\'',
    re.DOTALL,
)
HEX = r"\b0[xX][0-9a-fA-F](?:[0-9a-fA-F'])*(?:\.[0-9a-fA-F']*)?(?:[pP][+-]?\d+)?[uUlLfF]*\b"


def measure(source: str) -> dict[str, int]:
    code = NON_CODE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), source)
    counts = dict.fromkeys(METRICS, 0)
    counts["hex_literals"] = len(re.findall(HEX, code))
    counts["address_identifiers"] = len(re.findall(r"\b\w+_at_[0-9a-fA-F]+\b", code))
    counts["address_calls"] = len(re.findall(
        r"\b(?:call|ret|invoke)(?:_\w+)?\s*\(\s*" + HEX, code))
    counts["goto"] = len(re.findall(r"\bgoto\b", code))
    counts["address_labels"] = len(re.findall(
        r"\b(?:L[0-9a-fA-F]+|(?:loc|LAB|lab)_[0-9a-fA-F]+)\s*:(?!:)", code))
    counts["long_lines"] = sum(len(line.expandtabs(4)) > 120 for line in source.splitlines())
    depth = 0
    for line in code.splitlines():
        terminators = 0
        for char in line:
            if char == "(":
                depth += 1
            elif char == ")":
                depth = max(0, depth - 1)
            elif char == ";" and depth == 0:
                terminators += 1
        counts["multi_statement_lines"] += terminators > 1
    return counts


def snapshot(root: Path) -> dict[str, dict[str, int]]:
    return {
        path.relative_to(root).as_posix(): measure(path.read_text())
        for layer in LAYERS
        for path in sorted((root / "src" / layer).rglob("*"))
        if path.suffix in (".hpp", ".h", ".cpp", ".cc", ".c", ".mm")
    }


def validate(data: dict) -> dict[str, dict[str, int]]:
    if data.get("schema") != 1 or not isinstance(data.get("files"), dict):
        raise ValueError("expected schema 1 with files object")
    for path, counts in data["files"].items():
        parts = Path(path).parts
        if len(parts) < 3 or parts[0] != "src" or parts[1] not in LAYERS or ".." in parts:
            raise ValueError(f"invalid game-tier path: {path}")
        if not isinstance(counts, dict) or set(counts) != set(METRICS):
            raise ValueError(f"invalid metrics: {path}")
        if any(type(n) is not int or n < 0 for n in counts.values()):
            raise ValueError(f"invalid counts: {path}")
    return data["files"]


def regressions(current: dict, baseline: dict, label: str) -> list[str]:
    return [
        f"{path}: {metric} {counts[metric]} > {baseline.get(path, {}).get(metric, 0)} ({label})"
        for path, counts in current.items()
        for metric in METRICS
        if counts[metric] > baseline.get(path, {}).get(metric, 0)
    ]


def committed_baseline(root: Path) -> dict | None:
    # A source archive has no HEAD: it still enforces the supplied baseline.
    result = subprocess.run(
        ["git", "show", f"HEAD:{BASELINE}"], cwd=root, capture_output=True, text=True)
    if result.returncode:
        tracked = subprocess.run(
            ["git", "ls-tree", "HEAD", "--", BASELINE], cwd=root, capture_output=True, text=True)
        if tracked.returncode == 0 and tracked.stdout.strip():
            raise ValueError("cannot read committed baseline")
        return None
    return validate(json.loads(result.stdout))


def totals(files: dict) -> dict[str, int]:
    return {metric: sum(row[metric] for row in files.values()) for metric in METRICS}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--write-baseline", action="store_true")
    args = parser.parse_args()
    root = args.root.resolve()
    path = root / BASELINE
    try:
        current = snapshot(root)
        previous = committed_baseline(root)
        errors = []
        if path.exists():
            baseline = validate(json.loads(path.read_text()))
            errors += regressions(current, baseline, "working baseline")
            if previous is not None:
                errors += regressions(baseline, previous, "baseline increase against HEAD")
        elif not args.write_baseline or previous is not None:
            raise ValueError(f"missing {BASELINE}")
        if previous is not None:
            errors += regressions(current, previous, "HEAD baseline")
        if errors:
            for error in errors:
                print(f"READABILITY: {error}")
            return 1
        if args.write_baseline:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps({"schema": 1, "files": current}, indent=2) + "\n")
        print(f"Readability: {len(current)} files, {totals(current)}, 0 regressions")
        return 0
    except (ValueError, OSError) as error:
        print(f"READABILITY: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
