#!/usr/bin/env python3
"""Import effective language settings from the pinned loader's selected profile."""
import hashlib
import json
import configparser
import re
from pathlib import Path


def import_settings(game, output):
    game, output = Path(game), Path(output)
    profiles = game / 'mods/.modloader/profiles'
    selection_path = profiles / 'Profiles.json'
    selection = json.loads(selection_path.read_text())
    index = selection.get('ProfileIndex', 0)
    if type(index) is not int or not 0 <= index < len(selection['ProfilesList']):
        raise ValueError('invalid selected loader profile index')
    filename = selection['ProfilesList'][index]['Filename']
    if not isinstance(filename, str) or Path(filename).name != filename:
        raise ValueError('invalid selected loader profile filename')
    profile_path = profiles / filename
    profile = json.loads(profile_path.read_text())
    settings = profile.get('TestSpawn', {})
    result = {'version': 1}
    for source, name in (('GameTextLanguage', 'text_language'), ('GameVoiceLanguage', 'voice_language')):
        value = settings.get(source, 1)
        if type(value) is not int or not -2147483648 <= value <= 2147483647:
            raise ValueError('invalid loader language integer')
        result[name] = value
    result['profile_sha256'] = hashlib.sha256(profile_path.read_bytes()).hexdigest()
    result['selection_sha256'] = hashlib.sha256(selection_path.read_bytes()).hexdigest()
    ini_path = game / 'sonicDX.ini'
    ini = configparser.ConfigParser(interpolation=None)
    if ini_path.exists():
        ini.read_string(ini_path.read_text(encoding='utf-8-sig'))
    sections = [name for name in ini.sections() if name.casefold() == 'sonicdx']
    for key, name, fallback in (('framerate', 'frame_rate_setting', '2'),
                                ('fogemulation', 'fog_emulation_setting', '0')):
        text = ini.get(sections[0], key, fallback=fallback) if sections else fallback
        if not re.fullmatch(r'[+-]?[0-9]+', text.strip()):
            raise ValueError(f'{key} requires a decimal source integer')
        value = int(text)
        if not -(1 << 31) <= value < (1 << 31):
            raise ValueError(f'{key} exceeds signed32')
        result[name] = value
    result['settings_sha256'] = hashlib.sha256(ini_path.read_bytes()).hexdigest() if ini_path.exists() else None
    output.mkdir(parents=True, exist_ok=True)
    (output / 'native-settings.json').write_text(json.dumps(result, indent=2) + '\n')
    return result
