#!/usr/bin/env python3
"""Ratchet game-tier source debt; this is a lexical metric, not a C++ parser.

Comments and strings are masked before counting code constructs. Long lines count
all source text. Multi-statement lines contain two or more semicolons outside
parentheses (so for-loop headers do not count). Address calls count call/ret/invoke
dispatch helpers with a hex first argument; hex literals count independently.
Refresh with --write-baseline after a migration; increases against HEAD fail even
when the working baseline was edited. New files have a zero allowance.

Named constexpr object declarations exempt hex and large
decimal integer literals. Emulator debt counts X87Context type mentions, fp/typed
context member calls and explicit float/double memory arithmetic sites, deduplicated
per callable token. Address comments count physical lines with 6-8 digit hex
addresses or address labels, except standalone provenance annotations. These are
lexical indicators, not type resolution, address classification or a style verdict.
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
SOURCE_SUFFIXES = (".hpp", ".h", ".cpp", ".cc", ".c", ".mm")
LEGACY_METRICS = (
    "hex_literals", "address_identifiers", "address_calls", "goto", "address_labels",
    "long_lines", "multi_statement_lines",
)
NEW_METRICS = ("large_decimal_literals", "emulator_arithmetic", "address_comments")
METRICS = LEGACY_METRICS + NEW_METRICS
# Raw strings must precede ordinary strings. Masking keeps offsets and newlines.
NON_CODE = re.compile(
    r'R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"'
    r'|//(?:\\\n|[^\n])*|/\*.*?\*/'
    r'|"(?:\\.|[^"\\])*"|(?<!\w)(?:u8|u|U|L)?\'(?:\\.|[^\'\\])*\'',
    re.DOTALL,
)
HEX = r"\b0[xX][0-9a-fA-F](?:[0-9a-fA-F'])*(?:\.[0-9a-fA-F']*)?(?:[pP][+-]?\d+)?[uUlLfF]*\b"
# Consume a whole preprocessing number, including exponent signs and UDL suffixes.
# Searching just for digits would count pieces of floats, identifiers and hex.
NUMBER = re.compile(r"(?<![\w.])(?:\d|\.\d)(?:[eEpP][+-]|[\w.'])*")
DECIMAL_INTEGER = re.compile(r"[1-9][0-9']*(?:[uU](?:ll|LL|[lLzZ])?|(?:ll|LL|[lLzZ])[uU]?)?")
CONSTEXPR = re.compile(
    r"\bconstexpr\s+([^;{}()=]+?)\b([A-Za-z_]\w*)\s*(?:\[[^;{}()=]*\]\s*)*(=|\{|\()")
ADDRESS = re.compile(r"\b(?:0[xX][0-9a-fA-F]{6,8}|(?:loc|LAB|lab)_[0-9a-fA-F]{6,8})\b")
ANNOTATION = re.compile(
    r"//\s*(?:FUNCTION|GLOBAL|STUB|DATA|LIBRARY):\s*"
    r"(?:SONIC|CHRMODELS|LOADER|INPUT|SPEEDRUN|SDL2)\s+0[xX][0-9a-fA-F]+\s+[A-Za-z_]\w*"
    r"|//\s*PATCH:\s*(?:SONIC|CHRMODELS|LOADER|INPUT|SPEEDRUN|SDL2)\s+"
    r"\+?0[xX][0-9a-fA-F]+\s*->\s*0[xX][0-9a-fA-F]+")


def constexpr_mask(code: str) -> str:
    """Mask named object declarations, across physical lines.

    Parenthesized initializers must begin with a numeric expression; ambiguous
    identifier-first forms stay counted rather than exempt function parameters.
    """
    spans = []
    for match in CONSTEXPR.finditer(code):
        # A type must precede the declarator; constexpr alone is not a name.
        if not re.search(r"[A-Za-z_]\w*", match[1]):
            continue
        if match[3] == "(" and not re.match(r"\s*[+~!-]?\s*(?:\d|\.\d)", code[match.end():]):
            continue
        depth = 0
        for end in range(match.end() - 1, len(code)):
            char = code[end]
            if char in "({[":
                depth += 1
            elif char in ")}]":
                depth -= 1
            elif char == ";" and depth == 0:
                spans.append((match.start(), end + 1))
                break
    for start, end in reversed(spans):
        code = code[:start] + re.sub(r"[^\n]", " ", code[start:end]) + code[end:]
    return code


def address_comment_lines(source: str) -> int:
    lines = set()
    for match in NON_CODE.finditer(source):
        comment = match.group()
        if not comment.startswith(("//", "/*")):
            continue
        line_start = source.rfind("\n", 0, match.start()) + 1
        annotation = not source[line_start:match.start()].strip() and ANNOTATION.match(comment)
        first_line = source.count("\n", 0, match.start())
        for offset, line in enumerate(comment.splitlines()):
            # Exempt only the annotation's physical line, including when a
            # backslash continued the comment over more instruction markers.
            if offset == 0 and annotation:
                continue
            if ADDRESS.search(line):
                lines.add(first_line + offset)
    return len(lines)


def emulator_sites(code: str) -> int:
    # Type mentions are debt indicators too (parameters, members, constructors).
    sites = {m.start() for m in re.finditer(r"\bX87Context\b", code)}
    receivers = {"fp"}
    receivers.update(re.findall(r"\bX87Context\s*(?:const\s*)?[&*]?\s*(\w+)", code))
    for match in re.finditer(r"\b(\w+)\s*(?:\.|->)\s*(\w+)\s*\(", code):
        if match[1] in receivers:
            sites.add(match.start(2))
    # Memory-operand operations remain visible after renaming the context, and
    # with free/static calls. Deduplicate fp.multiply_float_memory at one site.
    for match in re.finditer(
            r"\b(?:reverse_)?(?:add|subtract|multiply|divide|compare)_(?:float|double)_memory\s*\(", code):
        sites.add(match.start())
    return len(sites)


def measure(source: str) -> dict[str, int]:
    code = NON_CODE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), source)
    counts = dict.fromkeys(METRICS, 0)
    literals = constexpr_mask(code)
    counts["hex_literals"] = sum(bool(re.fullmatch(HEX, m.group()))
                                 for m in NUMBER.finditer(literals))
    counts["large_decimal_literals"] = sum(
        bool(DECIMAL_INTEGER.fullmatch(m.group())) and
        int(re.sub(r"[uUlLzZ]", "", m.group()).replace("'", "")) >= 65536
        for m in NUMBER.finditer(literals))
    counts["emulator_arithmetic"] = emulator_sites(code)
    counts["address_comments"] = address_comment_lines(source)
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
        if path.suffix in SOURCE_SUFFIXES
    }


def validate(data: dict, *, legacy: bool = False) -> dict[str, dict[str, int]]:
    schema = data.get("schema")
    if schema not in ((1, 2) if legacy else (2,)) or not isinstance(data.get("files"), dict):
        raise ValueError("expected schema 2 with files object (schema 1 migration requires Git HEAD)")
    for path, counts in data["files"].items():
        parts = Path(path).parts
        if len(parts) < 3 or parts[0] != "src" or parts[1] not in LAYERS or ".." in parts:
            raise ValueError(f"invalid game-tier path: {path}")
        metrics = LEGACY_METRICS if schema == 1 else METRICS
        if not isinstance(counts, dict) or set(counts) != set(metrics):
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
    data = json.loads(result.stdout)
    files = validate(data, legacy=True)
    if data["schema"] == 1:
        # Seed only from immutable HEAD source, never from the working tree or
        # user-edited baseline. Include tracked sources absent from the old
        # baseline: their old metrics still have zero allowance, but the new
        # measurements must cover ALL original HEAD source. New files get zero.
        tree = subprocess.run(["git", "ls-tree", "-r", "--name-only", "HEAD", "--", "src"],
                              cwd=root, capture_output=True, text=True)
        if tree.returncode:
            raise ValueError("cannot enumerate HEAD source for metric migration")
        for path in tree.stdout.splitlines():
            parts = Path(path).parts
            if len(parts) < 3 or parts[1] not in LAYERS or Path(path).suffix not in SOURCE_SUFFIXES:
                continue
            counts = files.setdefault(path, dict.fromkeys(LEGACY_METRICS, 0))
            source = subprocess.run(["git", "show", f"HEAD:{path}"], cwd=root,
                                    capture_output=True, text=True)
            if source.returncode:
                raise ValueError(f"cannot seed new metrics from HEAD:{path}")
            measured = measure(source.stdout)
            counts.update({metric: measured[metric] for metric in NEW_METRICS})
        for counts in files.values():
            for metric in NEW_METRICS:
                counts.setdefault(metric, 0)
    return files


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
            data = json.loads(path.read_text())
            baseline = validate(data, legacy=previous is not None)
            if data["schema"] == 1:
                for name in previous:
                    baseline.setdefault(name, dict.fromkeys(LEGACY_METRICS, 0))
                for name, counts in baseline.items():
                    counts.update({metric: previous.get(name, {}).get(metric, 0)
                                   for metric in NEW_METRICS})
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
            path.write_text(json.dumps({"schema": 2, "files": current}, indent=2) + "\n")
        print(f"Readability: {len(current)} files, {totals(current)}, 0 regressions")
        return 0
    except (ValueError, OSError) as error:
        print(f"READABILITY: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
