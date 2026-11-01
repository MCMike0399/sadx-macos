"""Replay every frozen original input through a supplied native C ABI library.

This preserves the entire recorded source/native stream and compares all words,
including FP status and every failure prefix. It does not rerun original code.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("slug", choices=["player_animation", "player_animation_blend", "sonic_animation_table"])
    parser.add_argument("library", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--constants", type=pathlib.Path)
    args = parser.parse_args()
    baseline = json.loads((ROOT / "docs/refactor-inventory/packets/animation-judges/current-baseline.json").read_text())[args.slug]
    frozen = pathlib.Path(baseline["stream_local_path"])
    assert hashlib.sha256(frozen.read_bytes()).hexdigest() == baseline["stream_file_sha256"]
    args.output.mkdir(parents=True, exist_ok=True)
    table = args.slug == "sonic_animation_table"
    library = ctypes.CDLL(str(args.library.resolve()))
    probe = getattr(library, "sadx_native_" + args.slug)
    probe.restype = ctypes.c_int
    if table:
        probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                          ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
    else:
        raw = args.constants.read_bytes()
        assert hashlib.sha256(raw).hexdigest() == json.loads((ROOT / "docs/refactor-inventory/packets/animation-judges/current-baseline.json").read_text())["player_animation"]["report"]["constants"]["sha256"]
        constants = (ctypes.c_float * 3).from_buffer_copy(raw)
        probe.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_uint32),
                          ctypes.POINTER(ctypes.c_uint32)]
    differences = []
    count = 0
    with frozen.open() as source, (args.output / "cases.jsonl").open("w") as output:
        for line in source:
            record = json.loads(line)
            inputs = (ctypes.c_uint32 * len(record["input"]))(*record["input"])
            result = ((ctypes.c_uint8 if table else ctypes.c_uint32) * len(record["native"]))()
            status = probe(inputs, len(inputs), result, len(result)) if table else probe(constants, inputs, result)
            if status != record["native_return"] or list(result) != record["native"]:
                differences.append(dict(case=record["case"], expected=record["native"], actual=list(result),
                                        expected_return=record["native_return"], actual_return=status))
            record["native"] = list(result)
            record["native_return"] = status
            output.write(json.dumps(record, separators=(",", ":")) + "\n")
            count += 1
    stream = args.output / "cases.jsonl"
    receipt = dict(cases=count, mismatch_count=len(differences), differences=differences,
                   frozen_stream_sha256=baseline["stream_file_sha256"],
                   current_stream_sha256=hashlib.sha256(stream.read_bytes()).hexdigest(),
                   library_sha256=hashlib.sha256(args.library.read_bytes()).hexdigest(),
                   preserved_identity_indexes=baseline["identity_index"] if not differences else None,
                   inherited_verdict=baseline["verdict"], original_code_rerun=False, holdouts_run=False)
    (args.output / "replay.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({k: v for k, v in receipt.items() if k not in ("differences", "preserved_identity_indexes")}))
    return int(bool(differences))


if __name__ == "__main__":
    raise SystemExit(main())
