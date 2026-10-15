#!/usr/bin/env python3
"""Build a provenance-preserving symbol index; no game binary is read.

This is a declaration reader, not a C++ compiler. Types retain their declarations;
unknown ABI sizes stay null. Every candidate survives selection in alternatives.
"""
import argparse
import collections
import csv
import hashlib
import json
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
LOADER = 'eb4cdcca9637d611a98b910fcfed6ce18580451c'
MACROS = {'FunctionPointer', 'VoidFunc', 'ThiscallFunctionPointer',
          'FastcallFunctionPointer', 'StdcallFunctionPointer', 'UsercallFunc',
          'UsercallFuncVoid', 'TaskFunc', 'ObjectFunc', 'CamFunc', 'CamAdjustFunc',
          'DataPointer', 'DataArray'}
WIDTHS = {'char': 1, 'signed char': 1, 'unsigned char': 1, 'bool': 1,
          'Sint8': 1, 'Uint8': 1, '__int8': 1, 'short': 2, 'unsigned short': 2,
          '__int16': 2, 'unsigned __int16': 2, 'Sint16': 2, 'Uint16': 2,
          'int': 4, 'unsigned int': 4, 'unsigned': 4, 'long': 4, 'unsigned long': 4,
          'Sint32': 4, 'Uint32': 4, 'float': 4, 'Float': 4, 'Angle': 4,
          'DWORD': 4, 'BOOL': 4, 'double': 8, '__int64': 8}


def clean(text):
    # Preserve offsets/line numbers, including strings containing comment delimiters.
    pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|^[ \t]*#[^\n]*(?:\\\n[^\n]*)*'
    return re.sub(pattern, lambda m: m[0] if m[0][0] in '\"\'' else
                  re.sub(r'[^\n]', ' ', m[0]), text, flags=re.M)


def split_args(text):
    parts, start, depth = [], 0, 0
    for i, char in enumerate(text):
        if char in '([{<':
            depth += 1
        elif char in ')]}>':
            depth -= 1
        elif char == ',' and depth == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    parts.append(text[start:].strip())
    return parts


def closing(text, start, left='(', right=')'):
    depth = 0
    for i in range(start, len(text)):
        if text[i] == left:
            depth += 1
        elif text[i] == right:
            depth -= 1
            if depth == 0:
                return i
    raise ValueError('unterminated declaration')


def normalize(text):
    return re.sub(r'\s+', ' ', text).strip()


def parse_header(text, source, priority):
    code = clean(text)
    entries, types = [], []
    for match in re.finditer(r'\b(' + '|'.join(sorted(MACROS)) + r')\s*\(', code):
        macro = match[1]
        end = closing(code, code.index('(', match.start()))
        args = split_args(code[code.index('(', match.start()) + 1:end])
        line = code.count('\n', 0, match.start()) + 1
        if macro in {'VoidFunc', 'TaskFunc', 'ObjectFunc', 'CamFunc', 'CamAdjustFunc'}:
            name, address = args
            result = 'void'
            signature = {'VoidFunc': '()', 'TaskFunc': '(task* tp)',
                         'ObjectFunc': '(ObjectMaster* obj)',
                         'CamFunc': '(_OBJ_CAMERAPARAM* pParam)',
                         'CamAdjustFunc': '(taskwk* twp, taskwk* ptwp, _OBJ_ADJUSTPARAM* adjwp)'}[macro]
        elif macro == 'UsercallFuncVoid':
            name, signature, _, address, *registers = args
            result = 'void'
        elif macro == 'UsercallFunc':
            result, name, signature, _, address, *registers = args
        elif macro == 'DataPointer':
            result, name, address = args
            signature = None
        elif macro == 'DataArray':
            result, name, address, signature = args
        else:
            result, name, signature, address = args
        try:
            value = int(address.rstrip('uUlL'), 0)
        except ValueError as error:
            raise ValueError(f'{source}:{line}: unresolved {macro} address {address}') from error
        kind = 'global' if macro.startswith('Data') else 'function'
        width = 4 if '*' in result else WIDTHS.get(normalize(result))
        size = None
        if kind == 'global':
            if macro == 'DataPointer':
                size = width
            elif width is not None and signature.isdecimal():
                size = width * int(signature)
        entries.append(dict(module='SONIC', address=f'0x{value:x}', kind=kind,
                            name=name, type=normalize(result), signature=normalize(signature) if signature else None,
                            size=size, source=source, line=line, macro=macro, priority=priority,
                            declaration=normalize(code[match.start():end + 1])))
    for match in re.finditer(r'\b(struct|enum(?:\s+class)?)\s+(\w+)[^;{}]*\{', code):
        start = code.index('{', match.start())
        end = closing(code, start, '{', '}')
        types.append(dict(kind=match[1], name=match[2], source=source,
                          line=code.count('\n', 0, match.start()) + 1,
                          declaration=normalize(code[match.start():end + 1]), size=None,
                          size_reason='ABI layout not inferred by declaration reader', priority=priority))
    return entries, types


