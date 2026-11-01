#!/usr/bin/env python3
"""Keep the original mode ABI exact and characterize its full diagnostic companion.

The companion adds adapter observations; original instructions still judge only
the old 256 words. The fixed original input generator and failure fixtures are
imported without changing their inputs, comparisons or expectations.
"""
import argparse
import ctypes
import gzip
import hashlib
import json
from pathlib import Path
import struct

from verify_original_game_mode import ARGC, fixtures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    destination = args.output / "diagnostics.bin.gz"
    if destination.exists():
        parser.error("Refusing to overwrite frozen diagnostic observations")
    library = ctypes.CDLL(str(args.library.resolve()))
    legacy = library.sadx_native_game_mode
    companion = library.sadx_native_game_mode_diagnostics
    signature = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t,
                 ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
    for function in (legacy, companion):
        function.argtypes, function.restype = signature, ctypes.c_int
    input_hash, old_hash, new_hash = (hashlib.sha256() for _ in range(3))
    outcome_ids = {}
    rows = fixtures()
    with gzip.open(args.baseline, "rb") as frozen, gzip.open(destination, "wb") as capture:
        for index, row in enumerate(rows):
            original_record = frozen.read(1220)
            if len(original_record) != 1220:
                raise RuntimeError(f"Missing frozen record {index}")
            expected = struct.unpack("<I48I256I", original_record)
            if expected[0] != 1 or list(expected[1:49]) != row:
                raise RuntimeError(f"Frozen fixture identity changed at {index}")
            source = (ctypes.c_uint32 * 48)(*row)
            old = (ctypes.c_uint32 * 256)()
            full = (ctypes.c_uint32 * 264)()
            if legacy(source, 48, old, 256) != 1 or companion(source, 48, full, 264) != 1:
                raise RuntimeError(f"Observer rejected original input {index}")
            if tuple(old) != expected[49:] or tuple(full[:256]) != tuple(old):
                raise RuntimeError(f"Original ABI output changed at {index}")
            if old[0] == 3:  # Existing fixture fails its reached owner at call N.
                last = 14 + 5 * (old[13] - 1)
                callee, site, *values = old[last:last + 5]
                wanted = [291, 1, site, callee, (1 << ARGC.get(callee, 0)) - 1, *values]
            else:
                wanted = [0] * 8
            if list(full[256:]) != wanted:
                raise RuntimeError(f"Nested/pending diagnostic payload changed at {index}")
            encoded_input = struct.pack("<48I", *row)
            encoded_full = encoded_input + struct.pack("<264I", *full)
            input_hash.update(encoded_input)
            old_hash.update(original_record)
            new_hash.update(encoded_full)
            capture.write(encoded_full)
            key = f"failure{old[0]}:pending{full[257]}:argmask{full[260]}"
            outcome_ids.setdefault(key, []).append(index)
        if frozen.read(1):
            raise RuntimeError("Frozen stream has extra records")
    report = {
        "records": len(rows), "original_abi_equal": True,
        "all_diagnostic_payloads_match_existing_fixture_contract": True,
        "input_sha256": input_hash.hexdigest(),
        "old_stream_sha256": old_hash.hexdigest(),
        "diagnostic_stream_sha256": new_hash.hexdigest(),
        "diagnostic_stream_record_bytes": 1248,
        "library_sha256": hashlib.sha256(args.library.read_bytes()).hexdigest(),
        "outcome_ids": outcome_ids,
        "limits": [
            "Pending availability/payload and nested receipt boundary are adapter contracts.",
            "Only the unchanged original oracle establishes original instruction equivalence.",
            "Missing owner/return and other missing facts are separately covered by the direct diagnostic unit.",
        ],
    }
    (args.output / "report.json").write_text(json.dumps(report, separators=(",", ":")) + "\n")
    print(f"{len(rows)} original ABI records exact; full diagnostic payloads characterized")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
