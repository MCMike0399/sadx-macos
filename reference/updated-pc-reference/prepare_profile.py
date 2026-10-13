"""Materialize the pinned local Windows reference without executing PE code."""
from pathlib import Path
import configparser
import hashlib
import json
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[2]
SUPPLIED = REPO / "Sonic Adventure DX"
GAME = ROOT / "game"
EXPECTED_SONIC = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"

def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

def write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() != content:
        raise RuntimeError(f"Refusing to replace differing reference file: {path}")
    if not path.exists():
        path.write_bytes(content)

def copy(source, destination):
    if source.is_symlink():
        raise RuntimeError(f"Unexpected source symlink: {source}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        if sha(source) != sha(destination):
            raise RuntimeError(f"Refusing to replace differing reference file: {destination}")
    else:
        shutil.copyfile(source, destination)

def dump(path, value):
    write(path, (json.dumps(value, indent=2, ensure_ascii=False) + "\n").encode())

def ini(path, value):
    lines = []
    for section, fields in value.items():
        lines.append(f"[{section}]")
        lines.extend(f"{key}={field}" for key, field in fields.items())
        lines.append("")
    write(path, ("\n".join(lines) + "\n").encode())

def inventory(directory):
    return [{"path": p.relative_to(ROOT).as_posix(), "size": p.stat().st_size,
             "sha256": sha(p)} for p in sorted(directory.rglob("*")) if p.is_file()]

assert sha(SUPPLIED / "sonic.exe") == EXPECTED_SONIC

# Copy selected game inputs, excluding legacy mods, launchers and managers.
for source in sorted(SUPPLIED.rglob("*")):
    if not source.is_file():
        continue
    relative = source.relative_to(SUPPLIED)
    if relative.parts[0] == "mods":
        continue
    if len(relative.parts) == 1 and relative.name not in ("sonic.exe", "sonicDX.ini"):
        continue
    if relative.as_posix().lower() == "system/chrmodels.dll":
        continue
    copy(source, GAME / relative)

loader = ROOT / "unpacked/loader"
copy(loader / "SADXModLoader.dll", GAME / "system/CHRMODELS.dll")
for source in sorted(loader.rglob("*")):
    if source.is_file():
        copy(source, GAME / "mods/.modloader" / source.relative_to(loader))
for label, folder in (("input", "sadx-input-mod"), ("speedrun", "sadx-speedrun-mod")):
    origin = ROOT / "unpacked" / label / folder
    for source in sorted(origin.rglob("*")):
        if source.is_file():
            copy(source, GAME / "mods" / folder / source.relative_to(origin))

patch_catalog = json.loads((loader / "Patches.json").read_text())
patches = {entry["Name"]: False for entry in patch_catalog["Patches"]}
graphics = {
    "SelectedScreen": 0, "HorizontalResolution": 640, "VerticalResolution": 480,
    "Enable43ResolutionRatio": False, "EnableVsync": False, "Antialiasing": 0,
    "Anisotropic": 0, "EnablePauseOnInactive": True,
    "CustomWindowWidth": 640, "CustomWindowHeight": 480,
    "EnableKeepResolutionRatio": False, "EnableResizableWindow": False,
    "FillModeBackground": 2, "FillModeFMV": 1, "RenderBackend": 0,
    "EnableUIScaling": False, "EnableForcedMipmapping": False,
    "EnableForcedTextureFilter": False, "ScreenMode": 0,
    "ShowMouseInFullscreen": False, "DisableBorderImage": True,
}
profile = {
    "SettingsVersion": 3, "Graphics": graphics,
    "Controller": {"EnabledInputMod": False},
    "Sound": {"EnableBassMusic": False, "EnableBassSFX": False, "SEVolume": 100},
    "EnabledGamePatches": [], "Patches": patches,
    "DebugSettings": {"EnableDebugConsole": False, "EnableDebugScreen": False,
                      "EnableDebugFile": False, "EnableDebugCrashLog": False},
    "TestSpawn": {"GameTextLanguage": 1, "GameVoiceLanguage": 1},
    "EnabledMods": ["sadx-input-mod", "sadx-speedrun-mod"],
}
profiles = {"ProfileIndex": 0, "ProfilesList": [{"Filename": "updated-speedrun.json"}]}
dump(GAME / "mods/.modloader/profiles/Profiles.json", profiles)
dump(GAME / "mods/.modloader/profiles/updated-speedrun.json", profile)

keyboard = {"-lefty": 38, "+lefty": 40, "-leftx": 37, "+leftx": 39,
            "-righty": 73, "+righty": 77, "-rightx": 74, "+rightx": 76,
            "a": 88, "b": 90, "x": 65, "y": 83, "start": 13, "back": 86,
            "lefttrigger": 81, "righttrigger": 87, "leftshoulder": 67,
            "rightshoulder": 66, "leftstick": 69, "rightstick": 160,
            "dpup": 104, "dpdown": 98, "dpleft": 100, "dpright": 102}
input_config = {"Config": {"Debug": "false", "DisableMouse": "true", "KeyboardPlayer": 0},
                "Keyboard": keyboard}
for slot in range(1, 9):
    input_config[f"Controller {slot}"] = {
        "DeadzoneL": 7849, "DeadzoneR": 8689, "RadialL": "true", "RadialR": "false",
        "TriggerThreshold": 30, "RumbleFactor": 1.0, "MegaRumble": "false", "RumbleMinTime": 0,
    }
ini(GAME / "mods/sadx-input-mod/config.ini", input_config)
speedrun_config = {
    "QuickSaveSettings": {"Enabled": "false", "ForceLoad": "false", "PremadeFile": "Custom",
                          "SaveFilePath": "", "SaveNum": 99},
    "OtherSettings": {"CCEF": "true", "FreeCam": "false", "Skipma": "true", "FrameLimit": "63FPS"},
}
ini(GAME / "mods/sadx-speedrun-mod/config.ini", speedrun_config)

# The isolated copy never inherits old Codes.dat/Patches.dat. Their absence,
# rather than an invented empty binary serialization, disables those code paths.
for relative in ("mods/Codes.dat", "mods/Patches.dat", "mods/.modloader/Codes.dat",
                 "mods/.modloader/Patches.dat"):
    if (GAME / relative).exists():
        raise RuntimeError(f"Unexpected executable code file: {relative}")

loader_commit = "eb4cdcca9637d611a98b910fcfed6ce18580451c"
for filename in ("config.cpp", "dllmain.cpp", "backend.cpp", "window.cpp", "direct3d.cpp",
                 "testspawn.cpp", "InterpolationFixes.cpp", "TextureReplacement.cpp",
                 "Events.cpp", "hudscale.cpp", "version.h"):
    data = subprocess.check_output(["git", "-C", str(REPO / "sadx-mod-loader"), "show",
                                    f"{loader_commit}:SADXModLoader/{filename}"])
    write(ROOT / "sources/loader" / filename, data)
for filename in ("speedrun-mod.cpp", "speedrun-frame.cpp", "skipma.cpp", "input-mod.cpp",
                 "input-keyboard.cpp", "input-keyboard.h", "input-dreampad.cpp", "input-dreampad.h"):
    copy(Path("/private/tmp/sadx-mod-findings") / filename, ROOT / "sources" / filename)

game_inventory = inventory(GAME)
dump(ROOT / "game-inventory.json", game_inventory)
artifacts = []
for label, asset_name, commit in (
    ("loader", "SADXModLoader.7z", loader_commit),
    ("input", "sadx-input-mod.7z", "68e2a91bec4a55967674c1420ecd9abf89bcd0ce"),
    ("speedrun", "sadx-speedrun-mod.zip", "1833a11fccb93b96c15eb044d4c8b628039e6f8a"),
):
    release_path = ROOT / "releases" / f"{label}.json"
    release = json.loads(release_path.read_text())
    asset = next(item for item in release["assets"] if item["name"] == asset_name)
    archive = ROOT / "archives" / asset_name
    observed_sha = sha(archive)
    if asset.get("digest") and asset["digest"] != "sha256:" + observed_sha:
        raise RuntimeError("Published digest mismatch")
    artifacts.append({"component": label, "release_id": release["id"], "tag": release["tag_name"],
                      "published_at": release["published_at"], "source_revision": commit,
                      "asset_id": asset["id"], "asset_updated_at": asset["updated_at"],
                      "url": asset["browser_download_url"], "archive": str(archive.relative_to(ROOT)),
                      "archive_size": archive.stat().st_size, "sha256": observed_sha,
                      "upstream_digest": asset.get("digest"), "release_metadata_sha256": sha(release_path),
                      "unpacked_inventory_sha256": sha(ROOT / "unpacked" / f"{label}-inventory.json")})

raw_config = []
for relative in ("sonicDX.ini", "mods/.modloader/profiles/Profiles.json",
                 "mods/.modloader/profiles/updated-speedrun.json", "mods/sadx-input-mod/config.ini",
                 "mods/sadx-speedrun-mod/config.ini"):
    raw_config.append({"path": "game/" + relative, "sha256": sha(GAME / relative)})

manifest = {
    "format_version": 1, "profile_kind": "intended-updated-windows-pc-reference",
    "prepared_on": "2026-10-11", "state": "prepared-not-executed",
    "execution_observed": False, "modules_loaded_observed": False,
    "community_acceptance_verified": False,
    "reference_definition": "Supplied sonic.exe plus loader 676, external Input 2.6.1 and Speedrun 2.0 with selected 63FPS policy",
    "base": {"supplied_path": str(SUPPLIED), "sonic_sha256": EXPECTED_SONIC,
             "sonic_size": (GAME / "sonic.exe").stat().st_size,
             "original_chrmodels_sha256": sha(GAME / "system/CHRMODELS_orig.dll"),
             "original_loader_replaced_only_in_reference": True},
    "launch": {"cwd": "game", "executable": "sonic.exe", "arguments": []},
    "artifacts": artifacts, "loader_profile": profile, "input_config": input_config,
    "speedrun_config": speedrun_config, "raw_configs": raw_config,
    "enabled_mod_order": profile["EnabledMods"],
    "owners": {"input": "external Input Mod 2.6.1; built-in loader input disabled",
               "camera_fix": "Speedrun Mod 2.0; loader KeepCamSettings disabled",
               "egg_hornet_fix": "Speedrun Mod 2.0 unconditional",
               "skipma": "Speedrun Mod 2.0 enabled",
               "frame_limit": "Speedrun Mod 2.0 FrameLimit=63FPS; no standalone limiter"},
    "absent_executable_code_files": ["mods/Codes.dat", "mods/Patches.dat",
                                    "mods/.modloader/Codes.dat", "mods/.modloader/Patches.dat"],
    "additional_mods_enabled": [], "game_inventory_sha256": sha(ROOT / "game-inventory.json"),
    "source_inventory": inventory(ROOT / "sources"),
    "runtime_verification_required": ["native Windows process and thread identity", "actual loaded module paths/digests",
                                      "effective profile and mod Init completion", "loaded hook bytes and destinations",
                                      "x87 control/status, MXCSR and rounding state", "simulation tick/frame multiplier/input/timing traces",
                                      "keyboard/controller physical transport behavior", "render/audio/device configuration and category fixture saves"],
}
dump(ROOT / "manifest.json", manifest)
write(ROOT / "manifest.sha256", (sha(ROOT / "manifest.json") + "  manifest.json\n").encode())
assert sha(SUPPLIED / "sonic.exe") == EXPECTED_SONIC
print(json.dumps({"manifest_sha256": sha(ROOT / "manifest.json"),
                  "game_file_count": len(game_inventory), "source_sonic_unchanged": True,
                  "state": manifest["state"]}))
