"""Development-only rendering of the pinned Ninja coefficient table as exact float literals.

Normal native builds use the checked-in C++ file and never invoke this emitter.
"""
import argparse
from pathlib import Path
import struct
from pe_image import PEImage
from extract import SOURCE_SHA256


def literal(word):
    value, = struct.unpack('<f', word)
    for precision in range(1, 10):
        text = format(value, f'.{precision}g')
        if struct.pack('<f', float(text)) == word:
            if '.' not in text and 'e' not in text:
                text += '.0'
            return text + 'F'
    raise ValueError('binary32 coefficient has no round-tripping decimal')


def render(image):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('pinned image identity mismatch')
    data = image.read(0x88af78, 4100)
    values = [literal(data[i:i + 4]) for i in range(0, len(data), 4)]
    lines = ['#include "math/ninja_sine_table.hpp"', '', 'namespace sadx {', 'namespace {',
             '// The library uses these rounded coefficients; analytic sine changes their bits.',
             '// DATA: SONIC 0x88af78 BAMSTable',
             'constexpr NinjaQuarterSineTable kQuarterSineTable{{']
    for start in range(0, len(values), 8):
        lines.append('    ' + ', '.join(values[start:start + 8]) + ',')
    lines += ['}};', '} // namespace', '', 'NinjaQuarterSineTable ninja_sine_table() {',
              '    return kQuarterSineTable;', '}', '} // namespace sadx', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--output', type=Path, default=Path('src/math/ninja_sine_table.cpp'))
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    source = render(PEImage(args.exe))
    if args.check:
        if args.output.read_text() != source:
            raise ValueError('maintained coefficients differ from pinned rendering')
        print('Ninja coefficient rendering: 1025 exact decimal literals match')
    else:
        args.output.write_text(source)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
