#!/usr/bin/env python3
"""Import native startup character data and PVM assets from the user's PC install.

This is the current importer slice, not a complete level/audio importer.
Original executables and model DLLs are never copied to the output.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

from character_actions import SOURCE_SHA
from movie_assets import import_content, import_movies
from input_config import input_configuration


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def import_assets(game, output):
    system = game / "system"
    files = {path.name.casefold(): path for path in system.iterdir() if path.is_file()}
    model = None
    for name in ("chrmodels_orig.dll", "chrmodels.dll"):
        candidate = files.get(name)
        if candidate and digest(candidate) == SOURCE_SHA:
            model = candidate
            break
    if model is None:
        raise ValueError("this install has no supported original character archive")
    textures = sorted((path for path in files.values() if path.suffix.casefold() == ".pvm"),
                      key=lambda path: path.name.casefold())
    if "segalogo_e.pvm" not in files:
        raise ValueError("missing SEGALOGO_E.PVM")
    output.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(Path(__file__).with_name("character_actions.py")),
                    str(model), str(output / "characters/sonic-startup.actions")], check=True)
    texture_directory = output / "system"
    texture_directory.mkdir(exist_ok=True)
    manifest = {"version": 1, "scope": "startup character actions and PVM textures",
                "model_source_sha256": SOURCE_SHA, "textures": []}
    for source in textures:
        # Windows lookup is case-insensitive; native resource names use the original uppercase form.
        target = texture_directory / source.name.upper()
        source_hash = digest(source)
        if not target.exists() or digest(target) != source_hash:
            shutil.copyfile(source, target)
        if digest(target) != source_hash:
            raise ValueError(f"copied asset hash mismatch: {source.name}")
        manifest["textures"].append({"path": str(target.relative_to(output)),
                                     "sha256": source_hash, "bytes": target.stat().st_size})
    (output / "startup-assets.json").write_text(json.dumps(manifest, indent=2) + "\n")
    import_content(game, output)
    import_movies(game, output)
    configuration = input_configuration(game / "mods/sadx-input-mod/config.ini")
    (output / "input-configuration.json").write_text(json.dumps(configuration, indent=2) + "\n")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_directory", type=Path)
    parser.add_argument("output_directory", type=Path)
    args = parser.parse_args()
    manifest = import_assets(args.game_directory, args.output_directory)
    print(json.dumps({"textures": len(manifest["textures"]),
                      "output": str(args.output_directory)}))


if __name__ == "__main__":
    main()
