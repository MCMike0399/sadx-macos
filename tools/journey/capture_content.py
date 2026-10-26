"""Fail-closed capture checks; expectations require recorded reference provenance."""
import hashlib
import json
from pathlib import Path

FIXTURE = Path(__file__).resolve().parents[2] / 'reference/frozen-corpora/capture-content.json'


def read_capture(path):
    data = Path(path).read_bytes()
    header = b'P6\n640 480\n255\n'
    if not data.startswith(header) or len(data) != len(header) + 640 * 480 * 3:
        raise ValueError(f'{path}: missing or malformed 640x480 PPM capture')
    pixels = data[len(header):]
    if pixels == pixels[:3] * (640 * 480):
        raise ValueError(f'{path}: full-frame blank (uniform RGB)')
    return pixels


def coarse_signature(pixels):
    cells = []
    for top in range(0, 480, 20):
        for left in range(0, 640, 20):
            sums = [0, 0, 0]
            for y in range(top, top + 20):
                for x in range(left, left + 20):
                    offset = 3 * (y * 640 + x)
                    for channel in range(3):
                        sums[channel] += pixels[offset + channel]
            cells.append([round(value / 400) for value in sums])
    return cells


def check_content(path, checkpoint):
    # Validate the whole payload even when the reference protects only a region.
    pixels = read_capture(path)
    fixture = json.loads(FIXTURE.read_text())
    expectation = fixture['checkpoints'].get(checkpoint)
    if expectation is None:
        raise ValueError(f'{checkpoint}: missing qualified reference content expectation; '
                         'see capture-content.json blockers (no native-only blessing)')
    reference = fixture['references'][expectation]
    if not reference.get('provenance'):
        raise ValueError(f'{checkpoint}: content expectation has no reference provenance')
    if 'signature' in reference:
        expected = reference['signature']
        if (len(expected) != 768 or any(len(cell) != 3 or
                any(type(value) is not int or not 0 <= value <= 255 for value in cell)
                for cell in expected)):
            raise ValueError('reference signature must have 768 RGB cells')
        actual = coarse_signature(pixels)
        distance = sum(abs(a - b) for ca, cb in zip(actual, expected)
                       for a, b in zip(ca, cb)) / (768 * 3)
        # Retain run_journeys.py's existing mean-channel distance ceiling.
        if distance > 12:
            raise ValueError(f'{checkpoint}: reference content signature distance {distance:.1f} > 12')
    elif reference.get('regions'):
        for region in reference['regions']:
            left, top, right, bottom = region['rectangle']
            if not (0 <= left < right <= 640 and 0 <= top < bottom <= 480):
                raise ValueError(f'{checkpoint}: invalid reference rectangle')
            rgb = b''.join(pixels[3 * (y * 640 + left):3 * (y * 640 + right)]
                           for y in range(top, bottom))
            if hashlib.sha256(rgb).hexdigest() != region['rgb_sha256']:
                raise ValueError(f'{checkpoint}: reference content region {region["name"]} differs')
    else:
        raise ValueError(f'{checkpoint}: empty reference content expectation')


def check_captures(captures, frames, journey):
    for frame in frames:
        try:
            check_content(captures / f'{frame}.ppm', f'{journey}/{frame}')
        except ValueError as error:
            raise RuntimeError(str(error)) from error
