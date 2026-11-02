"""Import both Gamma hit-effect frame tables, including all sentinel/padding bits."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'lib'))
from pe_image import PEImage

SOURCE_SHA = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'


def build(image):
    if image.sha256 != SOURCE_SHA:
        raise ValueError('unsupported Gamma effects image')
    textures = image.read(0x91d15c, 8)
    if struct.unpack('<II', textures) != (0x91d090, 17):
        raise ValueError('Gamma E102HIT declaration changed')
    hit = image.read(0x91d168, 17 * 44)
    burst = image.read(0x91d458, 5 * 44)
    for data, count in ((hit, 17), (burst, 5)):
        for index in range(count):
            texture = struct.unpack_from('<h', data, index * 44)[0]
            if (index == count - 1 and texture >= 0) or (
                    index < count - 1 and not 0 <= texture < 17):
                raise ValueError('Gamma frame index/sentinel changed')
    packed = b'SDXGEF01' + struct.pack('<III', 17, 17, 5) + hit + burst
    assert packed[20:768] == hit and packed[768:] == burst
    digest = lambda data: hashlib.sha256(data).hexdigest()
    proof = {'source_sha256': image.sha256, 'pack_sha256': digest(packed),
             'pack_bytes': len(packed), 'inverse_exact': True,
             'texture_binding': 'E102HIT', 'character': 6, 'list': 4,
             'source_ranges': {hex(address): {'size': len(raw), 'sha256': digest(raw)}
                               for address, raw in [(0x91d15c, textures),
                                                    (0x91d168, hit), (0x91d458, burst)]}}
    return packed, proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--proof', type=Path, required=True)
    args = parser.parse_args()
    source = args.exe.resolve()
    for path in (args.output, args.proof):
        if path.resolve().is_relative_to(source.parent):
            raise ValueError('output must be outside original input directory')
    packed, proof = build(PEImage(source))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(packed)
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + '\n')


if __name__ == '__main__':
    main()