def merge(candidates):
    groups = collections.defaultdict(list)
    for entry in candidates:
        groups[(entry['module'], int(entry['address'], 16))].append(entry)
    symbols, conflicts = [], []
    for key, entries in sorted(groups.items()):
        entries.sort(key=lambda e: (e['priority'], e['source'], e.get('line', 0), e['name']))
        selected = entries[0]
        record = {k: v for k, v in selected.items() if k != 'priority'}
        record['alternatives'] = entries
        record['alternative_names'] = sorted({e['name'] for e in entries} - {selected['name']})
        if len({(e['kind'], e['name'], e.get('type'), e.get('signature')) for e in entries}) > 1:
            conflicts.append(dict(module=key[0], address=f'0x{key[1]:x}',
                                  selected=selected, candidates=entries,
                                  reason='project override' if selected['priority'] == 0 else
                                  'preferred New headers, then classic, sa_tools, inventory'))
        symbols.append(record)
    return symbols, conflicts


def seeds(symbols, original):
    rows = {}
    for line in original.splitlines():
        if not line.strip() or line.startswith('#'):
            continue
        address, kind, *names = line.split('\t')
        rows[int(address, 0)] = (kind, names[0] if names else '')
    legacy = dict(rows)
    for entry in symbols:
        if entry['module'] != 'SONIC' or entry['source'].startswith('reference/inventory/'):
            continue
        address = int(entry['address'], 16)
        kind = 'function' if entry['kind'] == 'function' else 'label'
        # Keep callback definitions and original explicit names intact.
        previous = rows.get(address)
        if previous and previous[1]:
            continue
        name = re.sub(r'[^A-Za-z0-9_]', '_', entry['name'])
        rows[address] = (previous[0] if previous else kind, name)
    # Same symbol name can denote multiple entries. Use deterministic seed-only names.
    used = collections.Counter(name for _, name in rows.values() if name)
    for address, (kind, name) in list(rows.items()):
        if name and used[name] > 1 and not (address in legacy and legacy[address][1]):
            rows[address] = (kind, f'{name}_{address:x}')
    return '# Generated by tools/symbols/build_symbol_db.py; source names remain hypotheses.\n' + ''.join(
        f'0x{address:x}\t{kind}' + (f'\t{name}' if name else '') + '\n'
        for address, (kind, name) in sorted(rows.items()))


def git_commit(path):
    return subprocess.check_output(['git', '-C', str(path), 'rev-parse', 'HEAD'], text=True).strip()


