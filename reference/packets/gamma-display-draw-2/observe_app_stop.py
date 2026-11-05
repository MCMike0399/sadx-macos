"""Build a read-only failure-frame observer from the unchanged native app source."""
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser()
parser.add_argument('--output-directory', type=Path,
                    default=ROOT / 'build/draw-path-2/stop-observer-qualified')
OUT = parser.parse_args().output_directory.resolve()
OUT.mkdir(parents=True, exist_ok=False)
source = (ROOT / 'src/apps/sadx_native.mm').read_text()
needle = '    void report(std::ostream &output = std::cout) const {'
assert source.count(needle) == 1
observation = '''
        std::size_t object_draws = 0, normal_draws = 0, vertices = 0;
        for (const auto &command : display.output().frame().commands) {
            if (const auto *draw = std::get_if<sadx::graphics::Draw>(&command)) {
                if (draw->space == sadx::graphics::PositionSpace::Object) {
                    ++object_draws;
                    normal_draws += draw->has_normals;
                    vertices += draw->vertices.size();
                }
            }
        }
        std::cerr << "SADX_STOP_FRAME_OBSERVATION " << object_draws << ' '
                  << normal_draws << ' ' << vertices << '\\n';
'''
copy = OUT / 'sadx-native-observer.mm'
copy.write_text(source.replace(needle, needle + observation))
plan = subprocess.run(['make', '-n', '-j2', '-o',
                       'build/native-controller-deps/include/SDL3/SDL.h', '-W',
                       'src/apps/sadx_native.mm', 'build/sadx-native'], cwd=ROOT,
                      capture_output=True, text=True, check=True)
line = next(line for line in reversed(plan.stdout.splitlines())
            if line.startswith('xcrun clang++ ') and 'src/apps/sadx_native.mm' in line)
argv = shlex.split(line)
argv[argv.index('src/apps/sadx_native.mm')] = str(copy)
argv[-1] = str(OUT / 'sadx-native-observer')
# Observe the exact first-body call separately from other draws in the same frame.
binding = (ROOT / 'src/runtime/native_gamma_display_binding.cpp').read_text()
needle_call = "    animated_model::action_clip_no_scale(root, motion, frame, draw.animated(), context.draw_state(),\n                                         draw);"
assert binding.count(needle_call) == 1
binding_observation = """    const auto first_command = context.owner.rendering.output().frame().commands.size();
""" + needle_call + """
    std::size_t body_draws = 0, body_vertices = 0;
    const auto &commands = context.owner.rendering.output().frame().commands;
    for (auto index = first_command; index < commands.size(); ++index) {
        if (const auto *submitted = std::get_if<graphics::Draw>(&commands[index])) {
            ++body_draws;
            body_vertices += submitted->vertices.size();
        }
    }
    std::cerr << "SADX_BODY_OBSERVATION " << body_draws << ' ' << body_vertices << ' '
              << equipment(context) << '\\n';
"""
binding_copy = OUT / 'native-gamma-display-observer.cpp'
binding_copy.write_text('#include <iostream>\n' + binding.replace(needle_call, binding_observation))
argv.insert(argv.index(str(copy)) + 1, str(binding_copy))
build = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
(OUT / 'build.log').write_text(build.stdout + build.stderr)
assert build.returncode == 0, build.stderr
baseline = ROOT / 'build/draw-path-2/normal-input-qualified'
save = OUT / 'save'
shutil.copytree(baseline / 'save', save)
original = json.loads((baseline / 'result.json').read_text())
run = original['command'].copy()
run[0] = argv[-1]
run[run.index('--save-directory') + 1] = str(save)
run[run.index('--screenshot') + 1] = str(OUT / 'final.ppm')
completed = subprocess.run(run, cwd=ROOT, capture_output=True, text=True)
(OUT / 'app.log').write_text(completed.stdout + completed.stderr)
line = next(line for line in completed.stderr.splitlines()
            if line.startswith('SADX_STOP_FRAME_OBSERVATION '))
counts = [int(word) for word in line.split()[1:]]
body_line = next(line for line in completed.stderr.splitlines()
                 if line.startswith('SADX_BODY_OBSERVATION '))
body_counts = [int(word) for word in body_line.split()[1:]]
report = {'compile_command': argv, 'compile_exit': build.returncode,
          'command': run, 'exit': completed.returncode,
          'source_app_sha256': hashlib.sha256(source.encode()).hexdigest(),
          'observer_source_sha256': hashlib.sha256(copy.read_bytes()).hexdigest(),
          'inflight_object_draws': counts[0], 'inflight_normal_draws': counts[1],
          'inflight_vertices': counts[2], 'body_draws': body_counts[0],
          'body_vertices': body_counts[1], 'equipment': body_counts[2],
          'source_binding_sha256': hashlib.sha256(binding.encode()).hexdigest(),
          'observer_binding_sha256': hashlib.sha256(binding_copy.read_bytes()).hexdigest(),
          'screenshot_sha256': hashlib.sha256((OUT / 'final.ppm').read_bytes()).hexdigest(),
          'observation': 'read-only Frame.commands scans at report() and exact body submit boundary; presentation unchanged'}
(OUT / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
assert completed.returncode == 1 and body_counts[:2] == [98, 2862]
assert counts[0] == body_counts[0] + 1 and counts[2] == body_counts[1] + 840
assert report['screenshot_sha256'] == original['screenshot_sha256']
