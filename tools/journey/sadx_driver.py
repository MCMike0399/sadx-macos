#!/usr/bin/env python3
"""Drive the reference app like a player: advance frames, press keys, move the sticks, take
screenshots and read the game state. For tests and for agents exploring the game.

It starts `build/sadx-app --guest … --control -` (offscreen by default), paused on its first
frame. Every action is deterministic, because the guest runs on a virtual clock and only
advances when told to. The harness protocol is described in `src/apps/app_harness.hpp`.

Library use:

    from sadx_driver import Session          # tools/journey on sys.path
    with Session() as game:
        game.advance(450)                    # frames run, then the game pauses again
        game.press('enter')                  # skip the intro movie
        game.advance(250)
        game.screenshot('build/journeys/title.png')
        print(game.state()['game_mode'])     # 12: the title (ulGlobalMode)
        game.stick(1, 0, frames=60)          # hold the left stick right for 60 frames
        game.run_until(lambda s: s['player'] is not None, max_frames=3000)

Command line:

    tools/journey/sadx_driver.py demo [--window] [--out DIR]
        title -> file select -> character select -> Sonic, with a screenshot and a state
        JSON at each screen (the driver's own smoke test)
    tools/journey/sadx_driver.py shell [--window]
        read commands from stdin and print each reply: `advance N`, `screenshot PATH`,
        `state`, `press enter`, `stick 1 0 60`, ... (`help` lists them)

Keys: enter esc space tab backspace up down left right shift ctrl alt a..z 0..9.
Pad buttons: a b x y start back lb rb up down left right ls rs, joined with `+`.
SADX takes no mouse input, so there is nothing to click inside the game. Window focus and
covered windows do not matter offscreen.
"""
import argparse, json, os, queue, subprocess, sys, threading

REFERENCE_GAME_DIR = 'reference/updated-pc-reference/game'
WINDOWS_SYSTEM_DIR = 'reference/windows-system'


class HarnessError(RuntimeError):
    pass


class Session:
    def __init__(self, app='build/sadx-app', window=False, log_path='build/journeys/driver.log',
                 timeout=600):
        os.makedirs(os.path.dirname(log_path) or '.', exist_ok=True)
        command = [app, '--guest', REFERENCE_GAME_DIR, WINDOWS_SYSTEM_DIR, '--control', '-']
        if not window:
            command.append('--offscreen')
        self.timeout = timeout
        self.log = open(log_path, 'w')
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.replies = queue.Queue()
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()
        self.frame = self.state()['frame']

    def _read(self):
        for line in self.process.stdout:
            self.log.write(line)
            if line.startswith('harness: '):
                self.replies.put(line[len('harness: '):].rstrip('\n'))
        self.replies.put(None)

    def send(self, command, verb=None):
        """Send one harness command and wait for its reply; returns the text after `ok`."""
        verb = verb or command.split()[0].lstrip('@')
        if command.startswith('@'):
            verb = command.split()[1]
        self.process.stdin.write(command + '\n')
        self.process.stdin.flush()
        while True:
            try:
                reply = self.replies.get(timeout=self.timeout)
            except queue.Empty:
                raise HarnessError(f'no reply to {command!r} within {self.timeout} s')
            if reply is None:
                raise HarnessError(f'sadx-app exited while waiting for {command!r} (see {self.log.name})')
            frame, got, rest = (reply.split(' ', 2) + ['', ''])[:3]
            if got != verb:
                continue
            self.frame = int(frame)
            if rest.startswith('error'):
                raise HarnessError(f'{command!r}: {rest}')
            return rest[3:] if rest.startswith('ok ') else ''

    # Time.
    def advance(self, frames=1):
        """Run FRAMES frames, then pause. Returns the new frame number."""
        target = self.frame + max(int(frames), 1)
        self.process.stdin.write(f'@{target} pause\n')  # scheduled; it answers when it runs
        self.send('run')
        self._wait_for('pause', target)
        return self.frame

    def _wait_for(self, verb, frame):
        while True:
            reply = self.replies.get(timeout=self.timeout)
            if reply is None:
                raise HarnessError(f'sadx-app exited before frame {frame} (see {self.log.name})')
            at, got = reply.split(' ', 2)[:2]
            if got == verb and int(at) >= frame:
                self.frame = int(at)
                return

    def run_until(self, predicate, max_frames=6000, every=10):
        """Advance in steps of EVERY frames until predicate(state) holds; returns that state."""
        start = self.frame
        while self.frame - start < max_frames:
            self.advance(every)
            current = self.state()
            if predicate(current):
                return current
        raise HarnessError(f'condition not reached within {max_frames} frames (frame {self.frame})')

    # Input. FRAMES=None holds until released.
    def press(self, key, frames=5):
        self.send(f'press {key} {frames}')

    def key_down(self, key):
        self.send(f'down {key}')

    def key_up(self, key):
        self.send(f'up {key}')

    def pad(self, buttons, frames=None):
        self.send(f'pad {buttons}' + (f' {frames}' if frames else ''))

    def stick(self, x, y, frames=None):
        self.send(f'stick {x} {y}' + (f' {frames}' if frames else ''))

    def right_stick(self, x, y, frames=None):
        self.send(f'rstick {x} {y}' + (f' {frames}' if frames else ''))

    def triggers(self, left, right, frames=None):
        self.send(f'triggers {left} {right}' + (f' {frames}' if frames else ''))

    def release(self):
        self.send('release')

    # Observation.
    def screenshot(self, path):
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        return self.send(f'screenshot {os.path.abspath(path)}')

    def state(self):
        return json.loads(self.send('state'))

    def close(self):
        if self.process.poll() is None:
            try:
                self.process.stdin.write('quit\n')
                self.process.stdin.flush()
                self.process.stdin.close()
            except BrokenPipeError:
                pass
            try:
                self.process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                self.process.kill()
        self.log.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


