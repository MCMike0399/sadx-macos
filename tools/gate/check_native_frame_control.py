#!/usr/bin/env python3
"""Build/run the frame-control pilot using only an isolated maintained-source closure.

The native journey and portable fact-adapter corpus use distinct isolated closures.
This proves independence for this named core and host journey, not the game or
production application. The reference game directories are never moved or hidden.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
COMMON_SOURCES = (
    "src/engine/game_frame_control.hpp", "src/engine/game_frame_control_internal.hpp",
    "src/engine/game_frame_control.cpp", "src/engine/speedrun.hpp", "src/engine/speedrun.cpp",
)
NATIVE_SOURCES = (*COMMON_SOURCES, "src/engine/game_frame_control_native.cpp",
                  "tests/unit/native_frame_control.cpp")
CORPUS_SOURCES = (*COMMON_SOURCES, "src/engine/frame_control_fwd.hpp",
                  "src/guest/frame_control_reference.hpp", "src/guest/frame_control_reference.cpp",
                  "tests/unit/reference_frame_control.cpp", "tests/probes/native_game_frame_control_probe.cpp")
NATIVE_UNITS = ("src/engine/game_frame_control.cpp", "src/engine/game_frame_control_native.cpp",
                "src/engine/speedrun.cpp", "tests/unit/native_frame_control.cpp")
CORPUS_UNITS = ("src/engine/game_frame_control.cpp", "src/guest/frame_control_reference.cpp",
                "src/engine/speedrun.cpp", "tests/unit/reference_frame_control.cpp",
                "tests/probes/native_game_frame_control_probe.cpp")
PORTABLE_HEADERS = ("guest/frame_control_reference.hpp",)
FIXTURES = ("reference/frame-control-corpus.tsv", "reference/frame-control-corpus.json")
FORBIDDEN = re.compile(
    r"ExecutableImage|HleProcess|unicorn|(?:^|[^a-zA-Z])uc_[a-zA-Z]|sadx::guest|sadx::hle|"
    r"(?-i:\b(?:Guest|Hle)[A-Za-z_0-9]+)|(?:guest|hle)/", re.I)
LIMITS = [
    "Frame-control core and scripted native host journey only; no full-game or production-app independence claim.",
    "Inputs and counter samples are host-owned scripted facts; physical devices and real-time scheduling are unverified.",
    "Frozen original numeric cases cover the four frame-control entry points; live original-binary differential verification remains separate. The host-driven limiter journey is scripted, not part of this original numeric corpus.",
    "An isolated source closure, compiler dependency lists, symbols and linked-library inspection are checked; this is not an operating-system access sandbox.",
]


def run(command: list[str], cwd: Path, env: dict[str, str]) -> str:
    result = subprocess.run(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            timeout=120, check=False)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {shlex.join(command)}\n"
                           f"{result.stdout}{result.stderr}")
    return result.stdout


def inspect_libraries(binary: Path, cwd: Path, env: dict[str, str], self_id: str | None = None) -> tuple[str, list[str]]:
    if platform.system() == "Darwin":
        output = run(["/usr/bin/otool", "-L", str(binary)], cwd, env)
        libraries = [line.strip().split(" (", 1)[0]
                     for line in output.splitlines()[1:] if line.strip()]
        if self_id is not None:
            if not libraries or libraries[0] != self_id:
                raise RuntimeError("shared library has an unexpected install identity")
            libraries = libraries[1:]
        allowed = ("/usr/lib/", "/System/Library/")
    elif platform.system() == "Linux":
        output = run(["/usr/bin/ldd", str(binary)], cwd, env)
        libraries = []
        for line in output.splitlines():
            if "linux-vdso" in line:
                continue
            if "not found" in line:
                raise RuntimeError(f"unresolved linked dependency: {line}")
            match = re.search(r"(?:=>\s*)?(/\S+)", line)
            if match:
                libraries.append(match.group(1))
            elif line.strip():
                raise RuntimeError(f"unrecognized linked dependency: {line}")
        allowed = ("/usr/lib/", "/lib/", "/lib64/")
    else:
        raise RuntimeError("linked-library inspection is implemented for macOS and Linux only")
    if not libraries:
        raise RuntimeError("linked-library inspection returned no libraries")
    if FORBIDDEN.search(output):
        raise RuntimeError("reference-runtime dependency present in linked libraries")
    for library in libraries:
        if not library.startswith(allowed):
            raise RuntimeError(f"non-system linked dependency: {library}")
    return output, libraries


CORPUS_WORKER = r'''
import ctypes,json,sys
from pathlib import Path
library=ctypes.CDLL(str(Path(sys.argv[1]).resolve()))
probe=library.sadx_native_game_frame_control
probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t]
probe.restype=ctypes.c_int
rows=0
mismatches=0
examples=[]
for line in Path(sys.argv[2]).read_text().splitlines():
 if not line.strip() or line.lstrip().startswith('#'): continue
 words=[int(word) for word in line.split('\t')]
 source=(ctypes.c_uint32*19)(*words[:19])
 actual=(ctypes.c_uint32*16)()
 accepted=probe(source,19,actual,16)
 if accepted!=1 or list(actual)!=words[19:]:
  mismatches+=1
  if len(examples)<10: examples.append({'row':rows,'accepted':accepted,'expected':words[19:],'actual':list(actual)})
 rows+=1
print(json.dumps({'passed':mismatches==0,'rows_compared':rows,'mismatches':mismatches,'examples':examples}))
'''


def validate_corpus(isolated: Path) -> tuple[dict, dict[str, str]]:
    metadata = json.loads((isolated / FIXTURES[1]).read_text())
    corpus = (isolated / FIXTURES[0]).read_bytes()
    digest = hashlib.sha256(corpus).hexdigest()
    if metadata.get("corpus_sha256") != digest:
        raise RuntimeError("frozen corpus SHA-256 does not match provenance metadata")
    count = 0
    for number, line in enumerate(corpus.decode().splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        words = line.split("\t")
        if len(words) != 35 or any(not re.fullmatch(r"[0-9]+", word) or int(word) > 0xffffffff for word in words):
            raise RuntimeError(f"corpus line {number} must contain 19 input and 16 output decimal uint32 words")
        count += 1
    if count == 0 or type(metadata.get("row_count")) is not int or metadata["row_count"] != count:
        raise RuntimeError("frozen corpus row count does not match provenance metadata")
    return metadata, {name: hashlib.sha256((isolated / name).read_bytes()).hexdigest() for name in FIXTURES}


def validate_closure(isolated: Path, sources: tuple[str, ...], portable_headers: tuple[str, ...] = ()) -> None:
    for name in sources:
        content = (isolated / name).read_text()
        inspected = content
        # Only the corpus closure may name this exact portable fact header. Its body is
        # still checked, as are every dependency, symbol and linked library.
        for header in portable_headers:
            inspected = re.sub(r'^[ \t]*#[ \t]*include[ \t]+"' + re.escape(header) + r'"[ \t]*$',
                               '', inspected, flags=re.M)
        if FORBIDDEN.search(inspected):
            raise RuntimeError(f"reference-runtime identifier in source closure: {name}")
        for delimiter, include in re.findall(r'^\s*#\s*include\s*([<"])([^>"\n]+)[>"]', content, re.M):
            if Path(include).is_absolute() or ".." in Path(include).parts:
                raise RuntimeError(f"include escapes isolated closure: {name}: {include}")
            if delimiter == '"':
                candidates = ((isolated / name).parent / include, isolated / "src" / include)
                resolved = [path.resolve() for path in candidates if path.is_file()]
                if not resolved or any(path.relative_to(isolated).as_posix() not in sources for path in resolved):
                    raise RuntimeError(f"undeclared maintained include: {name}: {include}")


def compile_closure(compiler: str, isolated: Path, sources: tuple[str, ...], units: tuple[str, ...],
                    portable_headers: tuple[str, ...] = ()) -> dict:
    for name in sources:
        source = ROOT / name
        if source.is_symlink():
            raise RuntimeError(f"declared maintained source is a symlink: {name}")
        destination = isolated / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
    validate_closure(isolated, sources, portable_headers)
    (isolated / "tmp").mkdir()
    env = {"PATH": "/usr/bin:/bin:/usr/sbin:/sbin", "HOME": str(isolated),
           "TMPDIR": str(isolated / "tmp"), "LC_ALL": "C"}
    toolchain_allowed: set[Path] = set()
    if platform.system() == "Darwin":
        sdk = Path(run(["/usr/bin/xcrun", "--show-sdk-path"], isolated, env).strip())
        toolchain_allowed.add((sdk / "SDKSettings.json").resolve())
    flags = ["-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
             "-ffp-contract=off", "-fno-fast-math", "-fPIC", "-Isrc"]
    commands, objects, dependencies, toolchain = [], {}, set(), {}
    for index, source in enumerate(units):
        obj, dep = f"unit-{index}.o", f"unit-{index}.d"
        command = [compiler, *flags, "-MMD", "-MF", dep, "-c", source, "-o", obj]
        commands.append(command)
        run(command, isolated, env)
        text = (isolated / dep).read_text().replace("\\\n", " ")
        for token in shlex.split(text.split(":", 1)[1]):
            path = (isolated / token).resolve()
            if path in toolchain_allowed:
                toolchain[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
                continue
            if not path.is_relative_to(isolated):
                raise RuntimeError(f"compiler dependency escaped isolated closure: {token}")
            relative = path.relative_to(isolated).as_posix()
            if relative not in sources:
                raise RuntimeError(f"undeclared compiler dependency: {relative}")
            dependencies.add(relative)
        objects[source] = obj
    if dependencies != set(sources):
        raise RuntimeError(f"declared source closure differs from compiler dependencies: {sorted(dependencies)}")
    return dict(env=env, flags=flags, commands=commands, objects=objects,
                dependencies=sorted(dependencies), toolchain=toolchain,
                files={name:hashlib.sha256((isolated/name).read_bytes()).hexdigest() for name in sources})


def check(cxx: str) -> dict:
    compiler = shutil.which(cxx)
    if not compiler:
        raise RuntimeError(f"compiler unavailable: {cxx}")
    with tempfile.TemporaryDirectory(prefix="sadx-native-frame-control-") as temporary:
        base = Path(temporary).resolve()
        native, corpus = base / "native", base / "corpus"
        n = compile_closure(compiler, native, NATIVE_SOURCES, NATIVE_UNITS)
        c = compile_closure(compiler, corpus, CORPUS_SOURCES, CORPUS_UNITS, PORTABLE_HEADERS)
        for name in FIXTURES:
            destination = corpus / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT/name, destination)
        metadata, fixture_hashes = validate_corpus(corpus)
        command = [compiler, *n['objects'].values(), '-o', 'native-frame-control-test']
        n['commands'].append(command);run(command, native, n['env'])
        binary = native / 'native-frame-control-test'
        symbols = run(['/usr/bin/nm', '-C', str(binary)], native, n['env'])
        if FORBIDDEN.search(symbols):
            raise RuntimeError('reference-runtime symbol present in isolated executable')
        library_output, libraries = inspect_libraries(binary, native, n['env'])
        journey = json.loads(run([str(binary)], native, n['env']))
        if journey.get('passed') is not True or journey.get('journeys') != [
                'native_host_frame_control', 'plain_state_values', 'native_clock_failures']:
            raise RuntimeError('native executable did not report all required journeys')
        reference_test = 'reference-frame-control-test'
        command = [compiler, *[obj for source,obj in c['objects'].items() if not source.startswith('tests/probes/')],
                   '-o', reference_test]
        c['commands'].append(command);run(command, corpus, c['env'])
        reference_journey = json.loads(run([str(corpus/reference_test)], corpus, c['env']))
        if reference_journey.get('passed') is not True or reference_journey.get('journeys') != [
                'native_host_frame_control', 'explicit_unknowns', 'native_clock_failures']:
            raise RuntimeError('portable adapter did not preserve the missing-fact journeys')
        shared_name = 'native-frame-control-probe.dylib' if platform.system() == 'Darwin' else 'native-frame-control-probe.so'
        self_id = f'@rpath/{shared_name}' if platform.system() == 'Darwin' else None
        shared_flags = ['-dynamiclib', '-Wl,-install_name,'+self_id] if self_id else ['-shared']
        command = [compiler, *shared_flags,
                   *[obj for source,obj in c['objects'].items() if not source.startswith('tests/unit/')],
                   '-o', shared_name]
        c['commands'].append(command);run(command, corpus, c['env'])
        shared = corpus/shared_name
        shared_symbols = run(['/usr/bin/nm','-C',str(shared)], corpus, c['env'])
        if FORBIDDEN.search(shared_symbols) or 'sadx_native_game_frame_control' not in shared_symbols:
            raise RuntimeError('invalid portable corpus probe symbols')
        shared_output, shared_libraries = inspect_libraries(shared, corpus, c['env'], self_id)
        corpus_result = json.loads(run([sys.executable,'-I','-c',CORPUS_WORKER,str(shared),str(corpus/FIXTURES[0])],
                                       corpus,c['env']))
        if corpus_result.get('passed') is not True or corpus_result.get('rows_compared') != metadata['row_count']:
            raise RuntimeError(f'frozen original numeric corpus mismatch: {json.dumps(corpus_result)}')
        return dict(passed=True, scope='native-frame-control-pilot', limits=LIMITS,
            source_sha256=n['files'], compiler=run([compiler,'--version'],native,n['env']).splitlines()[0],
            fixture_sha256=fixture_hashes, commands=n['commands'], compiler_dependencies=n['dependencies'],
            toolchain_metadata_sha256=n['toolchain'], linked_libraries=libraries,
            library_inspection=library_output.replace(str(base),'<isolated>'),
            symbol_table_sha256=hashlib.sha256(symbols.encode()).hexdigest(),
            reference_runtime_symbols_found=False, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
            journey=journey, portable_adapter_journey=reference_journey,
            frozen_original_corpus={**corpus_result,'corpus_sha256':metadata['corpus_sha256'],
                'shared_binary_sha256':hashlib.sha256(shared.read_bytes()).hexdigest(),
                'source_sha256':c['files'],'compiler_dependencies':c['dependencies'],'commands':c['commands'],
                'symbol_table_sha256':hashlib.sha256(shared_symbols.encode()).hexdigest(),
                'linked_libraries':shared_libraries,'library_inspection':shared_output.replace(str(base),'<isolated>')},
            isolation={'copied_files':list(NATIVE_SOURCES),'copied_fixtures':[],
                       'portable_adapter_files':list(CORPUS_SOURCES),'portable_adapter_fixtures':list(FIXTURES),
                       'separate_build_roots':True,'inherited_environment':False,
                       'original_binary_inputs':[],'core_archive_linked':False})


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--report", type=Path, default=ROOT / "build/native-frame-control.json")
    args = parser.parse_args()
    try:
        report = check(args.cxx)
    except (OSError, RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
        report = {"passed": False, "scope": "native-frame-control-pilot", "limits": LIMITS,
                  "error": str(error)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(f"Native frame-control isolated build/journey: {'PASS' if report['passed'] else 'FAIL'} ({args.report})")
    if not report["passed"]:
        print(report["error"])
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
