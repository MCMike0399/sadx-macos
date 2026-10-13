"""Verify retained Windows reference bytes/settings, without executing them."""
from pathlib import Path
import configparser
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[2]
sys.path.insert(0, str(REPO / "sadx-native/tools"))
from pe_image import PEImage

def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

manifest = json.loads((ROOT / "manifest.json").read_text())
assert (ROOT / "manifest.sha256").read_text().split()[0] == sha(ROOT / "manifest.json")
assert sha(ROOT / "game-inventory.json") == manifest["game_inventory_sha256"]
inventory = json.loads((ROOT / "game-inventory.json").read_text())
expected_paths = {entry["path"] for entry in inventory}
actual_paths = {p.relative_to(ROOT).as_posix() for p in (ROOT / "game").rglob("*") if p.is_file()}
assert actual_paths == expected_paths, "unexpected/missing reference game files"
for entry in inventory + manifest["source_inventory"]:
    path = ROOT / entry["path"]
    assert not path.is_symlink() and path.stat().st_size == entry["size"] and sha(path) == entry["sha256"], path
for artifact in manifest["artifacts"]:
    assert sha(ROOT / artifact["archive"]) == artifact["sha256"]
for entry in manifest["raw_configs"]:
    assert sha(ROOT / entry["path"]) == entry["sha256"]

game = ROOT / "game"
assert sha(game / "sonic.exe") == manifest["base"]["sonic_sha256"]
assert sha(REPO / "Sonic Adventure DX/sonic.exe") == manifest["base"]["sonic_sha256"]
assert sha(game / "system/CHRMODELS.dll") == sha(ROOT / "unpacked/loader/SADXModLoader.dll")
assert sha(game / "system/CHRMODELS_orig.dll") == manifest["base"]["original_chrmodels_sha256"]
for relative in manifest["absent_executable_code_files"]:
    assert not (game / relative).exists(), relative

profiles = json.loads((game / "mods/.modloader/profiles/Profiles.json").read_text())
profile_file = profiles["ProfilesList"][profiles["ProfileIndex"]]["Filename"]
profile = json.loads((game / "mods/.modloader/profiles" / profile_file).read_text())
assert profile == manifest["loader_profile"]
assert profile["EnabledMods"] == ["sadx-input-mod", "sadx-speedrun-mod"]
assert profile["Controller"]["EnabledInputMod"] is False
assert all(value is False for value in profile["Patches"].values())
assert profile["EnabledGamePatches"] == [] and profile["Graphics"]["EnableVsync"] is False

def parsed_ini(path):
    config = configparser.ConfigParser(interpolation=None)
    config.optionxform = str
    config.read(path)
    result = {}
    for section in config.sections():
        result[section] = {}
        for key, raw in config.items(section):
            if raw.lower() in ["true", "false"]:
                value = raw.lower() == "true"
            elif raw == "":
                value = ""
            else:
                try: value = int(raw)
                except ValueError:
                    try: value = float(raw)
                    except ValueError: value = raw
            result[section][key] = value
    return result

input_config = parsed_ini(game / "mods/sadx-input-mod/config.ini")
speedrun = parsed_ini(game / "mods/sadx-speedrun-mod/config.ini")
assert input_config["Config"] == {"Debug": False, "DisableMouse": True, "KeyboardPlayer": 0}
assert speedrun["OtherSettings"] == {"CCEF": True, "FreeCam": False, "Skipma": True, "FrameLimit": "63FPS"}
assert speedrun["QuickSaveSettings"]["Enabled"] is False and speedrun["QuickSaveSettings"]["ForceLoad"] is False
effective = {"format_version": 1, "evidence_type": "static source-bound interpretation of explicit raw config",
             "manifest_sha256": sha(ROOT / "manifest.json"), "execution_observed": False,
             "loader_profile_path": "game/mods/.modloader/profiles/" + profile_file,
             "loader": profile, "input": input_config, "speedrun": speedrun,
             "frame_limit_integer": 63, "frame_limit_parse": "std::stoi(63FPS) consumes decimal prefix; does not require full-string consumption"}
(ROOT / "effective-config.json").write_text(json.dumps(effective, indent=2) + "\n")

exe = PEImage(game / "sonic.exe")
assert exe.read(0x789e50, 5) == bytes.fromhex("83ec285733"), "loader US-version check would fail"
expected_hooks = [
    (0x434870, "camera reset 1", "810d", "Speedrun Mod", None),
    (0x438330, "camera reset 2", "810d", "Speedrun Mod", None),
    (0x533939, "Egg Hornet dereference bypass", "909090", "Speedrun Mod", None),
    (0x440ef5, "Egg Hornet conditional bypass", "9090", "Speedrun Mod", None),
    (0x431517, "in-game Skipma jump", "e9", "Speedrun Mod", "sadx-speedrun-mod.dll"),
    (0x5132b1, "movie Skipma jump", "e9", "Speedrun Mod", "sadx-speedrun-mod.dll"),
    (0x7899e0, "frame limiter trampoline", "e9", "Speedrun Mod", "sadx-speedrun-mod.dll"),
    (0x411d1c, "extra tick policy jump", "e9", "Speedrun Mod", "sadx-speedrun-mod.dll"),
    (0x7dce58, "frame rate scalar 63.0f", "00007c42", "Speedrun Mod", None),
    (0x40f343, "analog input hook", "e9", "Input Mod", "sadx-input-mod.dll"),
    (0x40f451, "raw controller init hook", "e9", "Input Mod", "sadx-input-mod.dll"),
    (0x40f460, "skip vanilla controller update", "e9", "Input Mod", "sonic.exe:0x40fdb3"),
    (0x40fdb3, "input event dispatch", "e9", "Mod Loader", "CHRMODELS.dll"),
]
hooks = {"evidence_type": "expected static source write set, not observed live writes", "observed_live": False,
         "entries": [{"va": hex(va), "operation": name, "unmodified_source_bytes": exe.read(va, max(5, len(expected)//2)).hex(),
                       "expected_live_prefix": expected, "owner": owner, "required_target_module": target}
                      for va, name, expected, owner, target in expected_hooks]}
(ROOT / "expected-hooks.json").write_text(json.dumps(hooks, indent=2) + "\n")
print(json.dumps({"state": "static-reference-verified-not-executed", "verified_game_files": len(inventory),
                  "manifest_sha256": sha(ROOT / "manifest.json"), "enabled_mods": profile["EnabledMods"], "frame_limit": 63}))
