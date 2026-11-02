"""Import Gamma's setup declaration without executing original code.

SDXGSP01 contains no process addresses. E102EFFECT is bound by the caller to
its existing twelve-entry native texture-list owner, never to a private copy.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage

SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
MAGIC = b'SDXGSP01'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def build(image):
    if image.sha256 != SOURCE_SHA:
        raise ValueError('unsupported Gamma setup source image')
    definition = image.read(0x9820f8, 80)
    textures, frames, count, source_blend, destination_blend = struct.unpack_from('<5I', definition)
    pointer = image.read(0x982148, 4)
    if (textures, frames, count, source_blend, destination_blend,
            struct.unpack('<I', pointer)[0]) != (0x91c570, 0x982058, 8, 8, 10, 0x9820f8):
        raise ValueError('Gamma setup declaration identity changed')
    texture_list = image.read(textures, 8)
    if struct.unpack('<II', texture_list) != (0x3c53ed8, 12):
        raise ValueError('Gamma setup E102EFFECT texture identity changed')
    animation = image.read(frames, count * 20)
    packed = MAGIC + struct.pack('<I', 12) + definition[8:] + animation
    # Reconstruct every byte of the source declaration and each pointer identity.
    inverse = struct.pack('<II', textures, frames) + packed[12:84]
    if inverse != definition or packed[84:] != animation:
        raise ValueError('Gamma setup inverse byte proof failed')
    proof = {'schema': 'SDXGSP01', 'source_sha256': image.sha256,
             'pack_sha256': digest(packed), 'pack_bytes': len(packed),
             'inverse_exact': True, 'texture_binding': 'E102EFFECT', 'texture_count': 12,
             'source_ranges': [{'address': hex(address), 'size': len(raw), 'sha256': digest(raw)}
                               for address, raw in [(0x9820f8, definition), (frames, animation),
                                                    (textures, texture_list), (0x982148, pointer)]],
             'limits': ['Texture memory is acquired by the existing character loader.',
                        'No image pixels or runtime texture handles are manufactured.',
                        'Particle update, spawn, display and shared registry cleanup are not ported.']}
    return packed, proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--proof', type=Path, required=True)
    args = parser.parse_args()
    source = args.exe.resolve()
    output = args.output.absolute()
    output = output.parent.resolve() / output.name
    proof_path = args.proof.absolute()
    proof_path = proof_path.parent.resolve() / proof_path.name
    if any(path.is_relative_to(source.parent) for path in (output, proof_path)):
        raise ValueError('outputs must be outside the original source directory')
    packed, proof = build(PEImage(source))
    output.parent.mkdir(parents=True, exist_ok=True)
    proof_path.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(packed)
    proof_path.write_text(json.dumps(proof, indent=2) + '\n')
    print(json.dumps(proof))


if __name__ == '__main__':
    main()