def demo(window, out):
    """Title -> file select -> character select -> Sonic, with evidence at each screen."""
    os.makedirs(out, exist_ok=True)
    steps = [('title', 450, 250), ('file-select', 800, 150), ('main-menu', 950, 150),
             ('menu-2', 1100, 150), ('menu-3', 1250, 150), ('menu-4', 1400, 150),
             ('character-select', 1550, 150), ('sonic-chosen', 1700, 150),
             ('loading', 1850, 300)]
    results = []
    with Session(window=window) as game:
        for name, press_at, settle in steps:
            game.advance(press_at - game.frame)
            game.press('enter')
            game.advance(settle)
            shot = game.screenshot(f'{out}/{name}.png')
            current = game.state()
            results.append({'screen': name, 'frame': game.frame, 'screenshot': shot, 'state': current})
            print(f'{name:17} frame {game.frame:5}  mode {current["game_mode"]:3}  '
                  f'stage {current["stage"]:3}  player {"yes" if current["player"] else "no"}', flush=True)
    json.dump(results, open(f'{out}/demo.json', 'w'), indent=1)
    print(f'screenshots and states in {out}/')


HELP = """advance N | press KEY [FRAMES] | down KEY | up KEY | pad BUTTONS [FRAMES] |
stick X Y [FRAMES] | rstick X Y [FRAMES] | triggers L R [FRAMES] | release |
screenshot PATH | state | frame | quit"""


def shell(window):
    with Session(window=window) as game:
        print(f'paused at frame {game.frame}; `help` lists commands', flush=True)
        for line in sys.stdin:
            words = line.split()
            if not words:
                continue
            try:
                if words[0] == 'help':
                    print(HELP)
                elif words[0] == 'quit':
                    break
                elif words[0] == 'advance':
                    print(f'frame {game.advance(int(words[1]) if len(words) > 1 else 1)}')
                elif words[0] == 'frame':
                    print(f'frame {game.frame}')
                elif words[0] == 'state':
                    print(json.dumps(game.state()))
                elif words[0] == 'screenshot':
                    print(game.screenshot(words[1]))
                else:
                    game.send(' '.join(words))
                    print('ok')
            except (HarnessError, IndexError, ValueError) as error:
                print(f'error: {error}')
            sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    sub = parser.add_subparsers(dest='command', required=True)
    d = sub.add_parser('demo')
    d.add_argument('--window', action='store_true')
    d.add_argument('--out', default='build/journeys/demo')
    s = sub.add_parser('shell')
    s.add_argument('--window', action='store_true')
    args = parser.parse_args()
    if args.command == 'demo':
        demo(args.window, args.out)
    else:
        shell(args.window)


if __name__ == '__main__':
    main()
