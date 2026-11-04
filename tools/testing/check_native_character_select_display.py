#!/usr/bin/env python3
"""Normal-input CharacterSelect display checkpoint, with an explicit open boundary mode.

Default mode requires drawn reference content. Boundary mode verifies a named stop
and reports the drawn goal separately; it does not certify N3b or enter the gate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/journey'))
from capture_content import read_capture, check_content
from private_native_assets import reject_links

TITLE_SHA256 = 'c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427'
KEYS = ('500:13:down,501:13:up,750:13:down,751:13:up,950:13:down,951:13:up,'
        '1050:13:down,1051:13:up,1300:13:down,1301:13:up')
SAVE_RECEIPT = ROOT / 'reference/production-integration-20261102/sonic-initialization-fan-source-only.json'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def drawn_content(path):
    read_capture(path)  # Full payload and nonuniform frame, not just a PPM header.
    require(hashlib.sha256(path.read_bytes()).hexdigest() != TITLE_SHA256,
            'outgoing title image is not a CharacterSelect display frame')
    # Committed reference R3 signature, provenance retained in capture-content.json.
    # This is a screen-level check; first-Gamma pose alignment remains an integration limit.
    check_content(path, 'reference/character-select')


def semantic_state(state):
    require(state['execution'] == 'native', 'native execution required')
    require(state['game_mode'] == 12 and state['game_state'] == 21,
            'CharacterSelect mode/state differs')
    require(state['selected_save_name'] is None and state['active_save_name'] == 'SonicDX02.snc'
            and state['selected_save'] == state['menu_selected_save'] == 1,
            'verified slot 1 save was not selected and activated')
    require(state['fallback_save_name'] is None, 'unexpected fallback save')
    require(state['gamma_setup_assets_loaded'] and state['gamma_setup_registered'],
            'canonical Gamma setup missing')
    require(not state.get('tasks_error') and isinstance(state['tasks'], list),
            'canonical scheduled task graph unavailable')
    tasks = {task['path']: task for task in state['tasks']}
    # Paths/flags from the retained fbc1eec normal-input observation; see PACKET.md.
    for path, flags in EXPECTED_TASKS.items():
        require(path in tasks and all(tasks[path][key] == value for key, value in flags.items()),
                f'scheduled task {path} flags differ')
    require(not state['preview_task_retired'] and state['preview_camera_live'],
            'selection preview or camera retired')
    require(len(state['preview_actors']) == 7, 'selection actor count differs')
    gamma = state['preview_actors'][5]
    require(gamma and gamma.get('character') == 6 and gamma.get('id') == 2 and
            gamma.get('mode') == 1 and gamma.get('player_index') == 5 and
            not gamma.get('retired'), 'initialized Gamma preview actor missing')


# Selection preview is list 3; actor callbacks are list 1. Gamma is 1/2 at this stop.
EXPECTED_TASKS = {**{f'1/{index}': {'exec': True} for index in range(1, 8)},
                  '1/2': {'exec': True, 'display': True, 'destructor': True},
                  '3/0': {'exec': True, 'display': True, 'destructor': True},
                  '0/0': {'exec': True}}


def run(args):
    output = args.output.absolute()
    require(not output.exists(), 'output must be a new run directory')
    assets = args.assets.resolve(strict=True)
    require((assets / '.private-native-assets.json').is_file(), 'private asset-tree proof missing')
    reject_links(assets)
    output.mkdir(parents=True)
    saves = output / 'savedata'
    saves.mkdir()
    expected = json.loads(SAVE_RECEIPT.read_text())['save_input']
    for name, digest in expected.items():
        source = args.save_source / name
        require(not source.is_symlink() and hashlib.sha256(source.read_bytes()).hexdigest() == digest,
                f'verified source save differs: {name}')
        shutil.copy2(source, saves / name)
        require(hashlib.sha256((saves / name).read_bytes()).hexdigest() == digest,
                f'private save copy differs: {name}')
    (output / 'initial-save-hashes.json').write_text(json.dumps(expected, indent=2) + '\n')
    capture = output / 'frontier.ppm'
    command = [str(args.app.resolve(strict=True)), '--assets', str(assets), '--offscreen', '--paced',
               '--exit-after', str(args.frame), '--save-directory', str(saves),
               '--controller-library', str(args.controller_library.resolve(strict=True)),
               '--key-events', KEYS, '--screenshot', str(capture)]
    (output / 'argv.json').write_text(json.dumps(command, indent=2) + '\n')
    with (output / 'stdout.log').open('w') as stdout, (output / 'stderr.log').open('w') as stderr:
        result = subprocess.run(command, stdout=stdout, stderr=stderr, timeout=120)
    states = [json.loads(line) for line in (output / 'stdout.log').read_text().splitlines()
              if line.startswith('{') and '"execution":"native"' in line]
    error_log = (output / 'stderr.log').read_text()
    stops = [line for line in error_log.splitlines() if 'native session stopped:' in line]
    report = {'command': command, 'exit': result.returncode,
              'state': states[-1] if states else None,
              'frame': states[-1]['frames'] if states else None, 'stop': stops[-1] if stops else None,
              'screenshot_sha256': hashlib.sha256(capture.read_bytes()).hexdigest() if capture.exists() else None,
              'mode': 'boundary-only' if args.expected_boundary else 'drawn-goal',
              'drawn_goal_passed': False, 'passed': False}
    try:
        require(bool(states), 'native final state missing')
        semantic_state(states[-1])
        report['semantic_state_passed'] = True
        try:
            drawn_content(capture)
            report['drawn_goal_passed'] = True
        except (ValueError, OSError) as error:
            report['drawn_goal_error'] = str(error)
        if args.expected_boundary:
            require(result.returncode == args.expected_exit and states[-1]['frames'] == args.expected_frame,
                    'observed exit/frame differs from expected boundary')
            require(report['stop'] == 'native session stopped: ' + args.expected_boundary,
                    'observed stop differs from expected boundary')
            # Current open boundary has a known exact outgoing image, not a header-only smoke check.
            read_capture(capture)
            require(report['screenshot_sha256'] == TITLE_SHA256,
                    'boundary capture differs from the pinned outgoing title')
        else:
            require(report['drawn_goal_passed'], report.get('drawn_goal_error', 'drawn goal failed'))
            require(result.returncode == args.expected_exit, 'unexpected application exit')
            require(states[-1]['frames'] == args.frame, 'requested CharacterSelect frame not reached')
        report['passed'] = True
    except (ValueError, KeyError, OSError) as error:
        report['error'] = str(error)
    (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: value for key, value in report.items() if key not in ('command', 'state')}, indent=2))
    return 0 if report['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=ROOT / 'build/sadx-native')
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--save-source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--controller-library', type=Path,
                        default=ROOT / 'build/native-controller-deps/lib/libSDL3.0.dylib')
    parser.add_argument('--frame', type=int, default=1550)
    parser.add_argument('--expected-boundary')
    parser.add_argument('--expected-frame', type=int, default=1332)
    parser.add_argument('--expected-exit', type=int, default=0)
    args = parser.parse_args()
    try:
        return run(args)
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
