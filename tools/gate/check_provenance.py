#!/usr/bin/env python3
"""Check ADR-0006 annotations while legacy ownership remains checked separately.

GLOBAL annotations require an explicit source-byte size (native sizeof is not the
32-bit game's layout). Annotate one owning declaration per line. FUNCTION,
LIBRARY and STUB identify one declaration/implementation per source entry. STUB
permits optional reason text after the symbol and ordinary member functions;
callable value fields are not function declarations. DATA describes
imported content, not mutable storage. PATCH identifies a module-relative hook.
This is a deliberately restricted declaration grammar, not a C++ parser; ambiguous
annotated declarations fail instead of being assigned a guessed width/owner.
"""
import argparse
import collections
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[2]
KINDS = {'GLOBAL', 'FUNCTION', 'LIBRARY', 'STUB', 'DATA', 'PATCH'}
MODULES = {'SONIC', 'CHRMODELS', 'LOADER', 'INPUT', 'SPEEDRUN', 'SDL2'}
LEGACY = re.compile(r'\b\w+_at_[0-9a-f]{6,8}_?\b')
WIDTHS = {'uint8_t': 1, 'int8_t': 1, 'bool': 1, 'uint16_t': 2,
          'int16_t': 2, 'uint32_t': 4, 'int32_t': 4, 'float': 4,
          'uint64_t': 8, 'int64_t': 8, 'double': 8}


def source_files(root):
    return sorted(p for p in (root / 'src').rglob('*') if p.suffix in {'.hpp', '.cpp', '.mm'})


def legacy_identifiers(root):
    return {str(p.relative_to(root)): sorted(set(LEGACY.findall(p.read_text())))
            for p in source_files(root) if LEGACY.search(p.read_text())}


def legacy_snapshot(root):
    """Freeze source extents independently of the spelling allowances that shrink."""
    by_owner = collections.defaultdict(list)
    for (module, byte), names in sorted(legacy_owners(root).items()):
        for name in sorted(names):
            ranges = by_owner[(module, name)]
            if ranges and ranges[-1][0] + ranges[-1][1] == byte:
                ranges[-1][1] += 1
            else:
                ranges.append([byte, 1])
    return dict(identifiers=legacy_identifiers(root), owners=[
        dict(module=module, identity=name, ranges=[[hex(start), size] for start, size in ranges])
        for (module, name), ranges in sorted(by_owner.items())])


def without_block_comments(text):
    return re.sub(r'/\*[\s\S]*?\*/', lambda m: re.sub(r'[^\n]', ' ', m[0]), text)


def function_declaration(declaration):
    depth = 0
    for index, char in enumerate(declaration):
        depth += (char == '<') - (char == '>')
        if char == '(' and depth == 0:
            prefix = declaration[:index]
            return ('=' not in prefix and not declaration[index + 1:].lstrip().startswith(('*', '&'))
                    and bool(re.search(r'\b[A-Za-z_]\w*\s*$', prefix)))
    return False


def value_declaration(declaration):
    if not declaration.endswith(';'):
        return None
    depth = 0
    for index, char in enumerate(declaration):
        depth += (char in '<({[') - (char in '>)}]')
        if char == ',' and depth == 0:
            return None
        if char == ';' and depth == 0 and index != len(declaration) - 1:
            return None
    left = declaration.split('=')[0].rstrip('; ').strip()
    if re.search(r'\b(typedef|using|extern|auto)\b', left):
        return None
    if any(token in left for token in ('&', '*', '(')):
        return None
    variable = re.search(r'\b([A-Za-z_]\w*)\s*$', left)
    return variable[1] if variable else None


def legacy_width(decl):
    # Same conservative legacy extents as check_global_owners.py. Keep that checker
    # running; this copy exists to catch overlaps between migrated and legacy state.
    block = re.search(r'SourceBytes<\s*0x([0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*>', decl)
    if block:
        return int(block[2], 0)
    if 'std::array' in decl or 'std::vector' in decl:
        return 1
    scalar = re.search(r'std::optional<\s*(?:std::)?(\w+)\s*>', decl) or re.match(
        r'\s*(?:const\s+)?(?:std::)?(\w+)\s', decl)
    return WIDTHS.get(scalar[1], 1) if scalar else 1


