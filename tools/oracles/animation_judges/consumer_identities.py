"""Index every FP partial and native failure/boundary in full consumer captures."""
import argparse
import gzip
import hashlib
import json
import pathlib


def digest(value):
    return hashlib.sha256(json.dumps(value, separators=(",", ":"), sort_keys=True).encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=pathlib.Path)
    args = parser.parse_args()
    capture = json.loads((args.directory / "capture.json").read_text())
    identities = dict(fp_partials=[], native_boundaries=[], native_failures=[])
    stream = gzip.decompress((args.directory / "cases.jsonl.gz").read_bytes())
    assert hashlib.sha256(stream).hexdigest() == capture["full_stream_sha256"]
    for line in stream.splitlines():
        row = json.loads(line)
        a, source = row["arguments"], row["source"]["result"]
        slug = capture["slug"]
        if slug == "gamma_selection_core":
            fixture, native, meta, source_meta = [a[0], a[2]], a[3], a[5], source[1]
            flags = source_meta[2], meta[2]
            boundary, failure = meta[1], row["native_return"]
        elif slug == "player_startup":
            fixture, native, meta, source_meta = [a[4], a[6], a[2]], [a[7], a[9]], a[7], source
            flags = source["flags"], meta[7]
            boundary, failure = meta[1], meta[0]
        else:
            fixture, native, meta, source_meta = a[2], a[3], a[3], source
            boundary, failure = meta[1], meta[0]
            if row["source"]["class_name"] == "GroundOriginal":
                flags = source[0][1], meta[4]
            else:
                flags = source[0]["17"], meta[17]
        identity = dict(case=row["case"], input_sha256=digest(fixture),
                        native_output_sha256=digest(native), source_output_sha256=digest(source))
        if flags[0] != flags[1]:
            identities["fp_partials"].append(dict(identity, source=flags[0], native=flags[1]))
        if boundary:
            identities["native_boundaries"].append(dict(identity, boundary=boundary))
        if failure:
            identities["native_failures"].append(dict(identity, failure=failure, boundary=boundary))
    receipt = dict(capture=capture, identities=identities)
    (args.directory / "identities.json").write_text(json.dumps(receipt, separators=(",", ":")) + "\n")
    print(json.dumps(dict(slug=capture["slug"], counts={key: len(value) for key, value in identities.items()})))


if __name__ == "__main__":
    main()
