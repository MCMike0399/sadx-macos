#!/usr/bin/env python3
"""Journey checks: drive sadx-app through the screens a player sees and check each frame.

GOAL §3.1 defines the journey ladder (R1..R6 for the reference app, N1..N5 natively). This
runner covers the reference-app rows that can be scripted today. Each journey launches
`build/sadx-app --guest` with scripted key presses (Enter = VK 13). The guest runs on a virtual
clock, so a given frame number shows the same screen on every run. The runner takes the
screenshot the app writes at `--exit-after`, and checks:

* the log: every frame line present and in order, the guest never stopped, and no draw was
  skipped (`not drawn`) in the frames the journey cares about;
* the picture: frame properties (size, colour count, colour balance, textured draw count);
* optionally, a local golden signature: a 32x24 grid of mean colours, written by `--bless`
  under build/journeys/ (ignored). The comparison uses a mean-difference threshold because
  Metal filtering may differ between GPUs. Game imagery is never committed.

A journey is either `required` (it must pass; a failure is a regression) or `open` (a ladder
row the port has not reached yet). An open journey is expected to fail. When it passes, the
runner says so, and the next step promotes it to required in JOURNEYS below.

Usage: run_journeys.py [--only NAME,...] [--bless] [--app build/sadx-app]
Exit status: 0 when every required journey passes, 1 otherwise (open journeys never fail the run).
"""
import argparse, json, os, re, subprocess, sys, time

REFERENCE_GAME_DIR = 'reference/updated-pc-reference/game'
WINDOWS_SYSTEM_DIR = 'reference/windows-system'
OUT = 'build/journeys'
ENTER = 13
STALL_SECONDS = 45

def enter_at(*frames):
    return ','.join(f'{f}:{ENTER}' for f in frames)

# Frame numbers come from runs on 2026-10-16 (docs/REVIEW-2026-10-16.md §2). The menu presses
# are spaced so that each lands after the previous screen has finished its transition.
MENU_PRESSES = (450, 800, 950, 1100, 1250, 1400, 1550)
JOURNEYS = [
    {'name': 'sega-logo', 'row': 'R1', 'state': 'required', 'frames': 120, 'keys': '',
     'expect': {'white_fraction_min': 0.8, 'blue_min': 2000},
     'about': 'PRESENTED BY SEGA: white background with the blue logo (same as app-smoke)'},
    {'name': 'title', 'row': 'R2', 'state': 'required', 'frames': 700, 'keys': enter_at(450),
     'expect': {'colours_min': 2000, 'textured_draws_min': 20, 'no_skipped_from': 455},
     'about': 'Enter skips the intro movie; the title screen with Sonic and "Press ENTER"'},
    {'name': 'character-select', 'row': 'R3', 'state': 'required', 'frames': 1700,
     'keys': enter_at(*MENU_PRESSES),
     'expect': {'colours_min': 2000, 'textured_draws_min': 30, 'no_skipped_from': 455},
     'about': 'title -> file select -> main menu -> "Select a character" (needs the 100% save '
              'in reference/updated-pc-reference/game/savedata)'},
    {'name': 'world-3d', 'row': 'R4', 'state': 'open', 'frames': 2000,
     'keys': enter_at(*MENU_PRESSES, 1700, 1850),
     'expect': {'colours_min': 500, 'no_skipped_from': 455, 'textured_draws_min': 20},
     'about': 'choosing Sonic loads the 3D world; every draw must be drawn and textured. '
              'Open until the Metal fixed-function renderer exists (GOAL §6 item 3)'},
]

def read_ppm(path):
    data = open(path, 'rb').read()
    parts = data.split(b'\n', 3)
    if parts[0] != b'P6':
        raise ValueError(f'{path}: not a binary PPM')
    width, height = map(int, parts[1].split())
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
    return sum(abs(x - y) for ca, cb in zip(a, b) for x, y in zip(ca, cb)) / (len(a) * 3)

