"""Replay complete frozen Sonic ABI observations without altering their judges.

The source/native observations come from the unchanged original verifiers. This
checker validates the entire capture before calling a library and compares every
output word and return, including diagnostics and inherited FP partials.
"""
import argparse
import ctypes
import gzip
import hashlib
import json
from pathlib import Path


def sha(data):
    return hashlib.sha256(data).hexdigest()


def validate(folder, expected=None):
    folder = Path(folder)
    receipt = json.loads((folder / "capture.json").read_text())
    if expected is not None and receipt != expected:
        raise ValueError("Capture receipt differs from the committed baseline anchor")
    encoded = (folder / "cases.jsonl.gz").read_bytes()
    if sha(encoded) != receipt["compressed_sha256"]:
        raise ValueError("Compressed capture identity changed")
    raw = gzip.decompress(encoded)
    if sha(raw) != receipt["full_stream_sha256"] or len(raw) != receipt["raw_bytes"]:
        raise ValueError("Complete capture identity changed")
    rows = [json.loads(line) for line in raw.splitlines()]
    if not rows or len(rows) != receipt["records"]:
        raise ValueError("Empty or truncated capture")
    if [row["case"] for row in rows] != list(range(len(rows))):
        raise ValueError("Missing, duplicated or reordered case identities")
    for row in rows:
        name = row["function"]
        arguments = row["arguments"]
        output = 4 if name == "sadx_native_sonic_action" else 3
        expected_sizes = {"sadx_native_sonic_action": (6, 52, 32),
                          "sadx_native_sonic_player": (4, 36, 40),
                          "sadx_native_sonic_ground_input": (4, 13, 8)}
        if name not in expected_sizes:
            raise ValueError("Unknown observed function")
        arity, inputs, outputs = expected_sizes[name]
        if (len(arguments) != arity or len(arguments[0]) != arguments[1]
                or len(arguments[0]) != 52 or len(arguments[2]) != inputs
                or len(arguments[output]) != outputs or "source" not in row
                or "native_return" not in row):
            raise ValueError("Incomplete observed ABI")
    return receipt, rows


def invoke(library, name, arguments):
    output_index = 4 if name == "sadx_native_sonic_action" else 3
    parameters = []
    types = []
    for index, value in enumerate(arguments):
        if isinstance(value, list):
            scalar = ctypes.c_uint8 if index == 0 else ctypes.c_uint32
            # Match the original verifier's fresh, zero-filled output allocation.
            value = [0] * len(value) if index == output_index else value
            parameters.append((scalar * len(value))(*value))
            types.append(ctypes.POINTER(scalar))
        else:
            parameters.append(value)
            types.append(ctypes.c_size_t)
    function = getattr(library, name)
    function.argtypes = types
    function.restype = ctypes.c_int
    returned = function(*parameters)
    return returned, list(parameters[output_index])


def differences(expected_return, expected_words, actual_return, actual_words):
    if len(expected_words) != len(actual_words):
        raise ValueError("Truncated output array")
    fields = [[index, before, after] for index, (before, after) in
              enumerate(zip(expected_words, actual_words)) if before != after]
    return dict(returned=None if expected_return == actual_return else
                [expected_return, actual_return], words=fields)


def replay(folder, library_path):
    anchor_path = Path(__file__).resolve().parents[3] / "docs/refactor-inventory/packets/sonic-controller-native/BASELINE.json"
    anchors = json.loads(anchor_path.read_text())
    slug = json.loads((Path(folder) / "capture.json").read_text())["slug"]
    receipt, rows = validate(folder, anchors[slug]["capture"])
    library = ctypes.CDLL(str(Path(library_path).resolve()))
    mismatches = []
    digest = hashlib.sha256()
    for row in rows:
        returned, words = invoke(library, row["function"], row["arguments"])
        output = 4 if row["function"] == "sadx_native_sonic_action" else 3
        diff = differences(row["native_return"], row["arguments"][output], returned, words)
        digest.update(json.dumps([row["case"], returned, words], separators=(",", ":")).encode())
        if diff["returned"] or diff["words"]:
            mismatches.append(dict(case=row["case"], function=row["function"], **diff))
    return dict(slug=receipt["slug"], records=len(rows), complete_capture_sha256=receipt["full_stream_sha256"],
                library_sha256=sha(Path(library_path).read_bytes()),
                complete_output_sha256=digest.hexdigest(), differences=mismatches,
                exact=not mismatches)


def supplement(library_path):
    path = Path(__file__).resolve().parents[3] / "tests/fixtures/sonic_controller_supplement.json"
    fixture = json.loads(path.read_text())
    library = ctypes.CDLL(str(Path(library_path).resolve()))
    mismatches = []
    rows = fixture["cases"]
    if [row["case"] for row in rows] != [0, 1]:
        raise ValueError("Incomplete adequacy supplement")
    for row in rows:
        returned, words = invoke(library, row["function"], row["arguments"])
        diff = differences(row["native_return"], row["native_expected"], returned, words)
        if diff["returned"] or diff["words"]:
            mismatches.append(dict(case=row["case"], name=row["name"], **diff))
    return dict(records=len(rows), fixture_sha256=sha(path.read_bytes()),
                differences=mismatches, exact=not mismatches)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("library", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = replay(args.capture, args.library)
    result["supplement"] = (supplement(args.library) if result["slug"] == "sonic_player"
                            else dict(records=0, differences=[], exact=True,
                                      scope="player-only supplement does not apply to action ABI"))
    result["exact"] = result["exact"] and result["supplement"]["exact"]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({key: value for key, value in result.items() if key != "differences"}))
    return int(not result["exact"])


if __name__ == "__main__":
    raise SystemExit(main())
