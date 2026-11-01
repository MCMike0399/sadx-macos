"""Observe stock Sonic/startup/Gamma verifiers with their complete old cohorts.

All ctypes arguments are retained after each call (including full output arrays),
alongside the unchanged original run's return. No holdout option is exposed.
"""
import argparse
import ctypes
import gzip
import hashlib
import importlib
import json
import pathlib
import sys


def encode(value):
    if isinstance(value, (bytes, bytearray)):
        return {"bytes_hex": bytes(value).hex()}
    raise TypeError(type(value).__name__)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("slug", choices=["sonic_player", "player_startup", "gamma_selection_core"])
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("library", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[3]
    sys.path[:0] = [str(root / "tools/oracles"), str(root / "tools/lib")]
    module = importlib.import_module("verify_original_" + args.slug)
    args.output.mkdir(parents=True, exist_ok=True)
    pending = {}
    for name, value in list(vars(module).items()):
        if isinstance(value, type) and value.__module__ == module.__name__ and hasattr(value, "run"):
            original = value.run

            def observe(self, *pos, _original=original, _name=name, **kw):
                result = _original(self, *pos, **kw)
                pending["source"] = dict(class_name=_name, result=result)
                return result

            value.run = observe
    real_cdll = ctypes.CDLL
    records = []

    class Function:
        def __init__(self, function, name):
            object.__setattr__(self, "function", function)
            object.__setattr__(self, "name", name)

        def __setattr__(self, name, value):
            setattr(self.function, name, value)

        def __getattr__(self, name):
            return getattr(self.function, name)

        def __call__(self, *pos):
            result = self.function(*pos)
            arguments = [list(value) if isinstance(value, ctypes.Array) else value for value in pos]
            records.append(dict(case=len(records), function=self.name, arguments=arguments,
                                native_return=result, source=pending.pop("source")))
            return result

    class Library:
        def __init__(self, *pos, **kw):
            self.library = real_cdll(*pos, **kw)

        def __getattr__(self, name):
            return Function(getattr(self.library, name), name)

    ctypes.CDLL = Library
    sys.argv = [module.__file__, str(args.exe), str(args.library), "--output", str(args.output / "report.json")]
    try:
        status = module.main()
    finally:
        ctypes.CDLL = real_cdll
    raw = b"".join((json.dumps(row, separators=(",", ":"), default=encode) + "\n").encode() for row in records)
    stream = args.output / "cases.jsonl.gz"
    stream.write_bytes(gzip.compress(raw, mtime=0))
    receipt = dict(slug=args.slug, records=len(records), verifier_exit_status=status,
                   full_stream_sha256=hashlib.sha256(raw).hexdigest(),
                   compressed_sha256=hashlib.sha256(stream.read_bytes()).hexdigest(),
                   raw_bytes=len(raw), no_holdout=True)
    (args.output / "capture.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt))
    return status


if __name__ == "__main__":
    raise SystemExit(main())
