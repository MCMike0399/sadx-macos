"""Capture the unchanged evaluator and full ctypes ABI; never replace comparisons.

Run with the pinned reference Python. Streams are trusted-local pickle+gzip, not
portable frozen corpora. Each call contains exact arrays before and after, return
value, and source.run returns independently. Reports retain evaluator exclusions.
"""
import ctypes
import gzip
import hashlib
import json
import pickle
from pathlib import Path
import runpy
import sys

script, exe, library, directory, *extra = sys.argv[1:]
output = Path(directory)
output.mkdir(parents=True, exist_ok=True)
stream = output / (Path(script).stem + '.abi.pkl.gz')
report = output / (Path(script).stem + '.json')
counts = {'native_calls': 0, 'source_returns': 0}
original_cdll = ctypes.CDLL

def snapshot(value):
    if isinstance(value, ctypes.Array):
        return ('array', value._type_.__name__, len(value), bytes(value))
    if isinstance(value, (bytes, str, int, type(None))):
        return value
    raise TypeError(type(value))

with stream.open('wb') as raw, gzip.GzipFile(fileobj=raw, mode='wb', mtime=0) as data:
    def emit(row):
        pickle.dump(row, data, protocol=4)
    class Function:
        def __init__(self, fn):
            object.__setattr__(self, 'fn', fn)
        def __setattr__(self, key, value):
            setattr(self.fn, key, value)
        def __call__(self, *args):
            before = [snapshot(x) for x in args]
            result = self.fn(*args)
            emit(('native', self.fn.__name__, before, result, [snapshot(x) for x in args]))
            counts['native_calls'] += 1
            return result
    class Library:
        def __init__(self, *args, **kwargs):
            self.lib = original_cdll(*args, **kwargs)
        def __getattr__(self, key):
            return Function(getattr(self.lib, key))
    def profile(frame, event, arg):
        if (event == 'return' and frame.f_code.co_name in ('run', 'initialize', 'checksum')
                and Path(frame.f_code.co_filename).name.startswith(('verify_original_save', 'verify_original_load_selected_save', 'verify_original_file_select_controls', 'verify_original_game_loop'))):
            if isinstance(arg, (list, tuple, bytes)):
                emit(('source-return', frame.f_code.co_filename, arg))
                counts['source_returns'] += 1
    compare_lines = {n for n,line in enumerate(Path(script).read_text().splitlines(),1)
                     if ('expected' in line and line.lstrip().startswith('if '))
                     or line.lstrip().startswith('differences=[')}
    def trace(frame,event,arg):
        if frame.f_code.co_filename != script: return None
        if event == 'line' and frame.f_lineno in compare_lines:
            values = {}
            for key in ('expected','expected_rings','story','saved_rings','lives','v','n','case','index','pattern','rings','values'):
                if key in frame.f_locals:
                    value = frame.f_locals[key]
                    if isinstance(value,(bytes,int,list,tuple,str)): values[key] = value
            emit(('comparison-inputs', frame.f_lineno, values))
        return trace
    ctypes.CDLL = Library
    sys.path[:0] = ['tools/oracles', 'tools/lib']
    sys.argv = [script, exe, library, '--output', str(report), *extra]
    sys.setprofile(profile)
    sys.settrace(trace)
    status = None
    try:
        runpy.run_path(script, run_name='__main__')
        status = 0
    except SystemExit as error:
        status = int(error.code or 0)
    finally:
        sys.setprofile(None)
        sys.settrace(None)
        ctypes.CDLL = original_cdll
summary = {'exit': status, **counts, 'stream': str(stream),
           'stream_sha256': hashlib.sha256(stream.read_bytes()).hexdigest(),
           'stream_bytes': stream.stat().st_size,
           'library_sha256': hashlib.sha256(Path(library).read_bytes()).hexdigest(),
           'evaluator_sha256': hashlib.sha256(Path(script).read_bytes()).hexdigest(),
           'report': json.loads(report.read_text())}
(output / (Path(script).stem + '.receipt.json')).write_text(json.dumps(summary, indent=2)+'\n')
raise SystemExit(status)
