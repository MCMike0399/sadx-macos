#!/usr/bin/env python3
"""Compile a private renderer with every draw suppressed; qualify only an image rejection."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('checker_arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / 'build'):
        raise ValueError('fault output must be a new directory under this worktree build/')
    output.mkdir(parents=True, exist_ok=False)
    # A private source/object/link only: maintained shipping code stays byte-identical.
    original = ROOT / 'src/platform/mac/metal_device_frame.mm'
    text = original.read_text()
    anchor = '        const auto &draw = std::get<graphics::Draw>(command);'
    if text.count(anchor) != 1:
        raise ValueError('draw-suppression anchor must occur exactly once')
    candidate = output / 'metal_device_frame.mm'
    candidate.write_text(text.replace(anchor, '        continue; // deliberate private draw suppression\n' + anchor))
    overlay = output / 'fault.mk'
    # Paths are supplied as Makefile variables; reject whitespace/metacharacters rather than shell-interpolate them.
    if any(c.isspace() or c in '$#:%\\' for c in str(output)):
        raise ValueError('fault build output requires a simple path')
    overlay.write_text(f'''{output}/renderer.o: {candidate}
\t$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) -c $< -o $@
{output}/sadx-native: src/apps/sadx_native.mm build/libsadx-native.a $(filter-out build/obj/platform/mac/metal_device_frame.o,$(NATIVE_PLATFORM_OBJECTS)) {output}/renderer.o
\t$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< $(filter-out build/obj/platform/mac/metal_device_frame.o,$(NATIVE_PLATFORM_OBJECTS)) {output}/renderer.o build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@
''')
    build = ['make', '-j2', '-o', 'build/native-controller-deps/include/SDL3/SDL.h',
             '-f', 'Makefile', '-f', str(overlay), str(output / 'sadx-native')]
    with (output / 'build.log').open('w') as log:
        compiled = subprocess.run(build, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    report = {'build_command': build, 'build_exit': compiled.returncode,
              'source_sha256': hashlib.sha256(original.read_bytes()).hexdigest(),
              'mutant_source_sha256': hashlib.sha256(candidate.read_bytes()).hexdigest(),
              'mutant': 'skip every graphics::Draw; preserve clear commands and semantic execution',
              'qualified_image_rejection': False}
    if compiled.returncode == 0:
        extra = args.checker_arguments
        if extra[:1] == ['--']:
            extra = extra[1:]
        if '--app' in extra or '--output' in extra:
            raise ValueError('fault owns --app and --output')
        command = [sys.executable, str(ROOT / 'tools/testing/check_native_character_select_display.py'),
                   '--app', str(output / 'sadx-native'), '--output', str(output / 'run')] + extra
        with (output / 'checker.log').open('w') as log:
            checked = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
        report.update(checker_command=command, checker_exit=checked.returncode)
        result_path = output / 'run/result.json'
        if result_path.exists():
            result = json.loads(result_path.read_text())
            report['checker_result'] = result
            # An unrelated startup/Metal failure is never counted as a caught draw mutant.
            report['qualified_image_rejection'] = (checked.returncode == 1 and
                result.get('semantic_state_passed') is True and
                result.get('screenshot_sha256') is not None and
                'full-frame blank' in result.get('drawn_goal_error', ''))
    (output / 'fault-result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 0 if report['qualified_image_rejection'] else 2


if __name__ == '__main__':
    raise SystemExit(main())
