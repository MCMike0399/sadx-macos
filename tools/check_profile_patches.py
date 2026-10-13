#!/usr/bin/env python3
"""Report ported sonic.exe ranges that overlap a profile-active runtime patch.

Ported native functions are verified against the ORIGINAL unpatched sonic.exe
bytes. Under the pinned updated-speedrun profile the mod loader, Input mod and
Speedrun mod rewrite parts of sonic.exe at runtime, so a ported range that
overlaps an active patch diverges from the reference unless the native code
models that patch (``handled_by`` in profile_patches.json).

Inputs:
  tools/profile_patches.json             patch inventory (see its "conventions")
  build/original-*-comparison.json       oracle reports; the sonic.exe ranges
                                         they declare are read from the keys
                                         original_ranges_sha256,
                                         original_range_sha256, source_ranges
                                         and source_functions (at any depth).

Range keys are "0xSTART-0xEND" (end exclusive), or a start address alone
("0xADDR" or bare hex "ADDR"). A start-only key covers 0x40 bytes unless its
value is an object carrying an integer "size". Addresses >= 0x10000000 (mod
and loader DLLs) are skipped.

Exit status: 1 if any declared range overlaps an active patch whose
handled_by is null, otherwise 0. Patches whose "active" is "undetermined" are
listed separately and do not affect the exit status.
"""
import argparse
import glob
import json
import os
import re
import sys

RANGE_KEYS = ("original_ranges_sha256", "original_range_sha256", "source_ranges", "source_functions")
DEFAULT_SPAN = 0x40
DLL_FLOOR = 0x10000000
HEX = r"(?:0x)?([0-9A-Fa-f]+)"
RANGE_RE = re.compile(rf"^\s*{HEX}\s*-\s*{HEX}\s*$")
START_RE = re.compile(rf"^\s*{HEX}\s*$")


def parse_range(key, value):
    """Return (start, end) for one range key, or None if it is not an address."""
    m = RANGE_RE.match(key)
    if m:
        return int(m.group(1), 16), int(m.group(2), 16)
    m = START_RE.match(key)
    if not m:
        return None
    start = int(m.group(1), 16)
    size = value.get("size") if isinstance(value, dict) else None
    if isinstance(size, int) and size > 0:
        return start, start + size
    return start, start + DEFAULT_SPAN


def collect_ranges(node, out, path=""):
    if isinstance(node, dict):
        for key, value in node.items():
            here = f"{path}/{key}"
            if key in RANGE_KEYS:
                if isinstance(value, dict):
                    for rkey, rvalue in value.items():
                        r = parse_range(rkey, rvalue)
                        if r is not None:
                            out["ranges"].append((r[0], r[1], f"{here}/{rkey}"))
                        else:
                            out["unparsed"].append(f"{here}/{rkey}")
                else:
                    out["opaque"].append(here)
            else:
                collect_ranges(value, out, here)
    elif isinstance(node, list):
        for i, value in enumerate(node):
            collect_ranges(value, out, f"{path}[{i}]")


def load_reports(build_dir):
    reports = []
    for path in sorted(glob.glob(os.path.join(build_dir, "original-*-comparison.json"))):
        with open(path) as fh:
            try:
                doc = json.load(fh)
            except json.JSONDecodeError as exc:
                reports.append({"report": os.path.basename(path), "error": str(exc), "ranges": [], "opaque": [], "unparsed": []})
                continue
        found = {"ranges": [], "opaque": [], "unparsed": []}
        collect_ranges(doc, found)
        exe = [r for r in found["ranges"] if r[0] < DLL_FLOOR]
        found["skipped_dll"] = len(found["ranges"]) - len(exe)
        found["ranges"] = exe
        found["report"] = os.path.basename(path)
        reports.append(found)
    return reports


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--patches", default=os.path.join(here, "profile_patches.json"))
    ap.add_argument("--build-dir", default=os.path.join(here, "..", "build"))
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    args = ap.parse_args()

    with open(args.patches) as fh:
        inventory = json.load(fh)
    patches = []
    for p in inventory["patches"]:
        va = int(p["va"], 16)
        patches.append(dict(p, _start=va, _end=va + int(p["size"])))
    reports = load_reports(args.build_dir)

    unhandled, handled, undetermined = [], [], []
    for rep in reports:
        for start, end, key in rep["ranges"]:
            for p in patches:
                if p["_start"] < end and start < p["_end"]:
                    row = {
                        "report": rep["report"],
                        "range": f"{start:#x}-{end:#x}",
                        "range_key": key,
                        "patch_va": p["va"],
                        "patch_end": f"{p['_end']:#x}",
                        "size": p["size"],
                        "owner": p["owner"],
                        "kind": p["kind"],
                        "target": p["target"],
                        "active": p["active"],
                        "handled_by": p["handled_by"],
                        "source_ref": p["source_ref"],
                    }
                    if p["active"] is True and p["handled_by"] is None:
                        unhandled.append(row)
                    elif p["active"] is True:
                        handled.append(row)
                    elif p["active"] == "undetermined":
                        undetermined.append(row)

    summary = {
        "patches": len(patches),
        "active_patches": sum(1 for p in patches if p["active"] is True),
        "active_unhandled_patches": sum(1 for p in patches if p["active"] is True and p["handled_by"] is None),
        "reports": len(reports),
        "declared_exe_ranges": sum(len(r["ranges"]) for r in reports),
        "unhandled_overlaps": len(unhandled),
        "handled_overlaps": len(handled),
        "undetermined_overlaps": len(undetermined),
        "reports_without_addressed_ranges": [r["report"] for r in reports if not r["ranges"]],
    }
    result = {"summary": summary, "unhandled_overlaps": unhandled, "handled_overlaps": handled,
              "undetermined_overlaps": undetermined,
              "report_ranges": {r["report"]: {"exe_ranges": len(r["ranges"]), "dll_ranges_skipped": r.get("skipped_dll", 0),
                                              "opaque_range_hashes": r["opaque"], "unparsed_keys": r["unparsed"],
                                              **({"error": r["error"]} if "error" in r else {})} for r in reports}}

    if args.json:
        json.dump(result, sys.stdout, indent=1)
        sys.stdout.write("\n")
    else:
        print(f"patch inventory: {summary['patches']} entries, {summary['active_patches']} active, "
              f"{summary['active_unhandled_patches']} active with handled_by=null")
        print(f"reports: {summary['reports']}, declared sonic.exe ranges: {summary['declared_exe_ranges']}")
        if summary["reports_without_addressed_ranges"]:
            print("reports declaring no addressed sonic.exe range (not checked): "
                  + ", ".join(summary["reports_without_addressed_ranges"]))

        def table(title, rows):
            print(f"\n{title}: {len(rows)}")
            if not rows:
                return
            cols = [("report", 44), ("range", 21), ("patch", 21), ("owner", 8), ("kind", 10), ("source_ref / handled_by", 0)]
            print("  ".join(n.ljust(w) for n, w in cols).rstrip())
            for r in rows:
                last = r["handled_by"] or r["source_ref"]
                vals = [r["report"].replace("-comparison.json", ""), r["range"], f"{r['patch_va']}-{r['patch_end']}",
                        r["owner"], r["kind"], f"{last}  [{r['target'][:48]}]"]
                print("  ".join(str(v).ljust(w) for v, (_, w) in zip(vals, cols)).rstrip())

        table("UNHANDLED overlaps with active patches", unhandled)
        table("handled overlaps (native models the patch)", handled)
        table("overlaps with undetermined patches (informational)", undetermined)
    return 1 if unhandled else 0


if __name__ == "__main__":
    sys.exit(main())
