#!/usr/bin/env python3
"""Freeze the unchanged original-instruction oracle corpus as numeric reference data.

Development-only: requires the pinned executable and Unicorn. The independent
checker consumes only the emitted numbers/hashes, never this generator or EXE.
"""
import argparse
import hashlib
import json
import pathlib

import unicorn
from pe_image import PEImage
from verify_original_game_frame_control import SOURCE_SHA256, fixtures, source_run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('--output', type=pathlib.Path, default=pathlib.Path('reference/frame-control-corpus.tsv'))
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != '2.1.4':
        raise RuntimeError('pinned source/Unicorn mismatch')
    cases = fixtures()
    lines = ['# Frozen original instruction outputs: 19 input words, 16 expected output words.']
    for case in cases:
        expected = source_run(image, case)
        assert len(case) == 19 and len(expected) == 16
        words = case + expected
        assert all(0 <= word <= 0xffffffff for word in words)
        lines.append('\t'.join(str(word) for word in words))
    raw = ('\n'.join(lines) + '\n').encode()
    args.output.write_bytes(raw)
    root = pathlib.Path(__file__).resolve().parents[2]
    source_files = ['tools/oracles/verify_original_game_frame_control.py',
                    'tools/oracles/freeze_frame_control.py']
    digest = lambda data: hashlib.sha256(data).hexdigest()
    ranges = [(0x77ff50, 10), (0x411e20, 10), (0x411e30, 93), (0x7899a0, 54),
              (0x40f010, 86), (0x792f60, 3), (0x780069, 30)]
    metadata = dict(schema_version=1, row_count=len(cases), input_words=19, output_words=16,
                    source_sha256=image.sha256, unicorn_version=unicorn.__version__,
                    corpus_sha256=digest(raw),
                    generator_sha256={name: digest((root / name).read_bytes()) for name in source_files},
                    original_ranges_sha256={f'{at:x}-{at + size:x}': digest(image.read(at, size)) for at, size in ranges},
                    limits=['Frozen values, optional-fact masks, callback identity and declared availability boundaries only.',
                            'Unused input-callback EAX and FP exception flags are outside this original oracle scope.',
                            'No complete game, physical input or original startup-state independence claim.'])
    args.output.with_suffix('.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Frozen {len(cases)} original frame-control cases: {args.output}')


if __name__ == '__main__':
    main()
