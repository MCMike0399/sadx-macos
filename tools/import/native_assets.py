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
from audio_settings import audio_settings
from character_textures import import_character_textures
from sound_banks import import_banks
from title_menu import import_title_menu
from stage_lighting import import_stage_lighting
from native_settings import import_settings
from regular_textures import import_regular_textures
from menu_textures import import_menu_textures
from music_assets import import_music_assets
from voice_assets import import_voice_assets
from save_files import import_save_files
from font_assets import import_font_assets
from menu_voice import import_menu_voice
from file_select_display import import_file_select_display
from menu_dialog import import_menu_dialog
from sound_effects import import_sound_effects


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
    textures = sorted((path for path in files.values() if path.suffix.casefold() in (".pvm", ".pvr")),
                      key=lambda path: path.name.casefold())
    if "segalogo_e.pvm" not in files:
        raise ValueError("missing SEGALOGO_E.PVM")
    output.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(Path(__file__).with_name("character_actions.py")),
                    str(model), str(output / "characters/sonic-startup.actions")], check=True)
    subprocess.run([sys.executable, str(Path(__file__).with_name("character_actions.py")),
                    str(model), str(output / "characters/title.actions"), "--title"], check=True)
    import_character_textures(game, output)
    import_title_menu(game, output)
    import_stage_lighting(game, output)
    import_settings(game, output)
    import_regular_textures(game, output)
    import_menu_textures(game, output)
    import_font_assets(game, output)
    import_menu_voice(game, output)
    import_file_select_display(game, output)
    import_menu_dialog(game, output)
    import_music_assets(game, output, selected=(102, 72), menu_request_count=3)
    import_voice_assets(game, output)
    import_save_files(game, output)
    import_banks(game / "sonic.exe", system / "SoundData/SE", output,
                 selected=(42, 36, 1, 71, 72),
                 mod_directories=(game / "mods/sadx-input-mod", game / "mods/sadx-speedrun-mod"))
    import_sound_effects(game, output)
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
    audio = audio_settings(game / "sonicDX.ini")
    (output / "audio-configuration.json").write_text(json.dumps(audio, indent=2) + "\n")
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
