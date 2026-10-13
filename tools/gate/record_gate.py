#!/usr/bin/env python3
"""Record a frozen build/test gate: command, log digest, programs run and exact
source/artifact SHA-256 identities.

The record describes only what the supplied log shows ran. It does not certify
later source edits; rerun the gate and write a new record after any change.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path


SOURCE_GLOBS = ("Makefile", "*.mk", "tools/**/*.py", "tools/**/*.tsv", "src/**/*.hpp", "src/**/*.cpp", "src/**/*.mm", "tests/**/*.hpp", "tests/**/*.cpp", "tests/**/*.mm", "tests/**/*.py", "tests/**/*.tsv")
ARTIFACT_GLOBS = ("build/*.a", "build/*-test", "build/native-*-probe.dylib", "build/sadx-metal")
PROGRAM_LINE = re.compile(rb"^\./build/([A-Za-z0-9_.-]+)(?:\s|$)")
UNITTEST_LINE = re.compile(rb"^Ran (\d+) tests? in ")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2], help="repository root")
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--command", required=True, help="exact command that produced the log")
    parser.add_argument("--exit-status", type=int, required=True, help="observed exit status of the command")
    parser.add_argument("--limit", action="append", default=[], help="scope limit; repeatable")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    log_bytes = args.log.read_bytes()
    programs: list[str] = []
    python_tests: int | None = None
    for line in log_bytes.splitlines():
        program = PROGRAM_LINE.match(line)
        if program:
            programs.append("./build/" + program.group(1).decode("ascii"))
        tests = UNITTEST_LINE.match(line)
        if tests:
            python_tests = (python_tests or 0) + int(tests.group(1))

    root = args.root.resolve()
    prefix = root.name
    identities: dict[str, str] = {}
    for pattern in SOURCE_GLOBS + ARTIFACT_GLOBS:
        for path in sorted(root.glob(pattern)):
            if path.is_file():
                identities[f"{prefix}/{path.relative_to(root).as_posix()}"] = sha256(path)

    record = {
        "command": args.command,
        "exit_status": args.exit_status,
        "passed": args.exit_status == 0,
        "native_programs": len(programs),
        "python_tests": python_tests,
        "programs": programs,
        "log_sha256": hashlib.sha256(log_bytes).hexdigest(),
        "source_and_artifact_sha256": identities,
        "limits": args.limit,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(record, indent=1) + "\n")
    print(f"{args.output}: exit {args.exit_status}, {len(programs)} native programs, python tests {python_tests}, {len(identities)} identities")
    return 0


if __name__ == "__main__":
    sys.exit(main())
