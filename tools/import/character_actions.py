#!/usr/bin/env python3
"""Import startup/title character actions as a typed, pointer-free resource graph.

Schema facts: pinned sa_tools CHRMODELS.ini (___SONIC_ACTIONS, entries100/101),
Ninja NJS_ACTION/OBJECT/MODEL_SADX/MESHSET_SADX/MOTION/MDATA2 declarations.
The source image is read only here; no executable image or source pointers enter
the emitted pack. All embedded references become record IDs (zero is null).
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
from pe_image import PEImage

TITLE_ACTIONS = ((0, "SONIC", (100, 101)), (3, "KNUCKLES", (40, 41)),
                 (7, "BIG", (4, 29, 30)))

SOURCE_SHA = "76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5"
KINDS = {"action": 1, "object": 2, "model": 3, "meshsets": 4,
         "materials": 5, "vectors": 6, "colors": 7, "uv": 8, "indices": 9,
         "attributes": 10, "motion": 11, "tracks": 12, "position_keys": 13,
         "rotation_keys": 14}


def export(image, name):
    pe = image.unpack_offset("<I", 60)[0]
    directory, size = image.unpack_offset("<II", pe + 120)
    if not directory:
        raise ValueError("source has no exports (the mod-loader proxy is not the model archive)")
    fields = image.unpack("<IIHHIIIIIII", image.base + directory)
    _, _, _, _, _, _, function_count, name_count, functions, names, ordinals = fields
    for index in range(name_count):
        name_rva, = image.unpack("<I", image.base + names + index * 4)
        if image.string(image.base + name_rva) != name:
            continue
        ordinal, = image.unpack("<H", image.base + ordinals + index * 2)
        if ordinal >= function_count:
            raise ValueError("invalid export ordinal")
        target, = image.unpack("<I", image.base + functions + ordinal * 4)
        if directory <= target < directory + size:
            raise ValueError("forwarded model export is unsupported")
        return image.base + target
    raise ValueError(f"missing model export {name}")


class Importer:
    def __init__(self, image):
        self.image = image
        self.records = []
        self.ids = {}
        self.active = set()
        self.evidence = []
        self.source_addresses = {}

    def record(self, kind, address, count, build):
        if not address:
            if count:
                raise ValueError(f"null {kind} with nonzero count")
            return 0
        key = (kind, address, count)
        if key in self.active:
            raise ValueError(f"cyclic {kind} graph")
        if key in self.ids:
            return self.ids[key]
        identity = len(self.records) + 1
        self.ids[key] = identity
        self.source_addresses[identity] = address
        self.records.append(None)
        self.active.add(key)
        payload = build()
        self.active.remove(key)
        self.records[identity - 1] = (KINDS[kind], payload)
        self.evidence.append({"id": identity, "kind": kind, "source_address": hex(address),
                              "count": count, "payload_sha256": hashlib.sha256(payload).hexdigest()})
        return identity

    def array(self, kind, address, count, stride):
        if count < 0 or count > 1_000_000:
            raise ValueError(f"invalid {kind} count")
        if not address:
            return 0
        return self.record(kind, address, count, lambda: struct.pack("<I", count) +
                           self.image.read(address, count * stride))

    def object(self, address):
        if not address:
            return 0

        def build():
            source = bytearray(self.image.read(address, 52))
            model, = struct.unpack_from("<I", source, 4)
            child, sibling = struct.unpack_from("<II", source, 44)
            struct.pack_into("<I", source, 4, self.model(model))
            struct.pack_into("<II", source, 44, self.object(child), self.object(sibling))
            return bytes(source)
        return self.record("object", address, 1, build)

    def animated_objects(self, address, ancestors=()):
        if not address:
            return 0
        if address in ancestors:
            raise ValueError("cyclic object animation traversal")
        flags, = self.image.unpack("<I", address)
        child, sibling = self.image.unpack("<II", address + 44)
        return (int(not flags & 64) +
                self.animated_objects(child, ancestors + (address,)) +
                self.animated_objects(sibling, ancestors + (address,)))

    def model(self, address):
        if not address:
            return 0

        def build():
            source = bytearray(self.image.read(address, 44))
            points, normals, count, meshes, materials, mesh_count, material_count = \
                struct.unpack_from("<IIiIIHH", source)
            if count < 0 or (count and not points) or (material_count and not materials):
                raise ValueError("invalid model vertices")
            if struct.unpack_from("<I", source, 40)[0] != 0:
                raise ValueError("model contains an unexpected runtime buffer")
            struct.pack_into("<II", source, 0, self.array("vectors", points, count, 12),
                             self.array("vectors", normals, count, 12))
            struct.pack_into("<II", source, 12, self.meshsets(meshes, mesh_count, count),
                             self.array("materials", materials, material_count, 20))
            return bytes(source)
        return self.record("model", address, 1, build)

    def meshsets(self, address, count, vertex_count):
        def build():
            result = bytearray(struct.pack("<I", count))
            for index in range(count):
                source = bytearray(self.image.read(address + index * 28, 28))
                type_material, polygons, indices, attributes, normals, colors, uv, buffer = \
                    struct.unpack("<HHIIIIII", source)
                if buffer:
                    raise ValueError("mesh contains an unexpected runtime buffer")
                kind = type_material >> 14
                cursor, corners = indices, 0
                for _ in range(polygons):
                    if kind < 2:
                        size = 3 if kind == 0 else 4
                    else:
                        header, = self.image.unpack("<H", cursor)
                        size = header & 32767 if kind == 3 else header
                        cursor += 2
                    if size < 3:
                        raise ValueError("invalid primitive corner count")
                    values = self.image.unpack("<" + "H" * size, cursor)
                    if any(value >= vertex_count for value in values):
                        raise ValueError("mesh vertex index outside model")
                    cursor += size * 2
                    corners += size
                ids = [self.array("indices", indices, (cursor - indices) // 2, 2),
                       self.array("attributes", attributes, polygons, 4),
                       self.array("vectors", normals, polygons, 12),
                       self.array("colors", colors, corners, 4),
                       self.array("uv", uv, corners, 4)]
                struct.pack_into("<IIIII", source, 4, *ids)
                result.extend(source)
            return bytes(result)
        return self.record("meshsets", address, count, build)

    def motion(self, address, parts):
        def build():
            data, frames, channels, interpolation = self.image.unpack("<IIHH", address)
            if channels != 3 or interpolation & 15 != 2:
                raise ValueError("startup motion must contain position/rotation MDATA2")
            tracks = self.tracks(data, parts)
            return struct.pack("<IIHHI", tracks, frames, channels, interpolation, parts)
        return self.record("motion", address, parts, build)

    def tracks(self, address, count):
        def build():
            result = bytearray(struct.pack("<I", count))
            for index in range(count):
                position, rotation, positions, rotations = self.image.unpack(
                    "<IIII", address + index * 16)
                if (positions and not position) or (rotations and not rotation):
                    raise ValueError("missing animation key array")
                result.extend(struct.pack("<IIII",
                    self.array("position_keys", position, positions, 16),
                    self.array("rotation_keys", rotation, rotations, 16), positions, rotations))
            return bytes(result)
        return self.record("tracks", address, count, build)

    def action(self, address):
        def build():
            root, motion = self.image.unpack("<II", address)
            if not root or not motion:
                raise ValueError("missing startup action resource")
            return struct.pack("<II", self.object(root),
                               self.motion(motion, self.animated_objects(root)))
        return self.record("action", address, 1, build)

    def write(self, output, actions, title=False):
        self.verify_round_trip()
        body = bytearray(b"SDXACT02" if title else b"SDXACT01")
        body.extend(struct.pack("<II", len(self.records), len(actions)))
        for descriptor in actions:
            body.extend(struct.pack("<III" if title else "<II", *descriptor))
        for kind, payload in self.records:
            body.extend(struct.pack("<II", kind, len(payload)))
            body.extend(payload)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(body)
        return hashlib.sha256(body).hexdigest()

    def verify_round_trip(self):
        """Restore every reference and compare each record with its original source bytes."""
        by_id = {row["id"]: row for row in self.evidence}
        for identity, (_, payload) in enumerate(self.records, 1):
            row = by_id[identity]
            kind = row["kind"]
            if kind in ("action", "object", "model", "motion"):
                restored = bytearray(payload[:12] if kind == "motion" else payload)
                offsets = {"action": (0, 4), "object": (4, 44, 48),
                           "model": (0, 4, 12, 16), "motion": (0,)}[kind]
            else:
                restored = bytearray(payload[4:])
                if kind == "meshsets":
                    offsets = tuple(index * 28 + field for index in range(row["count"])
                                    for field in (4, 8, 12, 16, 20))
                elif kind == "tracks":
                    offsets = tuple(index * 16 + field for index in range(row["count"])
                                    for field in (0, 4))
                else:
                    offsets = ()
            for offset in offsets:
                resource, = struct.unpack_from("<I", restored, offset)
                address = self.source_addresses[resource] if resource else 0
                struct.pack_into("<I", restored, offset, address)
            original = self.image.read(int(row["source_address"], 16), len(restored))
            if bytes(restored) != original:
                raise ValueError(f"{kind} record {identity} failed source-byte roundtrip")
            row["source_size"] = len(restored)
            row["source_sha256"] = hashlib.sha256(original).hexdigest()
            row["round_trip_exact"] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model_dll", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--title", action="store_true", help="one combined Sonic/Knuckles/Big title pack")
    args = parser.parse_args()
    image = PEImage(args.model_dll)
    if image.sha256 != SOURCE_SHA:
        raise ValueError("unsupported character archive SHA256")
    importer = Importer(image)
    if args.title:
        actions = []
        for character, name, indices in TITLE_ACTIONS:
            table = export(image, "___" + name + "_ACTIONS")
            for index in indices:
                identity = importer.action(image.unpack("<I", table + index * 4)[0])
                actions.append((character, index, identity))
    else:
        table = export(image, "___SONIC_ACTIONS")
        actions = [(index, importer.action(image.unpack("<I", table + index * 4)[0]))
                   for index in (100, 101)]
    digest = importer.write(args.output, actions, title=args.title)
    evidence = {"schema": "SDXACT02" if args.title else "SDXACT01", "source_sha256": image.sha256, "pack_sha256": digest,
                "descriptor": ("sa_tools b72d52f3 CHRMODELS.ini SONIC[100,101], KNUCKLES[40,41], BIG[4,29,30]"
                               if args.title else "sa_tools b72d52f3 CHRMODELS.ini ___SONIC_ACTIONS[100,101]"),
                "actions": actions if args.title else dict(actions), "records": importer.evidence}
    args.output.with_suffix(".provenance.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(json.dumps({"records": len(importer.records), "bytes": args.output.stat().st_size,
                      "sha256": digest, "actions": actions}))


if __name__ == "__main__":
    main()
