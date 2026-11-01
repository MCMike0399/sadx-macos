"""Source-instruction-backed initializer missingness and retained-prefix judge.

Unavailable fixture facts stop before the corresponding original read. They do
not assert that Windows can execute absent memory. No generated recipe supplies
expected values: UC memory hooks observe the pinned original producer.
"""
import argparse
import ctypes
import hashlib
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / "tools/lib"), str(ROOT / "tools/oracles")]
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original, DATA, STACK, STOP

TABLE = 0x3C56210
EXPORT = DATA + 0x1000
OFFSETS = (0, 4, 5, 6, 8, 12)
WIDTHS = (4, 1, 1, 2, 4, 4)
WORDS = 3 + 147 * 13


def trace(image, null_ordinal=0xFFFFFFFF):
    original = Original(image, 0x7F)
    machine = original.machine
    machine.mem_map(0x38F6000, 0x1000)
    machine.mem_map(0x3C56000, 0x2000)
    machine.mem_write(0x38F6E7C, struct.pack("<I", EXPORT))
    machine.mem_write(EXPORT, struct.pack("<149I", *[
        0 if index == null_ordinal else 0x21000000 + (index % 7) * 32
        for index in range(149)]))
    events = []

    def read(m, access, address, size, value, user):
        if EXPORT <= address < EXPORT + 149 * 4:
            assert size == 4 and (address - EXPORT) % 4 == 0
            events.append(dict(kind="read", pc=m.reg_read(UC_X86_REG_EIP),
                               ordinal=(address - EXPORT) // 4))

    def write(m, access, address, size, value, user):
        if TABLE <= address < TABLE + 147 * 16:
            row, offset = divmod(address - TABLE, 16)
            field = OFFSETS.index(offset)
            assert WIDTHS[field] == size
            events.append(dict(kind="write", pc=m.reg_read(UC_X86_REG_EIP),
                               row=row, field=field, value=value & ((1 << (size * 8)) - 1)))

    machine.hook_add(unicorn.UC_HOOK_MEM_READ, read)
    machine.hook_add(unicorn.UC_HOOK_MEM_WRITE, write)
    machine.mem_write(STACK, struct.pack("<I", STOP))
    machine.reg_write(UC_X86_REG_ESP, STACK)
    machine.emu_start(0x7CEC90, STOP, count=5000)
    assert machine.reg_read(UC_X86_REG_EIP) == STOP
    return events


def expected(events, fixture):
    missing, size, mask, seeded, _, source_state = fixture
    rows = []
    touched = [0] * 147
    for index in range(147):
        values = [0x31000000 + index * 32, (index + 81) & 255,
                  (index + 17) & 255, 0x8100 + index,
                  0x3E000000 + index, 0x3F000000 + index]
        rows.append([int(bool(seeded))] + [part for field, value in enumerate(values)
                     for part in ((2 if field == 0 else 1) if seeded and mask & (1 << field) else 0,
                                  value if seeded and mask & (1 << field) else 0)])
    failure, boundary = (1, 0x7CEC90) if source_state == 1 else (0, 0)
    if not source_state:
        for event in events:
            if event["kind"] == "read":
                if event["ordinal"] == missing or event["ordinal"] >= size:
                    failure, boundary = 1, event["pc"]
                    break
            else:
                row, field, value = event["row"], event["field"], event["value"]
                rows[row][0] = 1
                rows[row][1 + field * 2:3 + field * 2] = [2 if field == 0 and value else 1, value]
                touched[row] |= 1 << field
    elif source_state == 2:
        failure, boundary = 2, next(event["pc"] for event in events if event["kind"] == "read")
    return [failure, boundary, touched.count(63)] + [word for row in rows for word in row]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("library", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    image = PEImage(args.exe)
    assert image.sha256 == SOURCE_SHA256 and unicorn.__version__ == "2.1.4"
    events = trace(image)
    (args.output / "source-events.json").write_text(json.dumps(events, indent=2) + "\n")
    metadata = json.loads((ROOT / "reference/sonic-animation-operations.json").read_text())["operations_detail"]
    assert len(events) == len(metadata) == 1027
    touched = [0] * 147
    reads = []
    loaded = {}
    for event, operation in zip(events, metadata):
        assert event["pc"] == operation["address"]
        assert (event["kind"] == "read") == operation["read"]
        if event["kind"] == "read":
            assert event["ordinal"] == operation["index"]
            assert operation["slot"] == len(reads), "load slot retains its value-index meaning"
            loaded[operation["slot"]] = 0x21000000 + (event["ordinal"] % 7) * 32
            assert all(mask in (0, 63) for mask in touched), "Incomplete new row at export read"
            reads.append(event["ordinal"])
        else:
            assert event["row"] == operation["record"]
            assert OFFSETS[event["field"]] == operation["field"]
            if event["field"] == 0:
                assert event["value"] == (0 if operation["slot"] == 65535 else loaded[operation["slot"]])
            else:
                assert operation["slot"] == 65535
                assert event["value"] == operation["bits"] & ((1 << (WIDTHS[event["field"]] * 8)) - 1)
            touched[event["row"]] |= 1 << event["field"]
    assert touched == [63] * 147
    library = ctypes.CDLL(str(args.library.resolve()))
    probe = library.sadx_animation_table_characterization
    probe.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32)]
    probe.restype = ctypes.c_int
    fixtures = []
    sentinel = 0xFFFFFFFF
    for seeded in (0, 1):
        for mask in (range(64) if seeded else (0,)):
            for ordinal in sorted(set(reads)):
                fixtures.append([ordinal, 149, mask, seeded, sentinel, 0])
            fixtures.extend([[sentinel, 149, mask, seeded, sentinel, state] for state in (0, 1, 2)])
    for size in range(150):
        fixtures.append([sentinel, size, 63, 1, sentinel, 0])
    # Each known-null export runs original instructions; null is a known value.
    null_traces = {ordinal: trace(image, ordinal) for ordinal in sorted(set(reads))}
    fixtures.extend([[sentinel, 149, 63, 1, ordinal, 0] for ordinal in null_traces])
    mismatches = []
    stream = args.output / "cases.jsonl"
    with stream.open("w") as output:
        for case, fixture in enumerate(fixtures):
            actual = (ctypes.c_uint32 * WORDS)()
            assert probe((ctypes.c_uint32 * 6)(*fixture), actual) == 0
            source_prefix = expected(null_traces.get(fixture[4], events), fixture)
            record = dict(case=case, input=fixture, source_backed_prefix=source_prefix, native=list(actual))
            output.write(json.dumps(record, separators=(",", ":")) + "\n")
            if list(actual) != source_prefix:
                mismatches.append(dict(case=case, input=fixture,
                                       differing_words=[i for i, pair in enumerate(zip(actual, source_prefix)) if pair[0] != pair[1]]))
    report = dict(cases=len(fixtures), mismatches=mismatches, passed=not mismatches,
                  source_reads=len(reads), distinct_ordinals=len(set(reads)), stores=882,
                  rows=147, incomplete_new_rows_at_read=0,
                  stream_sha256=hashlib.sha256(stream.read_bytes()).hexdigest(),
                  limits=["Unknown availability is projected onto observed source write prefixes; absent fixture memory is never executed as original memory",
                          "Seed masks represent arbitrary fixtures, not native BSS initialization",
                          "No final holdout used"])
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k not in ("mismatches", "limits")}))
    return int(bool(mismatches))


if __name__ == "__main__":
    raise SystemExit(main())
