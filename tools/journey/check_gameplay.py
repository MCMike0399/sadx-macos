#!/usr/bin/env python3
"""Exercise Sonic through real menu/input commands and retain state + image evidence.

This runs the guest reference, not the executable-independent native product. The
configured speedrun limit and measured host throughput are reported separately.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
import time

from sadx_driver import HarnessError, Session, REFERENCE_GAME_DIR

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'gate'))
from run_journeys import picture_facts, log_facts


def distance(a, b):
    if not isinstance(a, list) or not isinstance(b, list) or len(a) != 3 or len(b) != 3:
        raise HarnessError('expected two observed 3D vectors')
    if any(not isinstance(v, (int, float)) or not math.isfinite(v) for v in a + b):
        raise HarnessError('non-finite or unavailable observed vector')
    return math.dist(a, b)


def player(state):
    observed = state.get('player')
    if not observed or observed.get('character') != 0:
        raise HarnessError(f'Sonic is absent at frame {state.get("frame")}')
    distance(observed['position'], observed['position'])
    return observed


class Scenario:
    def __init__(self, game, out, report):
        self.game, self.out, self.report = game, out, report

    def command(self, verb, *args):
        self.report['commands'].append({'frame': self.game.frame, 'verb': verb, 'args': args})
        return getattr(self.game, verb)(*args)

    def capture(self, name):
        state = self.game.state()
        shot = self.out / f'{name}.png'
        self.game.screenshot(str(shot))
        (self.out / f'{name}.state.json').write_text(json.dumps(state, indent=2) + '\n')
        self.report['snapshots'].append({'name': name, 'state': state, 'screenshot': str(shot),
                                         'screenshot_sha256': hashlib.sha256(shot.read_bytes()).hexdigest()})
        print(f'{name}: frame={state["frame"]} mode={state["game_mode"]} '
              f'stage={state["stage"]} player={state["player"] is not None}', flush=True)
        return state

    def check(self, name, condition, **facts):
        self.report['checks'].append({'name': name, 'passed': bool(condition), **facts})
        if not condition:
            raise HarnessError(f'{name}: {facts}')

    def rendered(self, first_frame):
        shot = self.out / 'world.ppm'
        self.game.screenshot(str(shot))
        picture = picture_facts(shot)
        self.check('world-frame-visible', picture['width'] == 640 and picture['height'] == 480
                   and picture['colours'] >= 500, width=picture['width'], height=picture['height'],
                   colours=picture['colours'])
        frames = [f for f in log_facts(Path(self.game.log.name).read_text())['frames']
                  if f[0] >= first_frame]
        skipped = [f[0] for f in frames if 'not drawn' in f[3]]
        self.check('gameplay-draws-supported', bool(frames) and not skipped,
                   frames=len(frames), skipped_frames=skipped)

    def completed_save_route(self):
        # Same user-owned completed first slot as R1-R4; no guest memory writes.
        for target in (450, 800, 950, 1100, 1250, 1400, 1550, 1700, 1850):
            if self.game.frame < target:
                self.command('advance', target - self.game.frame)
            self.command('press', 'enter')
        self.command('advance', 300)
        return self.capture('sonic-start')

    def fresh_adventure_route(self):
        for target in (450, 800):
            self.command('advance', target - self.game.frame)
            self.command('press', 'enter')
        self.command('advance', 150)
        self.capture('file-select')
        self.command('press', 'up')
        self.command('advance', 40)
        self.command('press', 'enter')
        self.command('advance', 150)
        self.capture('new-file-created')
        # Observed normal route: Adventure -> Sonic -> Game -> start animation.
        # The local source saves remain unchanged; the game creates slot 3 in the overlay.
        for target in (1140, 1290, 1440, 1590, 1740):
            if self.game.frame < target:
                self.command('advance', target - self.game.frame)
            self.command('press', 'enter')
        self.command('advance', 300)
        intro = self.capture('sonic-intro')
        self.check('fresh-sonic-intro', intro['game_mode'] == 5 and intro['stage'] == 26,
                   stage=intro['stage'], mode=intro['game_mode'])
        self.command('advance', 300)
        self.capture('intro-before-skip')
        self.command('press', 'enter')  # normal Start/Skipma path, no state injection
        self.command('advance', 20)
        self.capture('chaos-zero-intro')
        # The boss intro arrives during MD_GAME_FADEIN; Start can be ignored there.
        # The observed main-state boundary at frame 2420 accepts the next normal skip.
        self.command('press', 'enter')
        self.command('advance', 60)
        self.capture('boss-intro-before-skip')
        self.command('press', 'enter')
        self.command('advance', 30)
        return self.capture('chaos-zero-start')

    def verify_playable_profile(self, start):
        # SADXEnumsNew.h: MD_ACTION=4, MD_ADVENTURE=5.
        expected_mode = 4 if start['stage'] == 15 else 5
        self.check('expected-gameplay-mode', start['game_mode'] == expected_mode,
                   expected=expected_mode, observed=start['game_mode'])
        expected_profile = {'ccef_reset_1': '810d', 'ccef_reset_2': '810d',
            'ehcf_dereference': '909090', 'ehcf_branch': '9090',
            'skipma_ingame_to_speedrun': True, 'skipma_movie_to_speedrun': True,
            'limiter_to_speedrun': True, 'extra_ticks_to_speedrun': True,
            'frame_rate_scalar': 63}
        self.check('pinned-profile-live-patches', start.get('profile') == expected_profile,
                   observed=start.get('profile'), expected=expected_profile)
        player(start)

    def chaos_controls(self, start, measure_frames):
        """Short fight input checks; camera input is qualified by the Station Square route."""
        self.verify_playable_profile(start)
        # Observed entry sequence: the boss camera holds control before its name/HUD.
        # Normal A input also uses the pinned Skipma path; no timer/state is overwritten.
        self.command('advance', 210)
        self.command('key_down', 'x')
        self.command('advance', 60)
        self.command('release')
        baseline = self.capture('chaos-ready')
        self.command('key_down', 'right')
        self.command('advance', 30)
        moved = self.capture('keyboard-moved')
        self.command('release')
        movement = distance(player(baseline)['position'], player(moved)['position'])
        self.check('keyboard-moves-sonic', movement > 1 and player(moved)['action'] == 2,
                   distance=movement, action=player(moved)['action'])
        # Check the pad before the first projectile; a hurt/knockback action cannot pass.
        self.command('advance', 30)
        before_pad = self.capture('before-pad')
        self.command('stick', -1, 0, 30)
        self.command('advance', 30)
        moved_pad = self.capture('pad-moved')
        self.command('release')
        movement = distance(player(before_pad)['position'], player(moved_pad)['position'])
        self.check('pad-moves-sonic', movement > 1 and player(moved_pad)['action'] == 2,
                   distance=movement, action=player(moved_pad)['action'])
        self.command('advance', 60)
        before_jump = self.capture('before-jump')
        self.command('press', 'x', 8)
        self.command('advance', 15)
        jumped = self.capture('jump')
        rise = player(jumped)['position'][1] - player(before_jump)['position'][1]
        self.check('keyboard-jump-rises', rise > 1 and player(jumped)['action'] == 8,
                   rise=rise, action=player(jumped)['action'])
        self.command('release')
        started = time.perf_counter()
        frame = self.game.frame
        self.command('advance', measure_frames)
        elapsed = time.perf_counter() - started
        self.capture('fight-after-inputs')
        self.report['performance'] = {'presents': self.game.frame - frame,
            'wall_seconds': elapsed, 'observed_host_fps': (self.game.frame - frame) / elapsed,
            'offscreen_uncapped': True, 'configured_profile_fps': 63}
        self.report['checks_not_run'] = ['Chaos 0 camera control; use station-square for camera input checks',
                                         'complete boss fight', 'Emerald Coast goal replay']

    def movement(self, start, measure_frames):
        self.verify_playable_profile(start)
        self.command('advance', 90)
        baseline = self.capture('settled')
        started = time.perf_counter()
        frame = self.game.frame
        self.command('advance', measure_frames)
        elapsed = time.perf_counter() - started
        self.report['performance'] = {'presents': self.game.frame - frame,
            'wall_seconds': elapsed, 'observed_host_fps': (self.game.frame - frame) / elapsed,
            'offscreen_uncapped': True, 'configured_profile_fps': 63}
        self.command('key_down', 'right')
        self.command('advance', 45)
        moved = self.capture('keyboard-moved')
        self.command('release')
        moved_distance = distance(player(baseline)['position'], player(moved)['position'])
        self.check('keyboard-moves-sonic', moved_distance > 1 and player(moved)['action'] == 2,
                   distance=moved_distance, action=player(moved)['action'])
        self.command('advance', 90)
        before_jump = self.capture('before-jump')
        self.command('press', 'x', 8)
        samples = []
        for index in range(6):
            self.command('advance', 5)
            samples.append(self.capture(f'jump-{index}'))
        rise = max(player(s)['position'][1] - player(before_jump)['position'][1] for s in samples)
        self.check('keyboard-jump-rises', rise > 1, rise=rise,
                   actions=[player(s)['action'] for s in samples])
        self.command('release')
        self.command('advance', 120)
        before_pad = self.capture('before-pad')
        self.command('stick', -1, 0, 45)
        self.command('advance', 45)
        pad_moved = self.capture('pad-moved')
        self.command('release')
        pad_distance = distance(player(before_pad)['position'], player(pad_moved)['position'])
        self.check('pad-moves-sonic', pad_distance > 1 and player(pad_moved)['action'] == 2,
                   distance=pad_distance, action=player(pad_moved)['action'])
        self.command('advance', 120)
        idle_start = self.capture('camera-idle-start')
        self.command('advance', 45)
        camera_start = self.capture('camera-before-input')
        self.command('key_down', 'l')  # pinned Input mod +rightx (camera right)
        self.command('advance', 45)
        camera_end = self.capture('camera-after-input')
        self.command('release')
        cameras = [s.get('camera') for s in (idle_start, camera_start, camera_end)]
        self.check('camera-observed', all(cameras), errors=[s.get('camera_error') for s in
                   (idle_start, camera_start, camera_end)])
        idle_distance = distance(cameras[0]['position'], cameras[1]['position'])
        input_distance = distance(cameras[1]['position'], cameras[2]['position'])
        angle_changed = cameras[1]['rotation'] != cameras[2]['rotation']
        drift = distance(player(camera_start)['position'], player(camera_end)['position'])
        self.check('camera-input-changes-view', input_distance > idle_distance + 1 and angle_changed
                   and drift < 1, idle_distance=idle_distance, input_distance=input_distance,
                   angle_changed=angle_changed, player_drift=drift)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=Path('build/journeys/gameplay'))
    parser.add_argument('--app', default='build/sadx-app')
    parser.add_argument('--measure-frames', type=int, default=120)
    parser.add_argument('--timeout', type=int, default=600,
                        help='per-command wall-clock timeout, including event/asset loading')
    parser.add_argument('--original-tasks', action='store_true',
                        help='explicit reference experiment: execute original x86 task routines')
    parser.add_argument('--route', choices=('station-square', 'fresh-adventure'),
                        default='station-square')
    args = parser.parse_args()
    if args.measure_frames <= 0:
        parser.error('--measure-frames must be positive')
    args.out.mkdir(parents=True, exist_ok=True)
    overlay = args.out / 'overlay'
    if overlay.exists():
        parser.error(f'overlay already exists: {overlay}; use a fresh output directory for reproducibility')
    report = {'engine': 'guest-reference', 'route': args.route,
              'task_execution': 'original-x86' if args.original_tasks else 'native-task-adapter',
              'passed': False, 'commands': [], 'checks': [], 'snapshots': []}
    game_dir = Path(REFERENCE_GAME_DIR)
    inputs = [Path(args.app), Path(__file__), Path(__file__).with_name('sadx_driver.py'),
              game_dir / 'sonic.exe', game_dir / 'savedata/SonicDX01.snc',
              game_dir / 'mods/.modloader/profiles/updated-speedrun.json',
              game_dir / 'mods/.modloader/SADXModLoader.dll',
              game_dir / 'mods/sadx-input-mod/sadx-input-mod.dll',
              game_dir / 'mods/sadx-input-mod/config.ini',
              game_dir / 'mods/sadx-speedrun-mod/sadx-speedrun-mod.dll',
              game_dir / 'mods/sadx-speedrun-mod/config.ini']
    report['input_sha256'] = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                              if path.is_file() else None for path in inputs}
    started = time.perf_counter()
    try:
        with Session(app=args.app, log_path=str(args.out / 'app.log'), overlay=overlay,
                     timeout=args.timeout, original_tasks=args.original_tasks) as game:
            scenario = Scenario(game, args.out, report)
            try:
                state = (scenario.completed_save_route() if args.route == 'station-square'
                         else scenario.fresh_adventure_route())
                expected_stage = 26 if args.route == 'station-square' else 15
                scenario.check('expected-stage', state['stage'] == expected_stage, stage=state['stage'])
                if args.route == 'fresh-adventure':
                    scenario.chaos_controls(state, args.measure_frames)
                else:
                    scenario.movement(state, args.measure_frames)
                scenario.rendered(state['frame'])
                report['passed'] = True
            except HarnessError:
                try:
                    scenario.capture('failure')
                except (HarnessError, OSError, ValueError) as observation_error:
                    report['failure_observation_error'] = str(observation_error)
                raise
    except (HarnessError, OSError, KeyError, ValueError) as error:
        report['error'] = str(error)
        print(f'FAIL: {error}', flush=True)
    finally:
        report['wall_seconds'] = time.perf_counter() - started
        (args.out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
