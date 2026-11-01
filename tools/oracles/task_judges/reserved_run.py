"""One-shot Root task qualification. Prepare/review/commit before run --admitted.

No preparation command generates inputs, compiles code or executes original/native
behavior. The run marker is never removed, including after failure.
"""
import argparse
import ctypes
import hashlib
import importlib
import io
import json
import os
from pathlib import Path
import secrets
import stat
import subprocess
import sys
import tarfile
import threading
from types import ModuleType

ROOT = Path(__file__).resolve().parents[3]
PACKET = ROOT / 'docs/refactor-inventory/packets/task-owner'
FREEZE = PACKET / 'RESERVED-FREEZE.json'
OUT = ROOT / 'build/task-final-reserved'
BEFORE = 'bd22ee423ea015f00152a97fa8fc2e1b4034b411'
CANDIDATE = 'f8ab5e408427bb799aa244e46274648862b0ec05'
SLUGS = ('task_work', 'task_lifecycle', 'task_display', 'task_delete')
ARCHIVE_PATHS = ('src', 'tests/probes', 'tests/fixtures', 'tools/oracles', 'tools/lib')


def digest(path):
    path = Path(path)
    if path.is_symlink():
        return hashlib.sha256(('link:' + os.readlink(path)).encode()).hexdigest()
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def write_new(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def source_module(name, path):
    """Bind executed code to checked source, ignoring timestamp-valid .pyc files."""
    module = ModuleType(name)
    module.__file__ = str(path.resolve())
    sys.modules[name] = module
    try:
        exec(compile(path.read_bytes(), module.__file__, 'exec'), module.__dict__)
    except BaseException:
        sys.modules.pop(name, None)
        raise
    return module


def tree_inputs(directory, seen=None):
    seen = set() if seen is None else seen
    canonical = directory.resolve()
    if canonical in seen:
        return {}
    seen.add(canonical)
    result = {}
    for path in directory.rglob('*'):
        if path.is_symlink():
            result[str(path)] = hashlib.sha256(('link:' + os.readlink(path)).encode()).hexdigest()
            if path.resolve().is_file():
                result[str(path.resolve())] = digest(path.resolve())
            elif path.resolve().is_dir():
                result.update(tree_inputs(path.resolve(), seen))
        elif path.is_file():
            result[str(path)] = digest(path)
    return result


def toolchain():
    # Keep the clang++ argv spelling: resolving its symlink to "clang" changes
    # the driver's default link language even when the source suffix is .cpp.
    compiler = Path(subprocess.check_output(['xcrun', '--find', 'clang++'], text=True).strip())
    linker = Path(subprocess.check_output(['xcrun', '--find', 'ld'], text=True).strip()).resolve()
    sdk = Path(subprocess.check_output(['xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()).resolve()
    resource = Path(subprocess.check_output([str(compiler), '-print-resource-dir'], text=True).strip()).resolve()
    search = subprocess.run([str(compiler), '-E', '-v', '-x', 'c++', '-',
                             '-arch', 'arm64', '-mmacosx-version-min=14.0',
                             '-isysroot', str(sdk), '-resource-dir', str(resource),
                             '--no-default-config'], input='', capture_output=True, text=True,
                            env=dict(PATH='/usr/bin:/bin', LANG='C', LC_ALL='C'))
    if search.returncode:
        raise ValueError('Cannot establish compiler include closure: ' + search.stderr)
    includes, inside = [], False
    for line in search.stderr.splitlines():
        if 'search starts here:' in line:
            inside = True
        elif 'End of search list.' in line:
            inside = False
        elif inside:
            path = Path(line.strip().removesuffix(' (framework directory)'))
            if path.is_dir():
                includes.append(str(path.resolve()))
    cxx = [path for path in includes if Path(path).name == 'v1' and Path(path).parent.name == 'c++']
    if len(cxx) != 1:
        raise ValueError('Explicit compiler C++ include tree unavailable')
    lto = compiler.parent.parent / 'lib/libLTO.dylib'
    if not lto.is_file():
        raise ValueError('Driver linker support library unavailable')
    return dict(compiler=str(compiler), linker=str(linker), lto=str(lto), sdk=str(sdk),
                resource=str(resource), cxx=cxx[0], include_search=includes)


def root_inputs(tools=None):
    paths = []
    for directory in (ROOT / 'tools/oracles', ROOT / 'tools/lib'):
        paths.extend(directory.rglob('*.py'))
    paths.append(PACKET / 'ROOT-RESERVED-PROTOCOL.json')
    package = Path(sys.prefix) / 'lib' / f'python{sys.version_info.major}.{sys.version_info.minor}'
    package /= 'site-packages/unicorn'
    if not package.is_dir():
        raise ValueError('Use the pinned reference Python with Unicorn installed')
    # Third-party Python caches are executable model inputs too. Project helpers
    # load verified source directly; the pinned Unicorn package binds both forms.
    paths.extend(p for p in package.rglob('*') if p.is_file())
    tools = toolchain() if tools is None else tools
    paths.extend((Path(tools['compiler']), Path(tools['linker']), Path(tools['lto']),
                  Path(sys.executable).resolve()))
    exe = ROOT / 'game/Sonic Adventure DX/sonic.exe'
    paths.append(exe)
    result = {}
    for path in sorted(set(paths)):
        result[str(path.absolute())] = digest(path)
        if path.is_symlink():
            result[str(path.resolve())] = digest(path.resolve())
    for name in ('sdk', 'resource', 'cxx'):
        result.update(tree_inputs(Path(tools[name])))
    for directory in tools['include_search']:
        result.update(tree_inputs(Path(directory)))
    return result


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
    tools = toolchain()
    write_new(FREEZE, dict(before=BEFORE, candidate=CANDIDATE,
                          protocol=protocol, inputs=root_inputs(tools), toolchain=tools,
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
    selected = toolchain()
    if frozen['toolchain'] != selected:
        raise ValueError('Selected toolchain differs from frozen selection')
    for name, expected in frozen['inputs'].items():
        if digest(name) != expected:
            raise ValueError('Frozen dependency changed: ' + name)
    if root_inputs(selected) != frozen['inputs']:
        raise ValueError('Dependency set/interpreter/toolchain changed')
    return frozen


def command(argv, cwd, log, stage=None):
    # Compiler/header/library/plugin search must not inherit operator settings.
    environment = {name: os.environ[name] for name in ('HOME', 'TMPDIR', 'USER', 'LOGNAME')
                   if name in os.environ}
    environment.update(PATH='/usr/bin:/bin:/usr/sbin:/sbin', LANG='C', LC_ALL='C')
    read_fd = write_fd = None
    if stage is not None:
        read_fd, write_fd = os.pipe()
        pipe = os.fstat(read_fd)
        token = secrets.token_hex(24)
        active = OUT / 'ACTIVE.json'
        temporary = OUT / 'ACTIVE.new.json'
        temporary.write_text(json.dumps(dict(parent_pid=os.getpid(), stage=stage, token=token,
                                            pipe_device=pipe.st_dev, pipe_inode=pipe.st_ino)))
        temporary.replace(active)
        argv = [*argv, '--parent-fd', str(read_fd), '--stage-token', token]
    try:
        with log.open('x') as stream:
            result = subprocess.run(argv, cwd=cwd, env=environment, stdout=stream,
                                    stderr=subprocess.STDOUT,
                                    pass_fds=() if read_fd is None else (read_fd,))
    finally:
        if read_fd is not None:
            os.close(read_fd)
            os.close(write_fd)
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


def build(world, tree, tools):
    folder = OUT / world / 'libraries'
    folder.mkdir()
    for slug in SLUGS:
        argv = [tools['compiler'], '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64',
                '-mmacosx-version-min=14.0', '-isysroot', tools['sdk'],
                '-resource-dir', tools['resource'], '-isystem', tools['cxx'],
                '--ld-path=' + tools['linker'], '-fno-modules', '-fno-implicit-modules',
                '--no-default-config',
                '-Isrc', '-Itests/fixtures', '-dynamiclib',
                'tests/probes/native_' + slug + '_probe.cpp',
                'src/engine/task.cpp', 'src/engine/task_work.cpp',
                '-MMD', '-MF', str(folder / (slug + '.d')),
                '-Wl,-map,' + str(folder / (slug + '.map')),
                '-o', str(folder / (slug + '.dylib'))]
        command(argv, tree, folder / (slug + '-build.log'))


def admit_child(stage, parent_fd, stage_token, monitor=True):
    if (OUT / 'RUN.json').exists():
        raise ValueError('Execution is already terminal')
    if parent_fd is None or stage_token is None:
        raise ValueError('Internal entry requires the live admitted parent')
    active = json.loads((OUT / 'ACTIVE.json').read_text())
    pipe = os.fstat(parent_fd)
    if (os.getppid() != active['parent_pid'] or active['stage'] != stage or
            active['token'] != stage_token or not stat.S_ISFIFO(pipe.st_mode) or
            pipe.st_dev != active['pipe_device'] or pipe.st_ino != active['pipe_inode']):
        raise ValueError('Child parent/stage/pipe identity differs')
    if monitor:
        # A pipe writer exists only in the parent. EOF on abrupt parent death
        # kills the observer even inside a native call; it cannot consume later
        # cases as an orphan. This is execution safety, not progress polling.
        def parent_lifetime():
            try:
                os.read(parent_fd, 1)
            finally:
                os._exit(125)
        threading.Thread(target=parent_lifetime, daemon=True).start()


def verify_origins(tree, package):
    """Existing module cache entries must not override the sealed search roots."""
    originals = {path.stem for folder in ('tools/oracles', 'tools/lib')
                 for path in (tree / folder).glob('*.py')}
    roots = (tree / 'tools/oracles', tree / 'tools/lib')
    for name, module in list(sys.modules.items()):
        top = name.split('.')[0]
        if top not in originals and top not in ('unicorn', 'reserved_inputs', 'reserved_observer'):
            continue
        location = getattr(module, '__file__', None)
        if location is None:
            raise ValueError('Unsealed module origin: ' + name)
        path = Path(location).resolve()
        if top == 'unicorn':
            allowed = path.is_relative_to(package)
        elif top in ('reserved_inputs', 'reserved_observer'):
            allowed = path == Path(__file__).with_name(top + '.py').resolve()
        else:
            allowed = any(path.is_relative_to(root.resolve()) for root in roots)
        if not allowed:
            raise ValueError('Unsealed module origin: ' + name)


def pinned_unicorn(package):
    """Refuse loader fallback before any outside native library can be loaded."""
    expected = (package / 'lib/libunicorn.2.dylib').resolve()
    if not expected.is_file():
        raise ValueError('Pinned Unicorn native library missing')
    previous = ctypes.cdll.LoadLibrary
    def load(path):
        if Path(path).resolve() != expected:
            raise OSError('Unsealed Unicorn native loader path')
        return previous(str(expected))
    try:
        ctypes.cdll.LoadLibrary = load
        package_module = importlib.import_module('unicorn')
        binding = importlib.import_module('unicorn.unicorn_py3.unicorn')
        if Path(binding.uclib._name).resolve() != expected or package_module.__version__ != '2.1.4':
            raise ValueError('Loaded Unicorn model differs from pin')
    finally:
        ctypes.cdll.LoadLibrary = previous


def child(mode, world=None, slug=None, parent_fd=None, stage_token=None):
    name = 'generate' if mode == 'generate' else world + '-' + slug
    admit_child(name, parent_fd, stage_token)
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
    write_new(OUT / (name + '.started.json'), dict(mode=mode, world=world, slug=slug))
    tree = OUT / ('old' if mode == 'generate' else world) / 'tree'
    verify_archive(tree, frozen['before'] if mode == 'generate' or world == 'old' else frozen['candidate'])
    package = (Path(sys.prefix) / 'lib' / f'python{sys.version_info.major}.{sys.version_info.minor}' /
               'site-packages/unicorn').resolve()
    verify_origins(tree, package)
    # All world-specific original scripts and helpers come from that immutable
    # archive. Root's recorder/observer come from the separately frozen Root tree.
    sys.path[:0] = [str(Path(__file__).parent), str(tree / 'tools/oracles'),
                   str(tree / 'tools/lib'), str(package.parent)]
    pinned_unicorn(package)
    source_module('reserved_inputs', Path(__file__).with_name('reserved_inputs.py'))
    observer = source_module('reserved_observer', Path(__file__).with_name('reserved_observer.py'))
    verify_origins(tree, package)
    if mode == 'generate':
        keys = dict(task_work='arena', task_lifecycle='lifecycle', task_display='display', task_delete='delete')
        for item in SLUGS:
            original = importlib.import_module('verify_original_' + item)
            verify_origins(tree, package)
            definition = frozen['protocol']['cohorts'][keys[item]]
            cases, labels = observer.generate(item, definition, original)
            write_new(OUT / (item + '-inputs.json'), dict(definition=definition, cases=cases, labels=labels))
        write_new(OUT / 'INPUTS.json', {item: digest(OUT / (item + '-inputs.json')) for item in SLUGS})
        return
    inputs = json.loads((OUT / (slug + '-inputs.json')).read_text())
    library = OUT / world / 'libraries' / (slug + '.dylib')
    importlib.import_module('verify_original_' + slug)
    verify_origins(tree, package)
    status = observer.observe(slug, inputs['cases'], inputs['labels'], inputs['definition'],
                              ROOT / 'game/Sonic Adventure DX/sonic.exe', library,
                              OUT / world / 'captures' / slug)
    verify_origins(tree, package)
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
            build(world, archive(world, commit), frozen['toolchain'])
        write_new(OUT / 'BUILD.json', dict(libraries={str(path): digest(path)
                  for world in ('old', 'current')
                  for path in sorted((OUT / world / 'libraries').glob('*.dylib'))}))
        script = str(Path(__file__).resolve())
        command([sys.executable, '-I', '-S', '-B', script, '_generate'], ROOT, OUT / 'generate.log', stage='generate')
        for slug in SLUGS:
            for world in ('old', 'current'):
                command([sys.executable, '-I', '-S', '-B', script, '_observe', '--world', world, '--slug', slug],
                        ROOT, OUT / (world + '-' + slug + '.log'), stage=world + '-' + slug)
            comparison = OUT / (slug + '-comparison.json')
            command([sys.executable, '-I', '-S', '-B', str(Path(__file__).with_name('compare_streams.py')),
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
    if not (sys.flags.isolated and sys.flags.no_site and sys.flags.dont_write_bytecode):
        raise ValueError('Use pinned Python with -I -S -B; unsealed startup is refused')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['prepare', 'run', '_generate', '_observe'])
    parser.add_argument('--admitted', action='store_true')
    parser.add_argument('--world', choices=['old', 'current'])
    parser.add_argument('--slug', choices=SLUGS)
    parser.add_argument('--parent-fd', type=int)
    parser.add_argument('--stage-token')
    args = parser.parse_args()
    if args.mode == 'prepare':
        prepare()
    elif args.mode == 'run':
        run(args.admitted)
    else:
        if args.mode == '_observe' and (args.world is None or args.slug is None):
            parser.error('Observation requires exact world and slug')
        child('generate' if args.mode == '_generate' else 'observe', args.world, args.slug,
              args.parent_fd, args.stage_token)


if __name__ == '__main__':
    main()
