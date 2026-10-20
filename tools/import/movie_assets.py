#!/usr/bin/env python3
"""Losslessly import the pinned English profile's movie assets.

This copies data only. AVFoundation validates/decodes MPEG at playback; no decoder,
original executable, DLL, or FFmpeg process is required by the native product.
The recovered English clip declarations reference RE-US and SA1..SA8. RE-JP is
not silently substituted: language redirection is not recovered yet.
"""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import shutil
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
from pe_image import PEImage

MOVIES = ("RE-US.MPG",) + tuple(f"SA{index}.MPG" for index in range(1, 9))
SOURCE_SHA = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"


def movie_content(source_exe, source_ini):
    """Extract data tables only. The game owns settings conversion and playback behavior."""
    image = PEImage(source_exe)
    if image.sha256 != SOURCE_SHA:
        raise ValueError("movie tables require the pinned original PC executable")
    # Import-only source locations. No source pointers or addresses enter runtime Content.
    clip_table, playlist_table = 0x10db8b0, 0x10db964
    raw_clips = image.read(clip_table, 9 * 16)
    raw_pointers = image.read(playlist_table, 9 * 4)
    pointers = struct.unpack("<9I", raw_pointers)
    if len(set(pointers)) != 9 or any(pointer < clip_table + 9 * 16 or pointer >= playlist_table
                                     for pointer in pointers):
        raise ValueError("unsupported movie playlist storage")
    clips, playlists = [], []
    inverse_clips = bytearray()
    string_bytes = bytearray()
    playlist_bytes = bytearray()
    ordered_pointers = sorted(pointers)
    for index in range(9):
        name_at, mode, fade, width, height = struct.unpack_from("<Iiihh", raw_clips, index * 16)
        source_path = image.string(name_at)
        resource = source_path.replace("\\", "/").split("/")[-1].upper()
        if resource != MOVIES[index] or width <= 0 or height <= 0:
            raise ValueError("unsupported movie resource declaration")
        clip = {"path": source_path, "asset_path": "movies/" + resource,
                "mode": mode, "fade_frame": fade, "width": width, "height": height}
        clips.append(clip)
        inverse_clips.extend(struct.pack("<Iiihh", name_at, clip["mode"], clip["fade_frame"],
                                         clip["width"], clip["height"]))
        encoded_name = clip["path"].encode("ascii") + b"\0"
        if image.read(name_at, len(encoded_name)) != encoded_name:
            raise ValueError("movie source string inverse mismatch")
        string_bytes.extend(encoded_name)
        pointer = pointers[index]
        position = ordered_pointers.index(pointer)
        end = ordered_pointers[position + 1] if position + 1 < len(ordered_pointers) else playlist_table
        original = image.read(pointer, end - pointer)
        values = []
        for byte in original:
            value, = struct.unpack("<b", bytes([byte]))
            if value not in (-1, -2) and not 0 <= value < 9:
                raise ValueError("unsupported signed movie playlist entry")
            values.append(value)
            if value == -1:
                break
        if not values or values[-1] != -1:
            raise ValueError("movie playlist has no source terminator")
        reconstructed = struct.pack(f"<{len(values)}b", *values) + bytes(len(original) - len(values))
        if reconstructed != original:
            raise ValueError("movie playlist inverse or padding mismatch")
        playlists.append(values)
        playlist_bytes.extend(reconstructed)
    if bytes(inverse_clips) != raw_clips or struct.pack("<9I", *pointers) != raw_pointers:
        raise ValueError("movie table byte inverse mismatch")
    configuration = configparser.ConfigParser(interpolation=None)
    settings_bytes = Path(source_ini).read_bytes()
    configuration.read_string(settings_bytes.decode("utf-8-sig"))
    sections = [name for name in configuration.sections() if name.casefold() == "sonicdx"]
    if len(sections) != 1:
        raise ValueError("movie settings require one sonicDX section")
    values = {}
    for source_key, native_key in (("bgm", "bgm_enabled"), ("bgmv", "bgm_volume_percent")):
        raw = configuration.get(sections[0], source_key, fallback=None)
        if raw is None:
            raise ValueError(f"missing movie setting: {source_key}")
        value = int(raw, 10)
        if not -(2**31) <= value < 2**31:
            raise ValueError(f"movie setting outside signed32 data domain: {source_key}")
        values[native_key] = value
    content = {"version": 1, "source_sha256": image.sha256, "clips": clips,
               "playlists": playlists, "configuration": values,
               "settings_sha256": hashlib.sha256(settings_bytes).hexdigest()}
    evidence = {"version": 1, "source_sha256": image.sha256,
                "inverse_clip_records_match": True, "inverse_playlist_pointers_match": True,
                "inverse_playlist_payloads_match": True, "inverse_source_strings_match": True,
                "clip_records_sha256": hashlib.sha256(raw_clips).hexdigest(),
                "playlist_pointers_sha256": hashlib.sha256(raw_pointers).hexdigest(),
                "playlist_payloads_sha256": hashlib.sha256(playlist_bytes).hexdigest(),
                "source_strings_sha256": hashlib.sha256(string_bytes).hexdigest()}
    return content, evidence


def import_content(game, output):
    content, evidence = movie_content(Path(game) / "sonic.exe", Path(game) / "sonicDX.ini")
    write_content(output, content, evidence)
    return content


def write_content(output, content, evidence):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    (output / "movie-content.json").write_text(json.dumps(content, indent=2) + "\n")
    (output / "movie-content-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def import_movies(game, output, names=MOVIES):
    system = Path(game) / "system"
    output = Path(output)
    files = {}
    for path in system.iterdir():
        if path.is_file():
            key = path.name.casefold()
            if key in files:
                raise ValueError(f"ambiguous Windows asset name: {path.name}")
            files[key] = path
    # Validate every requested source before creating any imported movie.
    sources = []
    for name in names:
        if name not in MOVIES:
            raise ValueError(f"movie is not in the pinned English profile: {name}")
        source = files.get(name.casefold())
        if source is None:
            raise ValueError(f"missing movie asset: {name}")
        if source.stat().st_size == 0:
            raise ValueError(f"empty movie asset: {name}")
        sources.append((name, source, digest(source)))
    movie_directory = output / "movies"
    movie_directory.mkdir(parents=True, exist_ok=True)
    clips = []
    for name, source, source_hash in sources:
        target = movie_directory / name
        if not target.exists() or digest(target) != source_hash:
            temporary = movie_directory / (name + ".importing")
            shutil.copyfile(source, temporary)
            if digest(temporary) != source_hash:
                raise ValueError(f"movie copy hash mismatch: {name}")
            temporary.replace(target)
        clips.append({"resource": name, "path": "movies/" + name,
                      "source_sha256": source_hash, "sha256": source_hash,
                      "bytes": target.stat().st_size})
    manifest = {"version": 1, "scope": "pinned English MPEG movie assets; byte-identical copies",
                "clips": clips}
    (output / "movie-assets.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_directory", type=Path)
    parser.add_argument("output_directory", type=Path)
    parser.add_argument("--clip", action="append", choices=MOVIES, dest="clips")
    parser.add_argument("--assets-only", action="store_true",
                        help="copy media without source-table/configuration import")
    args = parser.parse_args()
    content = None
    if not args.assets_only:
        content = movie_content(args.game_directory / "sonic.exe", args.game_directory / "sonicDX.ini")
    manifest = import_movies(args.game_directory, args.output_directory, args.clips or MOVIES)
    if content is not None:
        write_content(args.output_directory, *content)
    print(json.dumps({"movies": len(manifest["clips"]), "output": str(args.output_directory)}))


if __name__ == "__main__":
    main()
