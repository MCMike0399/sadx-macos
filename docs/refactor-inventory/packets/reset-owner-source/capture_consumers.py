"""Capture unchanged default reset-consumer judges without changing their inputs or verdicts.

Every call retains ordered identity, every scalar/byte/array argument, its exact extent,
return value and complete post-call buffers. Original observations are retained separately.
"""
import collections
import ctypes
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import sys


def argument(value):
    if isinstance(value, ctypes.Array):
        data = bytes(value)
        return {'kind': 'array', 'bytes': len(data), 'data': data.hex(),
                'element_type': value._type_.__name__}
    if isinstance(value, bytes):
        return {'kind': 'bytes', 'bytes': len(value), 'data': value.hex()}
    if isinstance(value, int):
        return {'kind': 'integer', 'value': value}
    raise TypeError(f'Uncaptured ABI argument type: {type(value)}')


def new_destination(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.mkdir()  # Exclusive: even an incomplete earlier capture is immutable.
    return path


def validate_arguments(name, arguments, contract):
    signature = contract['signatures'].get(name)
    if signature is None or len(arguments) != len(signature):
        raise ValueError('unexpected symbol or ABI argument count')
    for actual, expected in zip(arguments, signature):
        if any(actual.get(key) != value for key, value in expected.items()):
            raise ValueError('ABI argument kind, scalar, type or extent differs')


def validate_domain(capture, observed, contract):
    expected = contract['native_calls']
    sequence = capture['sequence']
    if expected <= 0 or capture['count'] != expected or len(sequence) != expected:
        raise ValueError('empty, truncated or expanded native default domain')
    if dict(collections.Counter(sequence)) != contract['symbols']:
        raise ValueError('default export counts differ')
    if hashlib.sha256('\n'.join(sequence).encode()).hexdigest() != contract['symbol_sequence_sha256']:
        raise ValueError('default exports reordered')
    if contract['original_observations'] <= 0 or len(observed) != contract['original_observations']:
        raise ValueError('empty or partial original domain')
    if observed != list(range(contract['original_observations'])):
        raise ValueError('original cases duplicated or reordered')
    if capture['inputs'].hexdigest() != contract['inputs_sha256']:
        raise ValueError('default ordered input domain differs from immutable old')


class Function:
    def __init__(self, function, capture):
        object.__setattr__(self, 'function', function)
        object.__setattr__(self, 'capture', capture)

    def __setattr__(self, name, value):
        setattr(self.function, name, value)

    def __getattr__(self, name):
        return getattr(self.function, name)

    def __call__(self, *args):
        inputs = [argument(a) for a in args]
        ordinal = self.capture['count']
        if 'contract' in self.capture:
            validate_arguments(self.function.__name__, inputs, self.capture['contract'])
            self.capture['sequence'].append(self.function.__name__)
            normalized = [{k: v for k, v in a.items() if k != 'element_type'} for a in inputs]
            self.capture['inputs'].update(json.dumps(normalized, separators=(',', ':')).encode() + b'\n')
        result = self.function(*args)
        outputs = [argument(a) for a in args]
        data = json.dumps({'ordinal': ordinal, 'function': self.function.__name__,
                           'argument_count': len(args),
                           'result': None if self.capture.get('contract', {}).get('return_kind') == 'void' else result,
                           'inputs': inputs, 'outputs': outputs}, separators=(',', ':')).encode()
        self.capture['stream'].write(struct.pack('<I', len(data)) + data)
        self.capture['count'] += 1
        return result


def observation_trace(origin, line, record, kind="game_winmain"):
    resolved = str(Path(origin).resolve())
    filenames = {resolved, str(Path(origin)), os.path.relpath(resolved)}
    observed_cases = set()
    def trace(frame, event, arg):
        if frame.f_code.co_filename not in filenames:
            return None
        if event == 'line' and frame.f_lineno == line and frame.f_code.co_name == 'main':
            values = frame.f_locals
            if kind == 'stream_players':
                record({'source': values['e'], 'native_comparison': values['got']})
            else:
                # Python 3.14 inlines comprehensions: repeated line events occur in main.
                case = values['n']
                if case in observed_cases:
                    return trace
                observed_cases.add(case)
                record({'case': case, 'source': values['e'],
                        'source_trace': values['st']['trace'], 'native_comparison': values['got_cmp']})
        return trace
    return trace


def main():
    slug, library, destination = sys.argv[1:4]
    source_root = Path(sys.argv[4]) if len(sys.argv) == 5 else Path('.')
    output = new_destination(Path(destination))
    contracts = json.loads(Path(__file__).with_name('CAPTURE-CONTRACTS.json').read_text())
    contract = contracts[slug]
    script = source_root / 'tools/oracles' / f'verify_original_{slug}.py'
    spec = importlib.util.spec_from_file_location('reset_capture_judge', script)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    abi = gzip.GzipFile(filename=str(output / f'{slug}.abi.gz'), mode='wb', mtime=0)
    observations = gzip.GzipFile(filename=str(output / f'{slug}.original.gz'), mode='wb', mtime=0)
    capture = {'stream': abi, 'count': 0, 'sequence': [], 'inputs': hashlib.sha256(),
               'contract': contract}
    real_cdll = ctypes.CDLL

    class Library:
        def __init__(self, *args, **kwargs):
            self.library = real_cdll(*args, **kwargs)

        def __getattr__(self, name):
            return Function(getattr(self.library, name), capture)

    observed = []
    def record(value):
        value.setdefault('case', len(observed))
        observed.append(value['case'])
        observations.write((json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n').encode())

    for value in list(vars(module).values()):
        if isinstance(value, type) and value.__module__ == module.__name__ and hasattr(value, 'run'):
            original = value.run

            def run(self, *args, _original=original, **kwargs):
                result = _original(self, *args, **kwargs)
                record({'class': type(self).__name__, 'input': args, 'result': result})
                return result

            value.run = run

    # WinMain's original instruction observation is inline rather than in Source.run.
    if slug in ('game_winmain', 'stream_players'):
        lines = script.read_text().splitlines()
        marker = 'd=[i for i in range(256)' if slug == 'game_winmain' else 'if got!=e:'
        line = next(i + 1 for i, text in enumerate(lines) if marker in text)

        sys.settrace(observation_trace(spec.origin, line, record, slug))
    ctypes.CDLL = Library
    sys.argv = [str(script), '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe',
                library, '--output', str(output / f'{slug}.json')]
    try:
        status = module.main()
    finally:
        sys.settrace(None)
        abi.close()
        observations.close()
        ctypes.CDLL = real_cdll
    validate_domain(capture, observed, contract)
    receipt = {'slug': slug, 'exit': int(status), 'calls': capture['count'], 'schema': 4, 'original_observations': len(observed),
               'source_git_base': __import__('subprocess').check_output(['git','rev-parse','HEAD'],text=True).strip(),
               'helper_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               'contract_sha256': hashlib.sha256(Path(__file__).with_name('CAPTURE-CONTRACTS.json').read_bytes()).hexdigest(),
               'comparison_old_base': contract['immutable_old_base'],
               'compiled_model_sha256': hashlib.sha256(Path(library).read_bytes()).hexdigest(),
               'executed_judge_sha256': hashlib.sha256(script.read_bytes()).hexdigest(),
               'source_freeze': {str(p.relative_to(source_root)):hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in (source_root / 'src').rglob('*') if p.suffix in ('.cpp','.hpp','.mm')},
               'files': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in output.glob(f'{slug}.*')},
               'limits': 'Complete default native ABI and original observations; no reserved inputs.'}
    (output / f'{slug}.capture.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt))
    return status


if __name__ == '__main__':
    raise SystemExit(main())