def legacy_owners(root):
    owners = collections.defaultdict(set)
    for path in source_files(root):
        if path.suffix != '.hpp':
            continue
        text = without_block_comments(path.read_text())
        scope = None
        for line in text.splitlines():
            code = line.split('//')[0]
            match = re.match(r'\s*(struct|class)\s+(\w+)', code)
            if match and '{' in code:
                scope = match[2]
            if scope is None:
                continue
            if '(' in code and ')' in code and not code.strip().startswith('std::'):
                if not re.search(r'std::(optional|array|vector|shared_ptr|function)', code):
                    continue
            for decl in code.split(';'):
                if re.search(r'(&|\*)\s*\w+_at_', decl):
                    continue
                for name, address in re.findall(r'(?<![.\w])(\w+?_at_([0-9a-f]{6,8}))_?(?!\w)', decl):
                    address = int(address, 16)
                    if 0x7db000 <= address < 0x891b88:
                        continue
                    for byte in range(address, address + legacy_width(decl)):
                        owners[('SONIC', byte)].add(f'{path.relative_to(root)}:{scope}.{name}')
            if code.strip().startswith('};'):
                scope = None
        for match in re.finditer(r'// owner-table: (\w+)(.*?)// end owner-table', text, re.S):
            for address in re.findall(r'0x([0-9a-f]{6,8})\b', match[2]):
                for byte in range(int(address, 16), int(address, 16) + 4):
                    owners[('SONIC', byte)].add(f'{path.relative_to(root)}:{match[1]}.table_{address}')
    return owners


