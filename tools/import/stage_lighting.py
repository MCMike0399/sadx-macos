#!/usr/bin/env python3
"""Import the PC stage and character lighting tables with exact float round trips."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
from pe_image import PEImage
from extract import SOURCE_SHA256

ENVIRONMENT = (("direction", 3), ("specular", 1), ("diffuse", 1),
               ("color", 3), ("ambient", 3))
PALETTE = (("direction", 3), ("diffuse", 1), ("ambient", 3),
           ("upper_color_power", 1), ("upper_color", 3),
           ("upper_specular_power", 1), ("upper_specular", 3),
           ("lower_color_power", 1), ("lower_color", 3),
           ("lower_specular_power", 1), ("lower_specular", 3))
SOURCES = (("default_stage", 0x9008b4, 1, False),
           ("default_character", 0x9008e4, 1, False),
           ("stages", 0x900e88, 256, False),
           ("palettes", 0x903e88, 256, True))


def decode(raw, palette=False):
    fields = PALETTE if palette else ENVIRONMENT
    expected = 96 if palette else 48
    if len(raw) != expected:
        raise ValueError("lighting record has wrong length")
    result = dict(zip(("stage", "act", "character" if palette else "light_index", "flags"), raw[:4]))
    offset = 4
    for name, count in fields:
        values = struct.unpack_from("<" + "f" * count, raw, offset)
        if any(not math.isfinite(value) for value in values):
            raise ValueError("nonfinite lighting source value requires an explicit representation")
        result[name] = values[0] if count == 1 else list(values)
        offset += count * 4
    return result


def encode(record, palette=False):
    fields = PALETTE if palette else ENVIRONMENT
    prefix = ("stage", "act", "character" if palette else "light_index", "flags")
    result = bytearray(record[name] for name in prefix)
    for name, count in fields:
        values = [record[name]] if count == 1 else record[name]
        if len(values) != count:
            raise ValueError("lighting vector has wrong length")
        result.extend(struct.pack("<" + "f" * count, *values))
    return bytes(result)


def import_stage_lighting(game, output):
    image = PEImage(Path(game) / "sonic.exe")
    if image.sha256 != SOURCE_SHA256:
        raise ValueError("stage lighting requires the pinned PC executable")
    content = {"version": 1}
    evidence = {"version": 1, "source_sha256": image.sha256, "tables": []}
    for name, address, count, palette in SOURCES:
        size = 96 if palette else 48
        raw = image.read(address, count * size)
        records = [decode(raw[index * size:(index + 1) * size], palette) for index in range(count)]
        # Exercise the serialized representation too: decimal JSON must retain every source float bit.
        records = json.loads(json.dumps(records, allow_nan=False))
        inverse = b"".join(encode(record, palette) for record in records)
        if inverse != raw:
            raise ValueError("lighting import did not preserve original record bytes")
        if count > 1 and not any(record["stage"] == 255 for record in records):
            raise ValueError("lighting table lacks a source sentinel")
        content[name] = records[0] if count == 1 else records
        evidence["tables"].append({"name": name, "address": address, "records": count,
                                   "record_bytes": size, "sha256": hashlib.sha256(raw).hexdigest(),
                                   "inverse_byte_match": True})
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    (output / "stage-lighting.json").write_text(json.dumps(content, indent=2, allow_nan=False) + "\n")
    (output / "stage-lighting-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    return evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_directory", type=Path)
    parser.add_argument("output_directory", type=Path)
    args = parser.parse_args()
    print(json.dumps(import_stage_lighting(args.game_directory, args.output_directory)))


if __name__ == "__main__":
    main()
