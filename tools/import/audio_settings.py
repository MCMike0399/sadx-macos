#!/usr/bin/env python3
"""Import raw SADX audio settings. Source conversion remains native game behavior."""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import re

def audio_settings(path):
    data = Path(path).read_bytes()
    ini = configparser.ConfigParser(interpolation=None)
    ini.read_string(data.decode('utf-8-sig'))
    sections = [section for section in ini.sections() if section.casefold() == 'sonicdx']
    if len(sections) != 1:
        raise ValueError('Audio settings require exactly one sonicDX section')
    section = sections[0]
    def integer(key):
        text = ini.get(section, key).strip()
        if not re.fullmatch(r'[+-]?[0-9]+', text):
            raise ValueError(f'Audio setting {key} requires a decimal source integer')
        value = int(text)
        if not -(1 << 31) <= value < (1 << 31):
            raise ValueError(f'Audio setting {key} is outside signed32')
        return value
    return {'version': 1, 'settings_sha256': hashlib.sha256(data).hexdigest(),
            'bgm_enabled': integer('bgm'), 'bgm_volume_percent': integer('bgmv'),
            'voice_enabled': integer('sevoice'), 'voice_volume_percent': integer('voicev')}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('settings', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(json.dumps(audio_settings(args.settings), indent=2) + '\n')
if __name__ == '__main__':
    main()
