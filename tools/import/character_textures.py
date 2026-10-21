#!/usr/bin/env python3
"""Import the nine source character PVM declarations as pointer-free JSON metadata.

PVMEntry/NJS_TEXLIST layouts and CharacterPVMEntries come from the pinned SDK.
Source pointers are retained only in the separate inverse-byte proof, never metadata.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
from pe_image import PEImage

SOURCE_SHA = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
# DATA: SONIC 0x90ed54 CharacterPVMEntries size=36
CHARACTER_TABLE = 0x90ed54
# NOTSA: stable semantic resource names, keyed by original archive names.
RESOURCE_NAMES = {
    "SONIC": "sonic", "SON_EFF": "sonic_effects", "EGGMAN": "eggman",
    "MILES": "tails", "KNU_EFF": "knuckles_effects", "KNUCKLES": "knuckles",
    "TIKAL": "tikal", "AMY": "amy", "AMY_EFF": "amy_effects",
    "E102": "gamma", "E102BEAM": "gamma_beam", "E102EFFECT": "gamma_effects",
    "E102TIME": "gamma_time", "E102HIT": "gamma_hit", "FISHING": "fishing",
    "BIG": "big", "BIG_KAERU": "froggy", "METALSONIC": "metal_sonic",
}


def declarations(image):
    if image.sha256 != SOURCE_SHA:
        raise ValueError("unsupported character texture source SHA256")
    chars, provenance, resources = [], [], {}
    tables = image.unpack("<9I", CHARACTER_TABLE)
    reconstructed_tables = []
    for character, table in enumerate(tables):
        if not table:
            raise ValueError("source character table is absent")
        bindings, restored, proof = [], bytearray(), []
        for index in range(32):
            address = table + index * 8
            name, texlist = image.unpack("<II", address)
            if not texlist:
                restored.extend(struct.pack("<II", name, 0))
                break
            if not name:
                raise ValueError("source archive name is absent")
            archive = image.string(name)
            if archive not in RESOURCE_NAMES:
                raise ValueError("unknown source character archive " + archive)
            texnames, count = image.unpack("<II", texlist)
            if not texnames or not 0 < count <= 4096:
                raise ValueError("invalid source texture declaration")
            resource = RESOURCE_NAMES[archive]
            previous = resources.setdefault(resource, (texlist, count))
            if previous != (texlist, count):
                raise ValueError("inconsistent resource alias")
            bindings.append({"archive": archive, "texture_count": count,
                             "resource_name": resource})
            # Restore each emitted semantic binding via the source symbol maps.
            restored.extend(struct.pack("<II", name, resources[resource][0]))
            texlist_bytes = struct.pack("<II", texnames, bindings[-1]["texture_count"])
            if texlist_bytes != image.read(texlist, 8):
                raise ValueError("texture-list source-byte roundtrip failed")
            proof.append({"source_entry": hex(address), "archive_pointer": hex(name),
                          "texlist_pointer": hex(texlist), "texnames_pointer": hex(texnames),
                          "texlist_sha256": hashlib.sha256(texlist_bytes).hexdigest(),
                          "resource_name": resource, "round_trip_exact": True})
        else:
            raise ValueError("unterminated source character PVM list")
        if restored != image.read(table, len(restored)):
            raise ValueError("character table source-byte roundtrip failed")
        chars.append({"id": character, "bindings": bindings})
        provenance.append({"character_id": character, "source_table": hex(table),
                           "source_bytes": len(restored), "bindings": proof,
                           "source_sha256": hashlib.sha256(restored).hexdigest(),
                           "round_trip_exact": True})
        reconstructed_tables.append(table)
    if struct.pack("<9I", *reconstructed_tables) != image.read(CHARACTER_TABLE, 36):
        raise ValueError("character directory source-byte roundtrip failed")
    return {"version": 1, "characters": chars}, {
        "source_sha256": image.sha256, "directory_source": hex(CHARACTER_TABLE),
        "directory_sha256": hashlib.sha256(image.read(CHARACTER_TABLE, 36)).hexdigest(),
        "directory_round_trip_exact": True, "characters": provenance}


def import_character_textures(game, output):
    source = Path(game)
    if source.is_dir():
        source = next((path for path in source.iterdir()
                       if path.name.casefold() == "sonic.exe"), source / "sonic.exe")
    metadata, proof = declarations(PEImage(source))
    output = Path(output)
    if output.suffix.casefold() != ".json":
        output = output / "character-textures.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = (json.dumps(metadata, indent=2) + "\n").encode()
    output.write_bytes(payload)
    proof["metadata_sha256"] = hashlib.sha256(payload).hexdigest()
    output.with_suffix(".provenance.json").write_text(json.dumps(proof, indent=2) + "\n")
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_executable", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    metadata = import_character_textures(args.source_executable, args.output)
    print(json.dumps({"characters": len(metadata["characters"]),
                      "bindings": sum(len(c["bindings"]) for c in metadata["characters"])}))


if __name__ == "__main__":
    main()
