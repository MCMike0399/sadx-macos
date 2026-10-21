#!/usr/bin/env python3
"""Run the required assets-only native checkpoints; missing prerequisites are failures."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def run(application, assets, output_root):
    application, assets, output_root = application.resolve(), assets.resolve(), output_root.resolve()
    if not application.is_file():
        raise RuntimeError(f'required native application is missing: {application}')
    if not assets.is_dir():
        raise RuntimeError(f'required imported native assets are missing: {assets}')
    snapshot = assets / 'saves'
    files = [path for path in snapshot.iterdir()
             if path.is_file() and re.fullmatch(r'SonicDX[0-9]{2}\.snc', path.name, re.IGNORECASE)] if snapshot.is_dir() else []
    if len(files) < 2:
        raise RuntimeError('required native FileSelect journey needs two distinct valid imported '
                           f'SonicDX saves in {snapshot}; no checkpoint was skipped')
    output_root.mkdir(parents=True, exist_ok=True)
    checkpoints = []
    for name, checker in (
            ('native-file-select', 'check_native_file_select.py'),
            ('native-main-menu', 'check_native_main_menu.py')):
        output = Path(tempfile.mkdtemp(prefix=name + '-', dir=output_root))
        command = [sys.executable, str(ROOT / 'tools/journey' / checker),
                   '--app', str(application), '--assets', str(assets), '--output', str(output)]
        result = subprocess.run(command, cwd=ROOT, check=False)
        checkpoints.append({'journey': name, 'exit_status': result.returncode,
                            'output': str(output), 'checker': 'tools/journey/' + checker})
        report = dict(checkpoints[0], checkpoints=checkpoints, exit_status=result.returncode)
        (output_root / 'native-journeys.json').write_text(json.dumps(report, indent=2) + '\n')
        if result.returncode != 0:
            return result.returncode
    return 0



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=ROOT / 'build/sadx-native')
    parser.add_argument('--assets', type=Path, default=ROOT / 'game/imported')
    parser.add_argument('--output-root', type=Path, default=ROOT / 'build')
    arguments = parser.parse_args()
    try:
        return run(arguments.app, arguments.assets, arguments.output_root)
    except (OSError, ValueError, RuntimeError) as error:
        print(f'Required native verification failed: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