def generate(root):
    loader = root / 'reference/sadx-mod-loader'
    expected = subprocess.check_output(['git', 'ls-tree', 'HEAD', 'reference/sadx-mod-loader'],
                                       cwd=root, text=True).split()[2]
    if expected != LOADER or git_commit(loader) != LOADER:
        raise ValueError('symbol-source mismatch: mod loader is not the pinned submodule')
    candidates, types, inputs = [], [], {}

    def read(path):
        raw = path.read_bytes()
        inputs[str(path.relative_to(root))] = hashlib.sha256(raw).hexdigest()
        return raw.decode('utf-8-sig')

    include = loader / 'SADXModLoader/include'
    paths = sorted({*include.glob('SADX*New.h'), *include.glob('SADXFunctions.h'),
                    *include.glob('SADXVariables.h'), *include.glob('SADXStructs.h'),
                    *include.glob('SADXEnums.h')})
    for path in paths:
        # A changed header with an unchanged git HEAD must not pass the pin check.
        relative = path.relative_to(loader).as_posix()
        committed = subprocess.check_output(['git', '-C', str(loader), 'show', f'{LOADER}:{relative}'])
        text = read(path)
        if path.read_bytes() != committed:
            raise ValueError(f'symbol-source mismatch: modified pinned header {relative}')
        found, definitions = parse_header(text, str(path.relative_to(root)), 10 if 'New.h' in path.name else 20)
        candidates.extend(found)
        types.extend(definitions)
    inventory_path = root / 'reference/inventory/functions.json'
    inventory = json.loads(read(inventory_path))
    for entry in inventory['functions']:
        if entry['is_external'] or not entry['address'].startswith('0x'):
            continue
        candidates.append(dict(module='SONIC', address=entry['address'], kind='function',
                               name=entry['name'], type=None, signature=None, size=entry.get('size'),
                               source='reference/inventory/functions.json', priority=40))
    # sa_tools label formats and pin are described in sources.json.
    sources = json.loads(read(root / 'reference/symbols/sources.json'))
    satools = root / sources['sa_tools']['path']
    if git_commit(satools) != sources['sa_tools']['commit']:
        raise ValueError('symbol-source mismatch: sa_tools is not at the pinned commit')
    for relative in sources['sa_tools']['labels']:
        path = satools / relative
        text = read(path)
        committed = subprocess.check_output(['git', '-C', str(satools), 'show',
                                             sources['sa_tools']['commit'] + ':' + relative])
        if path.read_bytes() != committed:
            raise ValueError(f'symbol-source mismatch: modified sa_tools labels {relative}')
        for number, line in enumerate(text.splitlines(), 1):
            if not line.strip() or line.startswith(';'):
                continue
            match = re.fullmatch(r'([0-9]+)=(.+)', line)
            if not match:
                raise ValueError(f'{path}:{number}: unsupported label syntax')
            # IniSerializer's Dictionary<int, string> keys are decimal absolute VAs.
            address = int(match[1], 10)
            candidates.append(dict(module='SONIC', address=f'0x{address:x}', kind='data', name=match[2],
                                   type=None, signature=None, size=None, priority=30,
                                   source=str(path.relative_to(root)), line=number))
    override_path = root / 'reference/symbols/overrides.tsv'
    overrides = csv.DictReader(read(override_path).splitlines(), delimiter='\t')
    seen = set()
    for row in overrides:
        key = (row['module'], int(row['address'], 0))
        if key in seen or not row['evidence_id'] or not row['reason']:
            raise ValueError('duplicate or unevidenced symbol override')
        seen.add(key)
        candidates.append(dict(module=key[0], address=f'0x{key[1]:x}', kind=row['kind'], name=row['name'],
                               type=row['type'] or None, signature=row['signature'] or None,
                               size=int(row['size'], 0) if row['size'] else None, priority=0,
                               evidence_id=row['evidence_id'], reason=row['reason'],
                               source='reference/symbols/overrides.tsv'))
    symbols, conflicts = merge(candidates)
    seed_text = seeds(symbols, read(root / 'reference/ghidra-game-loop-seeds.tsv'))
    indexed = {int(e['address'], 16): e for e in symbols if e['module'] == 'SONIC'}
    owners = set()
    for path in sorted((root / 'src').rglob('*.hpp')):
        text = read(path)
        owners.update(int(a, 16) for a in re.findall(r'\b\w+_at_([0-9a-f]{6,8})\b', text))
    named = lambda e: e and not re.match(r'^(?:FUN_|sub_|DAT_|LAB_|thunk_FUN_|Unwind@)', e['name'])
    procedures = [e for e in inventory['functions'] if not e['is_external']]
    named_procedures = sum(bool(named(indexed.get(int(e['address'], 16)))) for e in procedures)
    unnamed = [f'0x{address:x}' for address in sorted(owners) if not named(indexed.get(address))]
    coverage = dict(inventory_procedures=inventory['procedures'],
                    nonexternal_procedures=len(procedures), named_procedures=named_procedures,
                    legacy_owner_addresses=len(owners), named_owner_addresses=len(owners) - len(unnamed),
                    unnamed_owner_addresses=unnamed, conflicts=len(conflicts), types=len(types))
    def dump(value):
        return json.dumps(value, indent=2, sort_keys=True) + '\n'
    return {'symbols.json': dump(dict(schema_version=1, inputs=inputs, loader_commit=LOADER,
                                     sa_tools_commit=sources['sa_tools']['commit'],
                                     symbols=symbols, types=types)),
            'conflicts.json': dump(conflicts), 'coverage.json': dump(coverage), 'rea-seeds.tsv': seed_text}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='fail if any generated output is absent or stale')
    args = parser.parse_args()
    output = ROOT / 'build/symbols'
    outputs = generate(ROOT)
    output.mkdir(parents=True, exist_ok=True)
    for name, text in outputs.items():
        path = output / name
        if args.check:
            if not path.exists() or path.read_text() != text:
                raise SystemExit(f'stale symbol database: {path}; run make symbols')
        else:
            path.write_text(text)
    print(('checked' if args.check else 'generated') + ' symbol database: ' + str(output))


if __name__ == '__main__':
    main()
