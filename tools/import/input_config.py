#!/usr/bin/env python3
"""Import raw pinned Input-mod INI settings; runtime owns conversion and polling."""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
# KeyboardMouse::update_keyboard_buttons comparison order, not a host key map.
KEYS = ("rightstick", "leftstick", "a", "b", "x", "y", "rightshoulder", "leftshoulder",
        "back", "start", "lefttrigger", "righttrigger", "dpup", "dpdown", "dpleft", "dpright",
        "-lefty", "+lefty", "-leftx", "+leftx", "-righty", "+righty", "-rightx", "+rightx")

def input_configuration(path):
    data = Path(path).read_bytes()
    ini = configparser.ConfigParser(interpolation=None)
    ini.read_string(data.decode("utf-8-sig"))
    def integer(section, key):
        value = ini.getint(section, key)
        if not -(1 << 31) <= value < (1 << 31):
            raise ValueError(f"Input {section}.{key} is outside signed32")
        return value
    def boolean(section, key):
        value = ini.get(section, key).strip().casefold()
        if value not in ("true", "false"):
            raise ValueError(f"Input {section}.{key} requires an explicit true/false literal")
        return value == "true"
    result = {"version": 1, "settings_sha256": hashlib.sha256(data).hexdigest(),
              "disable_mouse": boolean("Config", "DisableMouse"),
              "keyboard_player": integer("Config", "KeyboardPlayer"),
              "keyboard_keys": [integer("Keyboard", key) for key in KEYS],
              "controllers": []}
    for i in range(1, 9):
        section = f"Controller {i}"
        result["controllers"].append({
            "deadzone_left": integer(section, "DeadzoneL"),
            "deadzone_right": integer(section, "DeadzoneR"),
            "radial_left": boolean(section, "RadialL"),
            "radial_right": boolean(section, "RadialR"),
            "trigger_threshold": integer(section, "TriggerThreshold")})
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("settings",type=Path);parser.add_argument("output",type=Path)
    args=parser.parse_args();content=input_configuration(args.settings)
    args.output.write_text(json.dumps(content,indent=2)+"\n")
if __name__ == "__main__":
    main()
