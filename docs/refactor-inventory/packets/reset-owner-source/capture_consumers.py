"""Run unchanged default judges and retain every native ABI buffer and original observation.

Development only: no input generation, filtering, tolerance, expected value or seed changes.
Original methods retain their return/fault behavior. WinMain's local comparison is captured
at its existing comparison line. Binary streams are framed per call and gzip compressed.
"""
import ctypes
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

slug, library, output = sys.argv[1:]
output = Path(output)
output.mkdir(parents=True, exist_ok=True)
script = Path('tools/oracles') / f'verify_original_{slug}.py'
spec = importlib.util.spec_from_file_location('reset_capture_judge', script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
abi = gzip.GzipFile(
    filename=str(output / f'{slug}.abi.gz'), mode='wb', mtime=0)
observations = gzip.GzipFile(filename=str(output / f'{slug}.original.gz'), mode='wb', mtime=0)
count = 0
real_cdll = ctypes.CDLL

class Function:
    def __init__(self, function):
        object.__setattr__(self, 'function', function)
    def __setattr__(self, name, value):
        setattr(self.function, name, value)
    def __getattr__(self, name):
        return getattr(self.function, name)
    def __call__(self, *args):
        global count
        inputs = [bytes(a) for a in args if isinstance(a, ctypes.Array)]
        result = self.function(*args)
        outputs = [bytes(a) for a in args if isinstance(a, ctypes.Array)]
        data = json.dumps({'function': self.function.__name__, 'result': result,
                           'inputs': [a.hex() for a in inputs],
                           'outputs': [a.hex() for a in outputs]}, separators=(',', ':')).encode()
        abi.write(struct.pack('<I', len(data)) + data)
        count += 1
        return result

class Library:
    def __init__(self, *args, **kwargs):
        self.library = real_cdll(*args, **kwargs)
    def __getattr__(self, name):
        return Function(getattr(self.library, name))

def record(value):
    observations.write((json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n').encode())

for value in list(vars(module).values()):
    if isinstance(value, type) and value.__module__ == module.__name__ and hasattr(value, 'run'):
        original = value.run
        def run(self, *args, _original=original, **kwargs):
            result = _original(self, *args, **kwargs)
            record({'class': type(self).__name__, 'input': args, 'result': result})
            return result
        value.run = run

# WinMain performs its instruction observation inline instead of through Source.run.
if slug == 'game_winmain':
    lines = script.read_text().splitlines()
    line = next(i + 1 for i, text in enumerate(lines) if 'd=[i for i in range(256)' in text)
    def trace(frame, event, arg):
        if event == 'line' and frame.f_code.co_filename == str(script) and frame.f_lineno == line:
            values = frame.f_locals
            record({'case': values['n'], 'source': values['e'],
                    'source_trace': values['st']['trace'], 'native_comparison': values['got_cmp']})
        return trace
    sys.settrace(trace)
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
receipt = {'slug': slug, 'exit': int(status), 'calls': count,
           'files': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in output.glob(f'{slug}.*')},
           'limits': 'Complete default native ABI and original observations; no reserved inputs.'}
(output / f'{slug}.capture.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt))
raise SystemExit(status)
