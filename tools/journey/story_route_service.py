#!/usr/bin/env python3
"""Own a persistent normal-input reference runner independently of a tool PTY.

start creates a FIFO and detached runner; send writes one JSON command, or starts
an independently owned feeder for a saved command file. The runner's existing
allowlist remains authoritative. status never advances the game.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
COMMANDS = {'advance', 'press', 'key_down', 'key_up', 'pad', 'stick',
            'right_stick', 'triggers', 'release', 'capture', 'expect', 'expect-boss'}


def commands(path):
    entries = json.loads(Path(path).read_text())
    if not isinstance(entries, list):
        raise ValueError('command file must contain a JSON list')
    for entry in entries:
        validate(entry)
    return entries


def validate(entry):
    if not isinstance(entry, dict) or entry.get('verb') not in COMMANDS:
        raise ValueError('unsupported normal-input story command')
    if not isinstance(entry.get('args', []), list):
        raise ValueError('command arguments must be a JSON list')


def metadata(directory):
    return json.loads((directory / 'service.json').read_text())


def start(args):
    directory = args.directory.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    fifo = directory / 'commands.fifo'
    os.mkfifo(fifo, 0o600)
    # Keep both ends open in the detached reader: closing a short-lived writer
    # must not send EOF and discard the paused game between commands.
    descriptor = os.open(fifo, os.O_RDWR)
    command = [sys.executable, '-u', str(ROOT / 'tools/journey/replay_story_route.py'),
               '--app', str(Path(args.app).resolve()), '--out', str(directory / 'run'),
               '--max-frames', str(args.max_frames), '--explore']
    try:
        with (directory / 'driver.log').open('wb') as log:
            process = subprocess.Popen(command, cwd=ROOT, stdin=descriptor,
                                       stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True, close_fds=True)
    finally:
        os.close(descriptor)
    record = {'driver_pid': process.pid, 'cwd': str(ROOT), 'command': command,
              'fifo': str(fifo), 'log': str(directory / 'driver.log'),
              'protocol': 'one JSON {verb,args} per line; send never advances unless commanded',
              'ownership': 'detached process session; FIFO reader retains its own writer'}
    (directory / 'service.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record))


def send(args):
    directory = args.directory.resolve()
    record = metadata(directory)
    if args.file:
        entries = commands(args.file)
        # The detached feeder may block on FIFO capacity during long replay;
        # its parent returns immediately and its input stays in a durable file.
        queue = directory / ('queue-' + str(os.getpid()) + '.json')
        queue.write_text(json.dumps(entries) + '\n')
        with (directory / 'feeder.log').open('ab') as log:
            process = subprocess.Popen([sys.executable, str(Path(__file__).resolve()),
                                        'feed', str(directory), '--file', str(queue)],
                                       cwd=ROOT, stdin=subprocess.DEVNULL,
                                       stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True, close_fds=True)
        print(json.dumps({'feeder_pid': process.pid, 'commands': len(entries), 'queue': str(queue)}))
    else:
        entry = json.loads(args.command)
        validate(entry)
        payload = (json.dumps(entry, separators=(',', ':')) + '\n').encode()
        if len(payload) > 4096:
            raise ValueError('use --file for a command longer than 4096 bytes')
        descriptor = os.open(record['fifo'], os.O_WRONLY | os.O_NONBLOCK)
        try:
            if os.write(descriptor, payload) != len(payload):
                raise RuntimeError('incomplete FIFO command write')
        finally:
            os.close(descriptor)
        with (directory / 'submitted.jsonl').open('a') as journal:
            journal.write(json.dumps(entry, separators=(',', ':')) + '\n')
        print(json.dumps({'queued': entry}))


def feed(args):
    record = metadata(args.directory.resolve())
    entries = commands(args.file)
    with open(record['fifo'], 'w') as output:
        for entry in entries:
            output.write(json.dumps(entry, separators=(',', ':')) + '\n')
            output.flush()
    print(json.dumps({'delivered_commands': len(entries), 'queue': str(args.file)}), flush=True)


def status(args):
    directory = args.directory.resolve()
    record = metadata(directory)
    try:
        os.kill(record['driver_pid'], 0)
        alive = True
    except ProcessLookupError:
        alive = False
    report_path = directory / 'run/report.json'
    report = json.loads(report_path.read_text()) if report_path.exists() else None
    log = (directory / 'driver.log').read_text()
    print(json.dumps({'driver_pid': record['driver_pid'], 'alive': alive,
                      'ready_seen': 'READY:' in log,
                      'last_output': log.splitlines()[-4:],
                      'completed_commands': len(report['route_commands']) if report else None}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_subparsers(dest='action', required=True)
    launch = actions.add_parser('start')
    launch.add_argument('directory', type=Path)
    launch.add_argument('--app', required=True)
    launch.add_argument('--max-frames', type=int, default=60000)
    launch.set_defaults(run=start)
    for verb in ('send', 'feed'):
        command = actions.add_parser(verb)
        command.add_argument('directory', type=Path)
        source = command.add_mutually_exclusive_group(required=True)
        source.add_argument('--command')
        source.add_argument('--file', type=Path)
        command.set_defaults(run=send if verb == 'send' else feed)
    query = actions.add_parser('status')
    query.add_argument('directory', type=Path)
    query.set_defaults(run=status)
    args = parser.parse_args()
    args.run(args)


if __name__ == '__main__':
    main()
