"""Observe unchanged stock verifiers; retain every input, native word and source result.

The wrapper delegates every native/original call unchanged. It does not extend
fixture coverage or turn native diagnostics into original-memory execution.
"""
import argparse
import ctypes
import hashlib
import importlib
import json
import pathlib
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("slug", choices=("sonic_animation_table", "player_animation", "player_animation_blend"))
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("library", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[3]
    sys.path[:0] = [str(root / "tools/oracles"), str(root / "tools/lib")]
    module = importlib.import_module("verify_original_" + args.slug)
    args.output.mkdir(parents=True, exist_ok=True)
    pending = {}
    records = []
    table = args.slug == "sonic_animation_table"
    if not table:
        original_class = module.AnimationOriginal if args.slug == "player_animation" else module.BlendOriginal
        original_run = original_class.run

        def observe_original(self, fixture):
            result = original_run(self, fixture)
            pending["source"] = result
            return result

        original_class.run = observe_original
    else:
        original_memory = module.Original

        class ObserveTable(original_memory):
            def __init__(self, *pos, **kw):
                super().__init__(*pos, **kw)
                # Native probe follows source execution. Read source output here,
                # without changing hooks, mapped memory or instruction dispatch.
                pending["machine"] = self.machine

        module.Original = ObserveTable
    real_cdll = ctypes.CDLL

    class Function:
        def __init__(self, function):
            object.__setattr__(self, "function", function)

        def __setattr__(self, name, value):
            setattr(self.function, name, value)

        def __getattr__(self, name):
            return getattr(self.function, name)

        def __call__(self, *pos):
            result = self.function(*pos)
            fixture = list(pos[0] if table else pos[1])
            native = list(pos[2])
            if table:
                machine = pending["machine"]
                source = list(machine.mem_read(module.TABLE, 2352))
            else:
                source = pending.pop("source")
            records.append(dict(case=len(records), input=fixture, source=source,
                                native=native, native_return=result))
            return result

    class Library:
        def __init__(self, *pos, **kw):
            self.library = real_cdll(*pos, **kw)

        def __getattr__(self, name):
            return Function(getattr(self.library, name))

    ctypes.CDLL = Library
    sys.argv = [module.__file__, str(args.exe), str(args.library), "--output", str(args.output / "report.json")]
    try:
        exit_status = module.main()
    finally:
        ctypes.CDLL = real_cdll
    stream = args.output / "cases.jsonl"
    with stream.open("w") as output:
        for record in records:
            output.write(json.dumps(record, separators=(",", ":")) + "\n")
    identities = {"fp_partials": [], "native_boundaries": [], "native_failures": [], "value_failures": []}
    for record in records:
        n, s = record["native"], record["source"]
        identity = dict(case=record["case"], input_sha256=hashlib.sha256(
            json.dumps(record["input"], separators=(",", ":")).encode()).hexdigest())
        if not table:
            animation = args.slug == "player_animation"
            expected = s[0] if animation else s
            status = 9 if animation else 30
            if n[status] != expected[status]:
                identities["fp_partials"].append(dict(identity, source=expected[status], native=n[status]))
            failure, boundary = (11, 12) if animation else (32, 33)
            if n[boundary]:
                identities["native_boundaries"].append(dict(identity, output=n))
            if n[failure]:
                identities["native_failures"].append(dict(identity, output=n))
            if any(n[i] != value for i, value in enumerate(expected) if i != status):
                identities["value_failures"].append(dict(identity, output=n, source=s))
        elif n != s:
            identities["value_failures"].append(identity)
    receipt = dict(slug=args.slug, cases=len(records), verifier_exit_status=exit_status,
                   strict_pass=json.loads((args.output / "report.json").read_text())["passed"],
                   stream_sha256=hashlib.sha256(stream.read_bytes()).hexdigest(), identities=identities)
    (args.output / "identities.json").write_text(json.dumps(receipt, separators=(",", ":")) + "\n")
    print(json.dumps({k: v for k, v in receipt.items() if k != "identities"}))
    return exit_status


if __name__ == "__main__":
    raise SystemExit(main())