def check(root, database, baseline):
    errors = []
    current = legacy_identifiers(root)
    for path, names in baseline['identifiers'].items():
        for name in set(names) - set(current.get(path, [])):
            errors.append(f'{path}: stale legacy allowance {name}; remove it with the annotated migration')
    index = {(s['module'], int(s['address'], 16)): s for s in database['symbols']}
    legacy = legacy_owners(root)
    owners = collections.defaultdict(set, {key: set(value) for key, value in legacy.items()})
    entries, patches = {}, {}
    annotated_bytes = set()
    counts = collections.Counter()
    for path in source_files(root):
        relative = str(path.relative_to(root))
        lines = without_block_comments(path.read_text()).splitlines()
        for number, line in enumerate(lines, 1):
            for name in LEGACY.findall(line):
                if name not in baseline['identifiers'].get(relative, []):
                    errors.append(f'{relative}:{number}: new legacy identifier {name}')
            header = re.match(r'\s*//\s*([A-Z]+):\s*(.*)$', line)
            if not header or header[1] not in KINDS:
                continue
            location = f'{relative}:{number}'
            kind, body = header.groups()
            counts[kind] += 1
            if number == len(lines) or not lines[number].strip() or lines[number].lstrip().startswith('//'):
                errors.append(f'{location}: annotation must immediately precede a declaration')
                continue
            declaration = lines[number].split('//')[0].strip()
            if kind == 'PATCH':
                match = re.fullmatch(r'([A-Z0-9]+)\s+\+(0x[0-9a-fA-F]+)\s*(?:->|→)\s*(0x[0-9a-fA-F]+)', body)
                if not match or match[1] not in MODULES:
                    errors.append(f'{location}: malformed PATCH')
                    continue
                if not function_declaration(declaration):
                    errors.append(f'{location}: PATCH must precede a function declaration')
                key = (match[1], int(match[2], 16))
                if key in patches:
                    errors.append(f'{location}: duplicate PATCH also at {patches[key]}')
                patches[key] = location
                continue
            if kind == 'STUB':
                match = re.fullmatch(
                    r'([A-Z0-9]+)\s+(0x[0-9a-fA-F]+)\s+(\S+)(?:\s+\S.*)?', body.strip())
            else:
                match = re.fullmatch(r'([A-Z0-9]+)\s+(0x[0-9a-fA-F]+)\s+(\S+)(?:\s+size=(0x[0-9a-fA-F]+|[0-9]+))?', body)
            if not match:
                errors.append(f'{location}: malformed {kind}')
                continue
            if kind == 'STUB':
                module, address, name = match.groups()
                extent = None
            else:
                module, address, name, extent = match.groups()
            key = (module, int(address, 16))
            symbol = index.get(key)
            candidates = ([symbol] + symbol.get('alternatives', [])) if symbol else []
            named = [s for s in candidates if s['name'] == name]
            expected = {'global', 'data'} if kind in {'GLOBAL', 'DATA'} else {'function'}
            named = [s for s in named if s['kind'] in expected]
            if not named:
                errors.append(f'{location}: {kind} {module} {address} {name} absent from symbol database (or wrong kind)')
                continue
            if kind == 'GLOBAL':
                # Only one named value member, with initialization allowed. Commas
                # inside a type are fine; commas after the declarator are not.
                variable = value_declaration(declaration)
                if not extent or not variable:
                    errors.append(f'{location}: GLOBAL needs size= and one owning value declaration')
                    continue
                size = int(extent, 16 if extent.startswith('0x') else 10)
                known_sizes = {s['size'] for s in named if s.get('size') is not None}
                if size <= 0 or size > 1 << 24 or (known_sizes and size not in known_sizes):
                    errors.append(f'{location}: invalid source size {size}; known sizes {sorted(known_sizes)}')
                    continue
                if LEGACY.search(variable):
                    errors.append(f'{location}: migrate identifier and annotation together')
                    continue
                for byte in range(key[1], key[1] + size):
                    byte_key = (module, byte)
                    owners[byte_key].add(f'{location}:{variable}')
                    annotated_bytes.add(byte_key)
            elif kind in {'FUNCTION', 'LIBRARY', 'STUB'}:
                if not function_declaration(declaration):
                    errors.append(f'{location}: {kind} must precede a function declaration')
                if key in entries:
                    errors.append(f'{location}: duplicate source entry also at {entries[key]}')
                entries[key] = location
    for record in baseline['owners']:
        for start, size in record['ranges']:
            missing = [byte for byte in range(int(start, 16), int(start, 16) + size)
                       if not owners.get((record['module'], byte))]
            if missing:
                errors.append(f'{record["identity"]}: lost source ownership at {missing[0]:#x}; '
                              'a semantic rename requires GLOBAL coverage of the retained source extent')
    # Legacy-vs-legacy behavior stays under the old checker, including its struct
    # alias allowances. Any byte touched by an annotation has exactly one owner.
    for key in sorted(annotated_bytes):
        claims = set(owners[key])
        # Legacy identifiers do not encode a source module. Until a DLL owner is
        # migrated, conservatively reject a same-VA claim in any annotated module.
        # Do not silently assume it belongs to a different DLL at the same base.
        if key[0] != 'SONIC':
            claims.update(legacy.get(('SONIC', key[1]), set()))
        if len(claims) > 1:
            errors.append(f'{key[0]} {key[1]:#x}: overlapping GLOBAL owners: ' + ', '.join(sorted(claims)))
    return errors, dict(counts), len(owners)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=pathlib.Path, default=ROOT)
    parser.add_argument('--database', type=pathlib.Path, default=ROOT / 'build/symbols/symbols.json')
    parser.add_argument('--baseline', type=pathlib.Path, default=ROOT / 'reference/symbols/legacy-identifiers.json')
    args = parser.parse_args()
    errors, counts, owned = check(args.root, json.loads(args.database.read_text()), json.loads(args.baseline.read_text()))
    for error in errors:
        print(error)
    print(f'Provenance: {counts}, {owned} legacy/annotated byte identities, {len(errors)} errors')
    return int(bool(errors))


if __name__ == '__main__':
    raise SystemExit(main())
