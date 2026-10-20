#!/usr/bin/env python3
"""Replay normal Sonic story inputs from a fresh save, retaining local evidence.

This is the guest reference, not native game completion. Route files contain a
JSON list of {"verb": ..., "args": [...]} commands. Only ordinary input/time
commands, screenshots, and read-only state assertions are accepted. An optional
stdin exploration session records commands in the same format for later replay.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import time

from check_gameplay import Scenario
from sadx_driver import HarnessError, Session, REFERENCE_GAME_DIR


INPUT_COMMANDS = {
    'advance', 'press', 'key_down', 'key_up', 'pad', 'stick',
    'right_stick', 'triggers', 'release',
}


def execute(scenario, entry, max_frames):
    verb, args = entry['verb'], entry.get('args', [])
    if verb == 'capture':
        name = args[0]
        if not name or Path(name).name != name:
            raise ValueError('capture name must be one filename component')
        return scenario.capture(name)
    if verb in ('expect', 'expect-boss'):
        name, expected = args
        state = scenario.game.state()
        owner = state if verb == 'expect' else state.get('boss')
        observed = {key: owner.get(key) for key in expected} if isinstance(owner, dict) else None
        scenario.check(name, observed == expected, frame=state['frame'],
                       expected=expected, observed=observed)
        return state
    if verb not in INPUT_COMMANDS:
        raise ValueError(f'unsupported story command: {verb}')
    if verb == 'advance':
        if len(args) != 1 or not isinstance(args[0], int) or args[0] <= 0:
            raise ValueError('advance requires a positive integer')
        if scenario.game.frame + args[0] > max_frames:
            raise HarnessError(f'route exceeds frame budget {max_frames}')
    scenario.command(verb, *args)
    return scenario.game.state()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--route', type=Path, help='normal-input JSON commands after Chaos entry')
    parser.add_argument('--explore', action='store_true', help='read additional JSON commands from stdin')
    parser.add_argument('--app', default='build/sadx-app')
    parser.add_argument('--max-frames', type=int, default=20000)
    parser.add_argument('--timeout', type=int, default=600)
    args = parser.parse_args()
    if args.out.exists():
        parser.error('use a new output directory so save writes cannot contaminate a replay')
    if args.max_frames < 3000:
        parser.error('--max-frames must allow at least the fresh-save entry route (3000)')
    entries = json.loads(args.route.read_text()) if args.route else []
    if not isinstance(entries, list):
        parser.error('route must contain a JSON command list')
    args.out.mkdir(parents=True)
    report = {'engine': 'guest-reference', 'task_execution': 'native-task-adapter',
              'normal_inputs_only': True, 'passed': False, 'commands': [], 'checks': [],
              'snapshots': [], 'route_commands': [],
              'limits': ['No native game completion or Windows parity claim.',
                         'Only explicitly asserted checkpoints are verified.']}
    game_dir = Path(REFERENCE_GAME_DIR)
    sources = [Path(args.app), Path(__file__), Path(__file__).with_name('check_gameplay.py'),
               Path(__file__).with_name('sadx_driver.py'),
               game_dir / 'sonic.exe', game_dir / 'savedata/SonicDX01.snc',
               game_dir / 'mods/.modloader/profiles/updated-speedrun.json',
               game_dir / 'mods/.modloader/SADXModLoader.dll',
               game_dir / 'mods/sadx-input-mod/sadx-input-mod.dll',
               game_dir / 'mods/sadx-input-mod/config.ini',
               game_dir / 'mods/sadx-speedrun-mod/sadx-speedrun-mod.dll',
               game_dir / 'mods/sadx-speedrun-mod/config.ini']
    if args.route:
        sources.append(args.route)
    report['input_sha256'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                              if path.is_file() else None
                              for path in sources}
    started = time.perf_counter()
    try:
        with Session(app=args.app, log_path=str(args.out / 'app.log'),
                     overlay=args.out / 'overlay', timeout=args.timeout) as game:
            scenario = Scenario(game, args.out, report)
            try:
                entry = scenario.fresh_adventure_route()
                scenario.verify_playable_profile(entry)
                scenario.check('chaos-zero-entry', entry['stage'] == 15, stage=entry['stage'])
                for command in entries:
                    report['route_commands'].append(command)
                    execute(scenario, command, args.max_frames)
                if args.explore:
                    print('READY: enter one JSON command per line; {"verb":"quit"} ends.', flush=True)
                    for line in sys.stdin:
                        if not line.strip():
                            continue
                        command = json.loads(line)
                        if command.get('verb') == 'quit':
                            break
                        report['route_commands'].append(command)
                        print(json.dumps(execute(scenario, command, args.max_frames)), flush=True)
                        (args.out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
                scenario.capture('route-final')
                scenario.rendered(entry['frame'])
                report['passed'] = True
            except (HarnessError, OSError, ValueError, KeyError):
                scenario.capture('route-failure')
                raise
    except (HarnessError, OSError, ValueError, KeyError) as error:
        report['error'] = str(error)
        print(f'FAIL: {error}', flush=True)
    finally:
        report['wall_seconds'] = time.perf_counter() - started
        (args.out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        (args.out / 'recorded-route.json').write_text(json.dumps(report['route_commands'], indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
