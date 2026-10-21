#!/usr/bin/env python3
"""Import pointer-free PC title/menu transitions and the initial texture-ID list."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
from pe_image import PEImage

SOURCE_SHA = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
SEQUENCE_POINTERS = 0x10D7B40
INITIAL_TEXTURE_IDS = 0x7EF1A0
MODE_NAMES = ("None", "CharacterSelect", "FileSelect", "TrialActSelect", "SoundTest",
              "Title", "TitleMenu", "OptionSelect", "Explanation", "MinigameSelect",
              "EmblemView", "InternetDemo", "TvSettings", "Puti")
MAX_RECORDS = 64


def mode_name(value):
    if value < 0 or value >= len(MODE_NAMES):
        raise ValueError(f"unsupported menu mode {value}")
    return MODE_NAMES[value]


def mode_number(name):
    try:
        return MODE_NAMES.index(name)
    except ValueError as error:
        raise ValueError(f"unsupported menu mode name {name!r}") from error


def transition(values):
    return dict(zip(("previous", "next", "next_sub"), map(mode_name, values)))


def encode_sequence(sequence):
    """Invert the semantic records, including exactly the source sentinel word."""
    first = sequence["born_mode"]
    if first == "None":
        if sequence["initial_transition"] is not None or sequence["following"]:
            raise ValueError("empty sequence contains transitions")
        return struct.pack("<i", 0)
    records = [{"mode": first, "transition": sequence["initial_transition"]}]
    records += sequence["following"]
    raw = bytearray()
    for record in records:
        mode = mode_number(record["mode"])
        if mode == 0:
            raise ValueError("unexpected sequence sentinel in semantic records")
        selection = record["transition"]
        values = [mode_number(selection[key]) for key in ("previous", "next", "next_sub")]
        raw.extend(struct.pack("<4i", mode, *values))
    raw.extend(struct.pack("<i", 0))
    return bytes(raw)


def read_sequence(image, address):
    records = []
    for index in range(MAX_RECORDS):
        mode, = struct.unpack("<i", image.read(address + 16 * index, 4))
        if mode == 0:
            break
        values = struct.unpack("<3i", image.read(address + 16 * index + 4, 12))
        records.append({"mode": mode_name(mode), "transition": transition(values)})
    else:
        raise ValueError("menu sequence has no bounded zero-mode terminator")
    result = {"born_mode": records[0]["mode"] if records else "None",
              "initial_transition": records[0]["transition"] if records else None,
              "following": records[1:]}
    raw = encode_sequence(result)
    if raw != image.read(address, len(raw)):
        raise ValueError("title menu sequence inverse mismatch")
    return result, raw


def extract_title_menu(image):
    """Only this import boundary reads original pointers; emitted records contain none."""
    if image.sha256 != SOURCE_SHA:
        raise ValueError("title menu tables require the pinned original PC executable")
    raw_pointers = image.read(SEQUENCE_POINTERS, 12)
    pointers = struct.unpack("<3I", raw_pointers)
    unique, sequences, entry_sequences, ranges = [], [], [], {}
    for address in pointers:
        if not address:
            raise ValueError("null title menu sequence pointer")
        if address not in unique:
            sequence, raw = read_sequence(image, address)
            unique.append(address)
            sequences.append(sequence)
            ranges[f"{address:#x}-{address + len(raw):#x}"] = hashlib.sha256(raw).hexdigest()
        entry_sequences.append(unique.index(address))
    inverse_pointers = struct.pack("<3I", *(unique[index] for index in entry_sequences))
    if inverse_pointers != raw_pointers:
        raise ValueError("title entry alias inverse mismatch")
    texture_ids = []
    for index in range(MAX_RECORDS):
        value, = struct.unpack("<i", image.read(INITIAL_TEXTURE_IDS + index * 4, 4))
        if value == 30:
            break
        if value < 0 or value >= 30:
            raise ValueError(f"unsupported initial menu texture ID {value}")
        texture_ids.append(value)
    else:
        raise ValueError("initial menu texture list has no bounded terminator")
    raw_textures = struct.pack(f"<{len(texture_ids) + 1}i", *texture_ids, 30)
    if raw_textures != image.read(INITIAL_TEXTURE_IDS, len(raw_textures)):
        raise ValueError("initial title texture list inverse mismatch")
    ranges[f"{SEQUENCE_POINTERS:#x}-{SEQUENCE_POINTERS + 12:#x}"] = hashlib.sha256(raw_pointers).hexdigest()
    ranges[f"{INITIAL_TEXTURE_IDS:#x}-{INITIAL_TEXTURE_IDS + len(raw_textures):#x}"] = hashlib.sha256(raw_textures).hexdigest()
    content = {"version": 1, "source_sha256": SOURCE_SHA, "entry_sequences": entry_sequences,
               "sequences": sequences, "initial_texture_ids": texture_ids}
    proof = {"source_sha256": SOURCE_SHA, "source_ranges": ranges,
             "sequence_count": len(sequences), "entry_sequences": entry_sequences,
             "inverse_matches": True, "initial_texture_count": len(texture_ids)}
    return content, proof


def extract_title_selection(image):
    """Import stage catalogs used by both constructors; preserve order and duplicate entries."""
    if image.sha256 != SOURCE_SHA:
        raise ValueError("title selection requires the pinned original PC executable")
    ranges = {}
    catalogs = {}
    for name, base in (("trial_stages", 0x7EF5EC), ("subgame_stages", 0x7EF624)):
        characters = []
        for character in range(7):
            raw = image.read(base + character * 8, 8)
            pointer, count = struct.unpack("<IB3x", raw)
            if count > 10 or (name == "trial_stages" and count == 0):
                raise ValueError("invalid title selection stage count")
            pairs = image.read(pointer, count * 2)
            entries = [{"stage": pairs[i], "act": pairs[i + 1]}
                       for i in range(0, len(pairs), 2)]
            if any(entry["stage"] >= 43 for entry in entries):
                raise ValueError("title selection stage exceeds save completion table")
            inverse = bytes(value for entry in entries for value in (entry["stage"], entry["act"]))
            if inverse != pairs:
                raise ValueError("title selection inverse mismatch")
            characters.append(entries)
            ranges[f"{pointer:#x}-{pointer + len(pairs):#x}"] = hashlib.sha256(pairs).hexdigest()
            address = base + character * 8
            ranges[f"{address:#x}-{address + 8:#x}"] = hashlib.sha256(raw).hexdigest()
        catalogs[name] = characters
    raw_characters = image.read(0x7EF6B4, 14)
    character_ids = list(struct.unpack("<7H", raw_characters))
    if any(character >= 8 for character in character_ids):
        raise ValueError("title character exceeds save completion table")
    display_stage_ids = list(image.read(0x7EF688, 44))
    catalogs.update(character_ids=character_ids, display_stage_ids=display_stage_ids)
    ranges["0x7ef6b4-0x7ef6c2"] = hashlib.sha256(raw_characters).hexdigest()
    ranges["0x7ef688-0x7ef6b4"] = hashlib.sha256(bytes(display_stage_ids)).hexdigest()
    return catalogs, ranges


def extract_mode_textures(image):
    if image.sha256 != SOURCE_SHA:
        raise ValueError("title mode textures require the pinned original PC executable")
    result, ranges = [], {}
    raw_pointers = image.read(0x10D7CC4, 14 * 4)
    for pointer in struct.unpack("<14I", raw_pointers):
        values = []
        for index in range(31):
            value, = struct.unpack("<i", image.read(pointer + index * 4, 4))
            if value == 30:
                break
            if not 0 <= value < 30:
                raise ValueError("invalid title mode texture ID")
            values.append(value)
        else:
            raise ValueError("title mode texture list has no bounded terminator")
        inverse = struct.pack(f"<{len(values) + 1}i", *values, 30)
        if inverse != image.read(pointer, len(inverse)):
            raise ValueError("title mode texture inverse mismatch")
        result.append(values)
        ranges[f"{pointer:#x}-{pointer + len(inverse):#x}"] = hashlib.sha256(inverse).hexdigest()
    ranges["0x10d7cc4-0x10d7cfc"] = hashlib.sha256(raw_pointers).hexdigest()
    return result, ranges


BACKGROUND_FIELDS = ("compact_logo_x", "compact_logo_y", "logo_x", "logo_y", "logo_start_x",
                     "text_start_x", "text_step", "text_x", "text_y")


def extract_background_layout(image):
    if image.sha256 != SOURCE_SHA:
        raise ValueError("title background requires the pinned original PC executable")
    raw = image.read(0x7EF928, 36)
    values = struct.unpack("<9f", raw)
    import math
    if not all(math.isfinite(value) for value in values):
        raise ValueError("nonfinite title background layout")
    result = dict(zip(BACKGROUND_FIELDS, values))
    decoded = json.loads(json.dumps(result))
    inverse = struct.pack("<9f", *(decoded[name] for name in BACKGROUND_FIELDS))
    if inverse != raw:
        raise ValueError("title background layout float inverse mismatch")
    words = dict(zip(BACKGROUND_FIELDS, struct.unpack("<9I", raw)))
    return result, words, {"0x7ef928-0x7ef94c": hashlib.sha256(raw).hexdigest()}


def import_title_menu(game_directory, output_directory):
    source = Path(game_directory)
    if source.is_dir():
        source = source / "sonic.exe"
    image = PEImage(source)
    content, proof = extract_title_menu(image)
    selection, selection_ranges = extract_title_selection(image)
    content["selection_data"] = selection
    proof["source_ranges"].update(selection_ranges)
    modes, mode_ranges = extract_mode_textures(image)
    content["mode_texture_ids"] = modes
    layout, layout_bits, layout_ranges = extract_background_layout(image)
    content["background_layout"] = layout
    content["background_layout_bits"] = layout_bits
    proof["source_ranges"].update(layout_ranges)
    proof["source_ranges"].update(mode_ranges)
    output = Path(output_directory)
    output.mkdir(parents=True, exist_ok=True)
    (output / "title-menu.json").write_text(json.dumps(content, indent=2) + "\n")
    # Provenance is data hashes, not an executable/DLL or a runtime pointer table.
    (output / "title-menu.proof.json").write_text(json.dumps(proof, indent=2) + "\n")
    return content


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_directory", type=Path)
    parser.add_argument("output_directory", type=Path)
    args = parser.parse_args()
    content = import_title_menu(args.game_directory, args.output_directory)
    print(json.dumps({"sequences": len(content["sequences"]),
                      "entry_sequences": content["entry_sequences"],
                      "initial_texture_ids": content["initial_texture_ids"]}))


if __name__ == "__main__":
    main()
