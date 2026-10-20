#!/usr/bin/env python3
"""One-time audio import. Runtime uses the emitted WAVs, never FFmpeg or original code.

DAT layout evidence: X-Hax/sa_tools Libraries/ArchiveLib/DAT.cs (archive V2.2),
tree b72d52f37c21a2f7215711e3516360a63aea12da.
WMA conversion requires an explicit local FFmpeg executable; nothing is installed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import wave


def dat_entries(data):
    if data[:16] != b"archive  V2.2\0\0\0" or len(data) < 20:
        raise ValueError("unsupported DAT header")
    count, = struct.unpack_from("<I", data, 16)
    table_end = 20 + count * 12
    if table_end > len(data):
        raise ValueError("truncated DAT table")
    entries = []
    for index in range(count):
        name_at, offset, size = struct.unpack_from("<III", data, 20 + index * 12)
        if not table_end <= name_at < len(data) or offset > len(data) or size > len(data) - offset:
            raise ValueError("DAT entry outside archive")
        end = data.find(b"\0", name_at)
        if end < 0:
            raise ValueError("unterminated DAT name")
        name = data[name_at:end].decode("ascii")
        if Path(name).name != name or "\\" in name or name in ("", ".", ".."):
            raise ValueError("unsafe DAT name")
        payload = data[offset:offset + size]
        if payload[:4] != b"RIFF" or payload[8:12] != b"WAVE":
            raise ValueError(f"compressed or unsupported DAT entry: {name}")
        if len(payload) < 12 or struct.unpack_from("<I", payload, 4)[0] + 8 != len(payload):
            raise ValueError("invalid RIFF length")
        entries.append((index, name, payload))
    return entries


def validate_wave(path):
    with wave.open(str(path), "rb") as source:
        if source.getcomptype() != "NONE" or source.getsampwidth() != 2:
            raise ValueError("import requires 16-bit PCM")
        if source.getnchannels() not in (1, 2) or source.getframerate() <= 0 or source.getnframes() == 0:
            raise ValueError("invalid PCM dimensions")
        frames = source.getnframes()
        if len(source.readframes(frames)) != frames * source.getnchannels() * 2:
            raise ValueError("truncated PCM payload")
        return {"frames": frames, "channels": source.getnchannels(), "rate": source.getframerate()}


def import_audio(source, destination, selections, ffmpeg=None):
    source, destination = Path(source).resolve(), Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    records = []
    for selection in selections:
        path = (source / selection).resolve()
        if not path.is_relative_to(source):
            raise ValueError("source outside sounddata")
        relative = path.relative_to(source)
        data = path.read_bytes()
        source_hash = hashlib.sha256(data).hexdigest()
        if path.suffix.lower() == ".dat":
            entries = [(relative.with_suffix("") / name.lower(), payload, index)
                       for index, name, payload in dat_entries(data)]
        elif path.suffix.lower() == ".wma":
            if not ffmpeg:
                raise ValueError("WMA needs --ffmpeg /path/to/ffmpeg at import time")
            output = relative.parent.parent / (path.stem.lower() + ".wav")
            with tempfile.TemporaryDirectory() as temporary:
                converted = Path(temporary) / "decoded.wav"
                subprocess.run([str(ffmpeg), "-nostdin", "-v", "error", "-i", str(path),
                                "-map", "0:a:0", "-c:a", "pcm_s16le", str(converted)], check=True)
                entries = [(output, converted.read_bytes(), None)]
        else:
            raise ValueError(f"unsupported audio asset: {relative}")
        for output, payload, index in entries:
            target = destination / output
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists() and target.read_bytes() != payload:
                raise ValueError(f"conflicting imported asset: {output}")
            target.write_bytes(payload)
            facts = validate_wave(target)
            records.append({"source": relative.as_posix(), "source_sha256": source_hash,
                            "entry": index, "path": output.as_posix(),
                            "sha256": hashlib.sha256(payload).hexdigest(), **facts})
    manifest = destination / "manifest.json"
    previous = json.loads(manifest.read_text()) if manifest.exists() else {"schema": 1, "assets": []}
    by_path = {row["path"]: row for row in previous["assets"]}
    by_path.update({row["path"]: row for row in records})
    manifest.write_text(json.dumps({"schema": 1, "assets": sorted(by_path.values(), key=lambda x: x["path"])},
                                   indent=2) + "\n")
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="installation system/sounddata")
    parser.add_argument("destination", type=Path)
    parser.add_argument("assets", nargs="+", help="relative DAT or WMA paths")
    parser.add_argument("--ffmpeg", type=Path)
    args = parser.parse_args()
    records = import_audio(args.source, args.destination, args.assets, args.ffmpeg)
    print(json.dumps({"imported": len(records), "destination": str(args.destination)}))


if __name__ == "__main__":
    main()
