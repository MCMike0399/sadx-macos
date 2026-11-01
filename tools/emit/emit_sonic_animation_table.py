"""Emit typed Sonic content from pinned source statements or their retained ledger.

This emits content values only. Read/publication order is implemented in named
initializer steps and checked independently against original instruction hooks.
"""
import argparse
import hashlib
import json
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "tools/lib"), str(ROOT / "tools/oracles")]
from pe_image import PEImage
from extract import SOURCE_SHA256

BASE = 0x3C56210
COUNT = 147
FIELDS = {0: "action_index", 4: "object_count", 5: "mode", 6: "next", 8: "transition_bits", 12: "step_bits"}
MODES = ("Initialize", "Select", "Transition", "Loop", "Next", "Hold", "ReverseNext",
         "ReverseHold", "SavedNext", "SpeedLoop", "ChargeLoop", "ChargeNext", "Wrap")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def source_statements(path, image):
    if path.suffix == ".json":
        ledger = json.loads(path.read_text())
        assert ledger["source_sha256"] == image.sha256
        assert ledger["procedure_address"] == "0x7cec90" and ledger["source_bytes"] == 7674
        assert ledger["procedure_sha256"] == sha(image.read(0x7CEC90, 7674))
        return ledger["statements"]
    statements = []
    for line in path.read_text().splitlines():
        if "=" not in line:
            continue
        match = re.fullmatch(r"\s*_?DAT_([0-9a-f]+) = (DAT_038f6e7c\[(0x[0-9a-f]+|[0-9]+)\]|\*DAT_038f6e7c|0x[0-9a-f]+|[0-9]+);", line)
        if not match:
            raise RuntimeError("Unrecognized source assignment: " + line)
        address = int(match[1], 16)
        offset = (address - BASE) % 16
        expression = match[2]
        value = int(match[3], 0) if match[3] else 0 if expression == "*DAT_038f6e7c" else int(expression, 0)
        exported = expression.startswith(("DAT_038f6e7c[", "*DAT_038f6e7c"))
        if offset == 0 and not exported:
            if value != 0:
                raise RuntimeError("Unresolved non-null literal action")
            value = None
        if offset != 0 and exported:
            raise RuntimeError("Unexpected pointer coercion")
        statements.append(dict(address=hex(address), field=FIELDS[offset], value=value))
    return statements


def scalar(bits):
    value = struct.unpack("<f", struct.pack("<I", bits))[0]
    for digits in range(1, 10):
        text = format(value, f".{digits}g")
        if struct.pack("<f", float(text)) == struct.pack("<I", bits):
            break
    if "." not in text and "e" not in text:
        text += ".0"
    return text + "f"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("source", type=pathlib.Path, help="Retained source statements or pinned recipe JSON")
    parser.add_argument("--header", type=pathlib.Path, required=True)
    parser.add_argument("--metadata", type=pathlib.Path, required=True)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise RuntimeError("Source mismatch")
    statements = source_statements(args.source, image)
    rows = [{} for _ in range(COUNT)]
    for statement in statements:
        row, offset = divmod(int(statement["address"], 16) - BASE, 16)
        if not 0 <= row < COUNT or offset not in FIELDS or offset in rows[row]:
            raise RuntimeError("Unexpected/duplicate source field")
        if statement["field"] != FIELDS[offset]:
            raise RuntimeError("Source field meaning differs")
        rows[row][offset] = statement["value"]
    if len(statements) != 882 or any(set(row) != set(FIELDS) for row in rows):
        raise RuntimeError("Incomplete source table")
    header = ["#pragma once", "", '#include "player/animation/animation_content.hpp"',
              "#include <array>", "", "namespace sadx::animation {",
              "// DATA: SONIC 0x3c56210 sonic_action",
              "inline constexpr std::array<SonicRecipe, 147> kSonicRecipes{{"]
    for row in rows:
        header.append("    {" + ("true" if row[0] is not None else "false") + ", " +
                      str(row[0] if row[0] is not None else 0) + ", " + str(row[4]) + ", Mode::" +
                      MODES[row[5]] + ", " + str(row[6]) + ", " + scalar(row[8]) + ", " + scalar(row[12]) + "},")
    header += ["}};", "} // namespace sadx::animation"]
    args.header.write_text("\n".join(header) + "\n")
    metadata = dict(source_sha256=image.sha256, procedure_address="0x7cec90", source_bytes=7674,
                    procedure_sha256=sha(image.read(0x7CEC90, 7674)),
                    source_statement_input_sha256=sha(args.source.read_bytes()),
                    generated_header_sha256=sha(args.header.read_bytes()), records=COUNT,
                    assignment_count=len(statements), statements=statements,
                    limits=["Content generation does not establish read/publication order",
                            "Original instruction hooks independently judge all 147 rows and 882 stores"])
    args.metadata.write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps({key: metadata[key] for key in ("records", "assignment_count", "procedure_sha256", "generated_header_sha256")}))


if __name__ == "__main__":
    main()
