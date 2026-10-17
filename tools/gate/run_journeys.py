#!/usr/bin/env python3
"""Check R1–R3 and report open R4 using one offscreen reference-app run.

Checkpoints retain the original windowed runner's frame properties and thresholds.
Screenshots, state snapshots and optional local goldens stay under build/journeys.
No game imagery is committed. Enter skips the black intro movie at frame 450.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time

REFERENCE_GAME_DIR = 'reference/updated-pc-reference/game'
WINDOWS_SYSTEM_DIR = 'reference/windows-system'
OUT = 'build/journeys'
RUN_TIMEOUT_SECONDS = 900

# Frame numbers come from runs on 2026-10-16 (docs/REVIEW-2026-10-16.md §2). The menu presses
# are spaced so that each lands after the previous screen has finished its transition.
MENU_PRESSES = (450, 800, 950, 1100, 1250, 1400, 1550)
JOURNEYS = [
    {'name': 'sega-logo', 'row': 'R1', 'state': 'required', 'frames': 120, 'presses': (),
     'expect': {'white_fraction_min': 0.8, 'blue_min': 2000},
     'about': 'PRESENTED BY SEGA: white background with the blue logo (same as app-smoke)'},
    {'name': 'title', 'row': 'R2', 'state': 'required', 'frames': 700, 'presses': (450,),
     'expect': {'colours_min': 2000, 'textured_draws_min': 20, 'no_skipped_from': 455},
     'about': 'Enter skips the intro movie; the title screen with Sonic and "Press ENTER"'},
    {'name': 'character-select', 'row': 'R3', 'state': 'required', 'frames': 1700,
     'presses': MENU_PRESSES,
     'expect': {'colours_min': 2000, 'textured_draws_min': 30, 'no_skipped_from': 455},
     'about': 'title -> file select -> main menu -> "Select a character" (needs the 100% save '
              'in reference/updated-pc-reference/game/savedata)'},
    {'name': 'world-3d', 'row': 'R4', 'state': 'open', 'frames': 2000,
     'presses': (*MENU_PRESSES, 1700, 1850),
     'expect': {'colours_min': 500, 'no_skipped_from': 455, 'textured_draws_min': 20},
     'about': 'choosing Sonic loads the 3D world; every draw must be drawn and textured. '
              'Open until the Metal fixed-function renderer exists (GOAL §6 item 3)'},
]

def read_ppm(path):
    data = Path(path).read_bytes()
    parts = data.split(b'\n', 3)
    if len(parts) != 4 or parts[0] != b'P6':
        raise ValueError(f'{path}: not a binary PPM')
    width, height = map(int, parts[1].split())
    if width <= 0 or height <= 0 or parts[2] != b'255' or len(parts[3]) != width * height * 3:
        raise ValueError(f'{path}: invalid dimensions, max value or pixel payload')
    return width, height, parts[3]

def picture_facts(path):
    width, height, px = read_ppm(path)
    white = blue = 0
    colours = set()
    grid = [[0, 0, 0, 0] for _ in range(32 * 24)]
    for i in range(0, len(px) - 2, 3):
        r, g, b = px[i], px[i + 1], px[i + 2]
        colours.add((r, g, b))
        if r > 240 and g > 240 and b > 240:
            white += 1
        elif b > r + 60 and b > 120:
            blue += 1
        pixel = i // 3
        cell = (pixel // width * 24 // height) * 32 + (pixel % width * 32 // width)
        acc = grid[cell]
        acc[0] += r; acc[1] += g; acc[2] += b; acc[3] += 1
    signature = [[round(c[0] / c[3]), round(c[1] / c[3]), round(c[2] / c[3])] if c[3] else [0, 0, 0]
                 for c in grid]
    return {'width': width, 'height': height, 'colours': len(colours),
            'white_fraction': white / (width * height), 'blue': blue, 'signature': signature}

FRAME_LINE = re.compile(r'^frame (\d+): guest present \d+, (\d+) draws \((\d+) textured\), (\d+) clears(.*)$', re.M)

def log_facts(text):
    frames = [(int(m[0]), int(m[1]), int(m[2]), m[4]) for m in FRAME_LINE.findall(text)]
    return {'frames': frames, 'stopped': re.search(r'guest stopped:[^\n]*', text)}

def signature_distance(a, b):
    if len(a) != 768 or len(b) != 768 or any(len(c) != 3 for c in [*a, *b]):
        raise ValueError('golden signature must have 768 RGB cells')
    return sum(abs(x - y) for ca, cb in zip(a, b) for x, y in zip(ca, cb)) / (len(a) * 3)

def check_checkpoint(journey, text, shot, goldens, bless=False):
    """Judge only the log prefix that led to this checkpoint, not later 3D draws."""
    problems = []
    facts = log_facts(text)
    frames = [f for f in facts['frames'] if f[0] <= journey['frames']]
    numbers = [f[0] for f in frames]
    if numbers != list(range(1, journey['frames'] + 1)):
        problems.append(f'frame lines missing or out of order ({len(numbers)} of {journey["frames"]})')
    if facts['stopped']:
        problems.append(facts['stopped'].group(0))
    expect = journey['expect']
    skipped_from = expect.get('no_skipped_from')
    if skipped_from is not None:
        skipped = [f for f in frames if f[0] >= skipped_from and 'not drawn' in f[3]]
        if skipped:
            first = skipped[0]
            problems.append(f'{len(skipped)} frames skipped draws from frame {first[0]}:{first[3].split("not drawn")[1]}')
    last = frames[-1] if frames else None
    picture = None
    try:
        picture = picture_facts(shot)
        if (picture['width'], picture['height']) != (640, 480):
            problems.append(f'screenshot is {picture["width"]}x{picture["height"]}, not 640x480')
        if picture['colours'] < expect.get('colours_min', 2):
            problems.append(f'{picture["colours"]} colours, expected at least {expect.get("colours_min", 2)}')
        if picture['white_fraction'] < expect.get('white_fraction_min', 0):
            problems.append(f'white fraction {picture["white_fraction"]:.2f} below {expect["white_fraction_min"]}')
        if picture['blue'] < expect.get('blue_min', 0):
            problems.append(f'{picture["blue"]} blue pixels, expected at least {expect["blue_min"]}')
        if last and last[2] < expect.get('textured_draws_min', 0):
            problems.append(f'last frame has {last[2]} textured draws, expected at least {expect["textured_draws_min"]}')
        name = journey['name']
        if bless and not problems:
            goldens[name] = picture['signature']
        elif name in goldens:
            distance = signature_distance(goldens[name], picture['signature'])
            picture['golden_distance'] = round(distance, 2)
            if distance > 12:
                problems.append(f'differs from the local golden (mean colour distance {distance:.1f} > 12)')
    except (OSError, ValueError) as error:
        problems.append(f'screenshot: {error}')
    result = {'name': journey['name'], 'row': journey['row'], 'state': journey['state'],
              'passed': not problems, 'problems': problems, 'screenshot': str(shot)}
    if picture:
        result['picture'] = {k: v for k, v in picture.items() if k != 'signature'}
    if last:
        result['last_frame'] = {'frame': last[0], 'draws': last[1], 'textured': last[2]}
    return result


def script_text(chosen, output):
    final_frame = max(j['frames'] for j in chosen)
    # All journeys are checkpoints on this same route. Later menu presses cannot
    # affect earlier captures. Keep five-frame key holds identical to --keys.
    presses = sorted({f for j in chosen for f in j['presses'] if f < final_frame})
    commands = [(f, f'press enter 5') for f in presses]
    for journey in chosen:
        frame, name = journey['frames'], journey['name']
        commands.extend([(frame, f'screenshot {output / (name + ".ppm")}'),
                         (frame, f'state {output / (name + ".state.json")}')])
    return '\n'.join(f'@{frame} {command}' for frame, command in sorted(commands)) + '\n'


def run(chosen, app, game_dir, system_dir, output, bless=False):
    output.mkdir(parents=True, exist_ok=True)
    # Remove previous checkpoint artifacts so a failed run cannot consume stale ones.
    for journey in chosen:
        for suffix in ('.ppm', '.state.json'):
            (output / (journey['name'] + suffix)).unlink(missing_ok=True)
    script = output / 'route.txt'
    script.write_text(script_text(chosen, output))
    log_path = output / 'route.log'
    command = [app, '--guest', game_dir, system_dir, '--offscreen', '--script', str(script),
               '--exit-after', str(max(j['frames'] for j in chosen))]
    started = time.monotonic()
    run_problems = []
    with log_path.open('w') as log:
        try:
            process = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                     timeout=RUN_TIMEOUT_SECONDS)
            if process.returncode:
                run_problems.append(f'sadx-app exited {process.returncode}')
        except subprocess.TimeoutExpired:
            run_problems.append(f'offscreen run exceeded {RUN_TIMEOUT_SECONDS} seconds')
    seconds = round(time.monotonic() - started, 2)
    text = log_path.read_text(errors='replace')
    golden_path = output / 'goldens.json'
    goldens = json.loads(golden_path.read_text()) if golden_path.exists() else {}
    results = []
    for journey in chosen:
        result = check_checkpoint(journey, text, output / (journey['name'] + '.ppm'), goldens,
                                  bless and not run_problems)
        state_path = output / (journey['name'] + '.state.json')
        try:
            state = json.loads(state_path.read_text())
            if state.get('frame') != journey['frames']:
                raise ValueError('state frame does not match checkpoint')
            result['game_state'] = state
        except (OSError, ValueError) as error:
            result['problems'].append(f'state snapshot: {error}')
        result['problems'].extend(run_problems)
        result['passed'] = not result['problems']
        results.append(result)
        verdict = 'pass' if result['passed'] else ('FAIL' if journey['state'] == 'required' else 'open')
        print(f'{journey["row"]:3} {journey["name"]:17} {verdict:4}  ' +
              ('; '.join(result['problems']) or journey['about']), flush=True)
        if result['passed'] and journey['state'] == 'open':
            print(f'    {journey["name"]} now passes: promote it in tools/gate/run_journeys.py')
    if bless and not run_problems:
        golden_path.write_text(json.dumps(goldens) + '\n')
    saves = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
             for p in sorted((Path(game_dir) / 'savedata').glob('*.snc'))}
    report = {'journeys': results, 'seconds': seconds, 'command': shlex.join(command),
              'log': str(log_path), 'save_sha256': saves, 'run_problems': run_problems}
    (output / 'report.json').write_text(json.dumps(report, indent=1) + '\n')
    failed = [r for r in results if r['state'] == 'required' and not r['passed']]
    print(f'journeys: {sum(r["passed"] for r in results)}/{len(results)} pass, '
          f'{len(failed)} required failing; {seconds}s; report {output / "report.json"}')
    return 1 if failed or run_problems else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--only', help='comma-separated journey names')
    parser.add_argument('--bless', action='store_true', help='write local golden signatures for passing journeys')
    parser.add_argument('--app', default='build/sadx-app')
    parser.add_argument('--game-dir', default=REFERENCE_GAME_DIR)
    parser.add_argument('--system-dir', default=WINDOWS_SYSTEM_DIR)
    parser.add_argument('--output', type=Path, default=Path(OUT))
    parser.add_argument('--list', action='store_true')
    args = parser.parse_args()
    if args.list:
        for j in JOURNEYS:
            print(f'{j["row"]:3} {j["name"]:17} {j["state"]:8} {j["frames"]:5} frames  {j["about"]}')
        return 0
    names = set(args.only.split(',')) if args.only else {j['name'] for j in JOURNEYS}
    unknown = names - {j['name'] for j in JOURNEYS}
    if unknown:
        parser.error(f'unknown journeys: {sorted(unknown)}')
    chosen = [j for j in JOURNEYS if j['name'] in names]
    if any(c.isspace() for c in str(args.output)):
        parser.error('--output must not contain whitespace (harness paths are whitespace-delimited)')
    if not Path(args.app).exists():
        parser.error(f'{args.app} not built (make -j10 build/sadx-app)')
    return run(chosen, args.app, args.game_dir, args.system_dir, args.output, args.bless)


if __name__ == '__main__':
    sys.exit(main())