def run(journey, app, bless, goldens):
    os.makedirs(OUT, exist_ok=True)
    shot = f'{OUT}/{journey["name"]}.ppm'
    log_path = f'{OUT}/{journey["name"]}.log'
    command = [app, '--guest', REFERENCE_GAME_DIR, WINDOWS_SYSTEM_DIR,
               '--exit-after', str(journey['frames']), '--screenshot', shot]
    if journey['keys']:
        command += ['--keys', journey['keys']]
    if os.path.exists(shot):
        os.remove(shot)
    started = time.time()
    stalled = False
    with open(log_path, 'w') as log:
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
        # The Metal view stops drawing while its window is hidden or covered, which freezes the
        # guest. Treat a log that stops growing as a stall instead of waiting forever.
        last_size, last_change = -1, time.time()
        while process.poll() is None:
            time.sleep(2)
            size = os.path.getsize(log_path)
            if size != last_size:
                last_size, last_change = size, time.time()
            elif time.time() - last_change > STALL_SECONDS:
                stalled = True
                process.kill()
                process.wait()
                break
        rc = process.returncode
    seconds = round(time.time() - started, 1)
    text = open(log_path, errors='replace').read()
    problems = []
    if stalled:
        problems.append(f'stalled: no new frame for {STALL_SECONDS} s (is the app window visible? '
                        'the Metal view pauses while it is hidden or covered)')
    elif rc != 0:
        problems.append(f'sadx-app exited {rc}')
    facts = log_facts(text)
    numbers = [f[0] for f in facts['frames']]
    if numbers != list(range(1, journey['frames'] + 1)):
        problems.append(f'frame lines missing or out of order ({len(numbers)} of {journey["frames"]})')
    if facts['stopped']:
        problems.append(facts['stopped'].group(0))
    expect = journey['expect']
    skipped_from = expect.get('no_skipped_from')
    if skipped_from is not None:
        skipped = [f for f in facts['frames'] if f[0] >= skipped_from and 'not drawn' in f[3]]
        if skipped:
            first = skipped[0]
            problems.append(f'{len(skipped)} frames skipped draws from frame {first[0]}:{first[3].split("not drawn")[1]}')
    last = facts['frames'][-1] if facts['frames'] else None
    picture = None
    if not os.path.exists(shot):
        problems.append('no screenshot written')
    else:
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
    result = {'name': journey['name'], 'row': journey['row'], 'state': journey['state'],
              'passed': not problems, 'problems': problems, 'seconds': seconds,
              'command': ' '.join(command), 'screenshot': shot, 'log': log_path}
    if picture:
        result['picture'] = {k: v for k, v in picture.items() if k != 'signature'}
    if last:
        result['last_frame'] = {'frame': last[0], 'draws': last[1], 'textured': last[2]}
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--only', help='comma-separated journey names')
    parser.add_argument('--bless', action='store_true', help='write local golden signatures for passing journeys')
    parser.add_argument('--app', default='build/sadx-app')
    parser.add_argument('--list', action='store_true')
    args = parser.parse_args()
    if args.list:
        for j in JOURNEYS:
            print(f'{j["row"]:3} {j["name"]:17} {j["state"]:8} {j["frames"]:5} frames  {j["about"]}')
        return 0
    if not os.path.exists(args.app):
        sys.exit(f'{args.app} not built (make -j10 build/sadx-app)')
    chosen = [j for j in JOURNEYS if not args.only or j['name'] in args.only.split(',')]
    golden_path = f'{OUT}/goldens.json'
    goldens = json.load(open(golden_path)) if os.path.exists(golden_path) else {}
    results = []
    for journey in chosen:
        result = run(journey, args.app, args.bless, goldens)
        results.append(result)
        verdict = 'pass' if result['passed'] else ('FAIL' if journey['state'] == 'required' else 'open')
        print(f'{journey["row"]:3} {journey["name"]:17} {verdict:4} {result["seconds"]:6}s  ' +
              ('; '.join(result['problems']) if result['problems'] else journey['about']), flush=True)
        if result['passed'] and journey['state'] == 'open':
            print(f'    {journey["name"]} now passes: promote it to required in tools/journey/run_journeys.py')
    os.makedirs(OUT, exist_ok=True)
    if args.bless:
        json.dump(goldens, open(golden_path, 'w'))
    json.dump({'journeys': results}, open(f'{OUT}/report.json', 'w'), indent=1)
    failed = [r for r in results if r['state'] == 'required' and not r['passed']]
    print(f'journeys: {sum(r["passed"] for r in results)}/{len(results)} pass, '
          f'{len(failed)} required failing; report {OUT}/report.json')
    return 1 if failed else 0

if __name__ == '__main__':
    sys.exit(main())
