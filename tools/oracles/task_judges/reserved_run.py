"""One-shot Root task qualification. Prepare/review/commit before run --admitted.

No preparation command generates inputs, compiles code or executes original/native
behavior. The run marker is never removed, including after failure.
"""
import argparse
import hashlib
import importlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[3]
PACKET = ROOT / 'docs/refactor-inventory/packets/task-owner'
FREEZE = PACKET / 'RESERVED-FREEZE.json'
OUT = ROOT / 'build/task-final-reserved'
BEFORE = 'bd22ee423ea015f00152a97fa8fc2e1b4034b411'
CANDIDATE = 'f8ab5e408427bb799aa244e46274648862b0ec05'
SLUGS = ('task_work', 'task_lifecycle', 'task_display', 'task_delete')
ARCHIVE_PATHS = ('src', 'tests/probes', 'tests/fixtures', 'tools/oracles', 'tools/lib')


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def write_new(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def root_inputs():
    paths = []
    for directory in (ROOT / 'tools/oracles', ROOT / 'tools/lib'):
        paths.extend(directory.rglob('*.py'))
    paths.append(PACKET / 'ROOT-RESERVED-PROTOCOL.json')
    package = Path(sys.prefix) / 'lib' / f'python{sys.version_info.major}.{sys.version_info.minor}'
    package /= 'site-packages/unicorn'
    if not package.is_dir():
        raise ValueError('Use the pinned reference Python with Unicorn installed')
    paths.extend(p for p in package.rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    compiler = Path(subprocess.check_output(['xcrun', '--find', 'clang++'], text=True).strip())
    sdk = Path(subprocess.check_output(['xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip())
    paths.extend((compiler, sdk / 'SDKSettings.json', Path(sys.executable).resolve()))
    exe = ROOT / 'game/Sonic Adventure DX/sonic.exe'
    paths.append(exe)
    return {str(path.resolve()): digest(path) for path in sorted(set(paths))}


def prepare():
    if OUT.exists():
        raise ValueError('Existing execution directory; preserve and inspect it')
    if git('diff', '--name-only', 'HEAD', '--', 'src', 'tools/oracles', 'tools/lib').strip():
        raise ValueError('Commit the complete source/helper closure before preparation')
    if git('ls-tree', '-r', BEFORE, '--', 'src') == git('ls-tree', '-r', CANDIDATE, '--', 'src'):
        raise ValueError('Old and current Source must be distinct')
    if git('ls-files', '--others', '--exclude-standard', '--', 'tools/oracles', 'tools/lib').strip():
        raise ValueError('Uncommitted helper in the execution closure')
    if git('rev-parse', '--show-object-format').decode().strip() != 'sha1':
        raise ValueError('Unsupported Git object format')
    if git('ls-tree', '-r', CANDIDATE, '--', 'src') != git('ls-tree', '-r', '72d0e83', '--', 'src'):
        raise ValueError('Candidate differs from the reviewed Source')
    protocol = json.loads((PACKET / 'ROOT-RESERVED-PROTOCOL.json').read_text())
    if protocol['immutable_before_source'] != BEFORE:
        raise ValueError('Wrong immutable before-source model')
    write_new(FREEZE, dict(before=BEFORE, candidate=CANDIDATE,
                          protocol=protocol, inputs=root_inputs(),
                          source_trees={world: git('rev-parse', commit + ':src').decode().strip()
                                        for world, commit in (('old', BEFORE), ('current', CANDIDATE))},
                          archive_paths=ARCHIVE_PATHS,
                          policy='One admitted run, immutable source archives, no retries or overwritten output.'))


def verify_freeze():
    relative = str(FREEZE.relative_to(ROOT))
    if git('show', 'HEAD:' + relative) != FREEZE.read_bytes():
        raise ValueError('Execution freeze must be committed unchanged')
    frozen = json.loads(FREEZE.read_text())
    if frozen['before'] != BEFORE or frozen['candidate'] != CANDIDATE:
        raise ValueError('Frozen model identity differs')
    for name, expected in frozen['inputs'].items():
        if digest(name) != expected:
            raise ValueError('Frozen dependency changed: ' + name)
    if root_inputs() != frozen['inputs']:
        raise ValueError('Dependency set/interpreter/toolchain changed')
    return frozen


def command(argv, cwd, log):
    environment = dict(os.environ)
    environment.pop('PYTHONPATH', None)
    with log.open('x') as stream:
        result = subprocess.run(argv, cwd=cwd, env=environment, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f'Exit {result.returncode}: {argv}; log {log}')


def archive(world, commit):
    tree = OUT / world / 'tree'
    tree.mkdir(parents=True)
    data = git('archive', '--format=tar', commit, *ARCHIVE_PATHS)
    with tarfile.open(fileobj=io.BytesIO(data)) as source:
        source.extractall(tree, filter='data')
    verify_archive(tree, commit)
    return tree


def verify_archive(tree, commit):
    """Reject missing, changed or extra files before compilation and imports."""
    expected = set()
    for entry in git('ls-tree', '-r', '-z', commit, '--', *ARCHIVE_PATHS).split(b'\0'):
        if not entry:
            continue
        metadata, encoded = entry.split(b'\t', 1)
        mode, kind, object_id = metadata.decode().split()
        name = encoded.decode()
        if kind != 'blob' or mode not in ('100644', '100755'):
            raise ValueError('Unsupported archive member: ' + name)
        expected.add(name)
        path = tree / name
        if path.is_symlink():
            raise ValueError('Archive alias: ' + name)
        data = path.read_bytes()
        actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if actual != object_id:
            raise ValueError('Archived dependency changed: ' + name)
    actual = {str(path.relative_to(tree)) for path in tree.rglob('*') if path.is_file() or path.is_symlink()}
    if actual != expected:
        raise ValueError('Archived dependency set changed')


def build(world, tree):
    folder = OUT / world / 'libraries'
    folder.mkdir()
    for slug in SLUGS:
        argv = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
                '-mmacosx-version-min=14.0', '-Isrc', '-Itests/fixtures', '-dynamiclib',
                'tests/probes/native_' + slug + '_probe.cpp',
                'src/engine/task.cpp', 'src/engine/task_work.cpp',
                '-MMD', '-MF', str(folder / (slug + '.d')),
                '-Wl,-map,' + str(folder / (slug + '.map')),
                '-o', str(folder / (slug + '.dylib'))]
        command(argv, tree, folder / (slug + '-build.log'))


def child(mode, world=None, slug=None):
    frozen = verify_freeze()
    marker = json.loads((OUT / 'EXECUTED_ONCE.json').read_text())
    if marker['freeze_sha256'] != digest(FREEZE):
        raise ValueError('Execution marker does not bind this freeze')
    manifest = json.loads((OUT / 'BUILD.json').read_text())
    required = {str(OUT / name / 'libraries' / (item + '.dylib'))
                for name in ('old', 'current') for item in SLUGS}
    if set(manifest['libraries']) != required:
        raise ValueError('Compiled model set differs')
    for name, expected in manifest['libraries'].items():
        if digest(name) != expected:
            raise ValueError('Compiled model changed: ' + name)
    if mode == 'observe':
        inputs_manifest = json.loads((OUT / 'INPUTS.json').read_text())
        if set(inputs_manifest) != set(SLUGS):
            raise ValueError('Input cohort set differs')
        for item, expected in inputs_manifest.items():
            if digest(OUT / (item + '-inputs.json')) != expected:
                raise ValueError('Generated inputs changed: ' + item)
    name = 'generate' if mode == 'generate' else world + '-' + slug
    write_new(OUT / (name + '.started.json'), dict(mode=mode, world=world, slug=slug))
    tree = OUT / ('old' if mode == 'generate' else world) / 'tree'
    verify_archive(tree, frozen['before'] if mode == 'generate' or world == 'old' else frozen['candidate'])
    # All world-specific original scripts and helpers come from that immutable
    # archive. Root's recorder/observer come from the separately frozen Root tree.
    sys.path[:0] = [str(Path(__file__).parent), str(tree / 'tools/oracles'), str(tree / 'tools/lib')]
    observer = importlib.import_module('reserved_observer')
    if mode == 'generate':
        keys = dict(task_work='arena', task_lifecycle='lifecycle', task_display='display', task_delete='delete')
        for item in SLUGS:
            original = importlib.import_module('verify_original_' + item)
            definition = frozen['protocol']['cohorts'][keys[item]]
            cases, labels = observer.generate(item, definition, original)
            write_new(OUT / (item + '-inputs.json'), dict(definition=definition, cases=cases, labels=labels))
        write_new(OUT / 'INPUTS.json', {item: digest(OUT / (item + '-inputs.json')) for item in SLUGS})
        return
    inputs = json.loads((OUT / (slug + '-inputs.json')).read_text())
    library = OUT / world / 'libraries' / (slug + '.dylib')
    status = observer.observe(slug, inputs['cases'], inputs['labels'], inputs['definition'],
                              ROOT / 'game/Sonic Adventure DX/sonic.exe', library,
                              OUT / world / 'captures' / slug)
    if status:
        raise RuntimeError(f'{world}/{slug}: original comparison exit {status}')


def run(admitted):
    if not admitted:
        raise ValueError('Root must explicitly admit execution after independent review')
    frozen = verify_freeze()
    OUT.mkdir(exist_ok=False)
    write_new(OUT / 'EXECUTED_ONCE.json', dict(freeze_sha256=digest(FREEZE)))
    completed, failure = [], None
    try:
        for world, commit in (('old', frozen['before']), ('current', frozen['candidate'])):
            build(world, archive(world, commit))
        write_new(OUT / 'BUILD.json', dict(libraries={str(path): digest(path)
                  for world in ('old', 'current')
                  for path in sorted((OUT / world / 'libraries').glob('*.dylib'))}))
        script = str(Path(__file__).resolve())
        command([sys.executable, '-I', '-B', script, '_generate'], ROOT, OUT / 'generate.log')
        for slug in SLUGS:
            for world in ('old', 'current'):
                command([sys.executable, '-I', '-B', script, '_observe', '--world', world, '--slug', slug],
                        ROOT, OUT / (world + '-' + slug + '.log'))
            comparison = OUT / (slug + '-comparison.json')
            command([sys.executable, '-I', str(Path(__file__).with_name('compare_streams.py')),
                     str(OUT / 'old/captures' / slug), str(OUT / 'current/captures' / slug),
                     '--slug', slug, '--output', str(comparison)], ROOT, OUT / (slug + '-compare.log'))
            old = json.loads((OUT / 'old/captures' / slug / 'inputs.json').read_text())
            current = json.loads((OUT / 'current/captures' / slug / 'inputs.json').read_text())
            if old != current:
                raise ValueError('Old/current input bundle differs')
            completed.append(slug)
    except BaseException as error:
        failure = dict(type=type(error).__name__, message=str(error))
        raise
    finally:
        write_new(OUT / 'RUN.json', dict(completed=completed, error=failure,
                                        uncompleted=[slug for slug in SLUGS if slug not in completed]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['prepare', 'run', '_generate', '_observe'])
    parser.add_argument('--admitted', action='store_true')
    parser.add_argument('--world', choices=['old', 'current'])
    parser.add_argument('--slug', choices=SLUGS)
    args = parser.parse_args()
    if args.mode == 'prepare':
        prepare()
    elif args.mode == 'run':
        run(args.admitted)
    else:
        if args.mode == '_observe' and (args.world is None or args.slug is None):
            parser.error('Observation requires exact world and slug')
        child('generate' if args.mode == '_generate' else 'observe', args.world, args.slug)


if __name__ == '__main__':
    main()
