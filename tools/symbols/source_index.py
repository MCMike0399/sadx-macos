"""Read-only recovery context from maintained indexes and lexical source evidence.

Source matches are navigation aids, not proof of compiled or executed behavior.
No executable is opened and no Ghidra session is started.
"""

import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
NON_CODE = re.compile(
    r'R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"'
    r'|//(?:\\\n|[^\n])*|/\*.*?\*/'
    r'|"(?:\\.|[^"\\])*"|(?<!\w)(?:u8|u|U|L)?\'(?:\\.|[^\'\\])*\'', re.S)
ANNOTATION = re.compile(
    r'^[ \t]*//\s*(FUNCTION|LIBRARY|STUB):\s+(\w+)\s+(0x[\da-fA-F]+)\s+(\S+)(.*)$', re.M)
LITERAL_BINDING = re.compile(r'\b\w+\.push_back\s*\(\s*\{\s*(0x[\da-fA-F]+)[uUlL]*\s*,')
LOOP_BINDING = re.compile(
    r'for\s*\(\s*const\s+auto\s+(\w+)\s*:\s*\{(\s*0x[\da-fA-F]+[uUlL]*'
    r'(?:\s*,\s*0x[\da-fA-F]+[uUlL]*)*\s*,?\s*)\}\s*\)'
    r'\s*\{?\s*\w+\.push_back\s*\(\s*\{\s*\1\s*,')


def address(value):
    """CLI addresses are hexadecimal, including unprefixed addresses."""
    if value.startswith('EXTERNAL:'):
        return 'EXTERNAL:' + address(value.split(':', 1)[1])
    if not re.fullmatch(r'(?:0[xX])?[0-9a-fA-F]+', value):
        raise ValueError(f'invalid hexadecimal address: {value}')
    return hex(int(value, 16))


def read_json(path):
    try:
        return json.loads(path.read_text())
    except FileNotFoundError as error:
        raise ValueError(f'missing input {path}; run make symbols for the symbol database') from error


def mask(source):
    return NON_CODE.sub(lambda m: re.sub(r'[^\n]', ' ', m.group()), source)


def source_evidence(root):
    entries = {}
    unresolved = []

    def add(module, at, kind, path, source, offset, **extra):
        line = source.count('\n', 0, offset) + 1
        entries.setdefault((module, address(at)), []).append(dict(
            kind=kind, path=path.relative_to(root).as_posix(), line=line, **extra))

    for path in sorted((root / 'src').rglob('*')):
        if path.suffix not in {'.hpp', '.cpp', '.mm', '.h'}:
            continue
        source = path.read_text()
        # Preserve annotation comments while ignoring block-commented examples and strings.
        annotation_text = NON_CODE.sub(
            lambda m: m.group() if m.group().startswith('//') else re.sub(r'[^\n]', ' ', m.group()),
            source)
        for match in ANNOTATION.finditer(annotation_text):
            kind, module, at, symbol, reason = match.groups()
            add(module, at, kind.lower() + '_annotation', path, source, match.start(),
                symbol=symbol, reason=reason.strip() or None)
        code = mask(source)
        for match in re.finditer(r'\b(\w+_([0-9a-fA-F]{6,8}))\s*\(', code):
            # A call is only a reference, not evidence of a native declaration.
            prefix = code[code.rfind('\n', 0, match.start()) + 1:match.start()]
            if re.search(r'\b(?:void|bool|float|double|int|\w+(?:Result|State))\s+$', prefix):
                add('SONIC', match[2], 'legacy_declaration_candidate', path, source,
                    match.start(), identifier=match[1])
        if path.relative_to(root / 'src').parts[0] not in {'guest', 'hle'}:
            continue
        if not re.search(r'\bOverrideEntry\b', code):
            continue
        matched = []
        for pattern, kind in [(LITERAL_BINDING, 'literal_override'), (LOOP_BINDING, 'loop_override')]:
            for match in pattern.finditer(code):
                matched.append(match.span())
                addresses = [match[1]] if kind == 'literal_override' else re.findall(
                    r'0x[\da-fA-F]+', match[2])
                for at in addresses:
                    add('SONIC', at, kind, path, source, match.start())
        for match in re.finditer(r'\b\w+\.push_back\s*\(\s*\{', code):
            if not any(start <= match.start() < end for start, end in matched):
                unresolved.append(dict(path=path.relative_to(root).as_posix(),
                                       line=source.count('\n', 0, match.start()) + 1))
    return entries, unresolved


class ContextIndex:
    def __init__(self, root=ROOT, database=None, with_inventory=True):
        self.root = Path(root)
        self.database = read_json(database or self.root / 'build/symbols/symbols.json')
        self.symbols = {(s['module'], address(s['address'])): s for s in self.database['symbols']}
        functions = (read_json(self.root / 'reference/inventory/functions.json')['functions']
                     if with_inventory else [])
        self.functions = {address(f['address']): f for f in functions}
        self.evidence, self.unresolved = source_evidence(self.root)
        self.callers = {}
        for at, function in self.functions.items():
            for callee in function.get('callees', []):
                self.callers.setdefault(address(callee), []).append(at)

    def lookup(self, at, module='SONIC'):
        at = address(at)
        symbol = self.symbols.get((module, at))
        evidence = self.evidence.get((module, at), [])
        kinds = {e['kind'] for e in evidence}
        status = next((kind for kind in ('function_annotation', 'library_annotation',
                      'literal_override', 'loop_override', 'legacy_declaration_candidate',
                      'stub_annotation') if kind in kinds), 'not_established')
        return dict(module=module, address=at, symbol=symbol, source_evidence=evidence,
                    inventory_name=(self.functions.get(at, {}).get('name') if module == 'SONIC' else None),
                    native_status=status)

    def bundle(self, at, module='SONIC', cache_dir=None, end=None):
        result = self.lookup(at, module)
        at = result['address']
        function = self.functions.get(at) if module == 'SONIC' else None
        result['inventory'] = function
        result['callees'] = (None if function is None else
                             [self.lookup(c, module) for c in function.get('callees', [])])
        result['callers'] = (None if module != 'SONIC' else
                            [self.lookup(c, module) for c in sorted(self.callers.get(at, []))])
        cache = (cached_evidence(cache_dir or self.root / 'reference/inventory-raw', at)
                 if module == 'SONIC' else dict(matches=[], unreadable=[], directory_present=False))
        result['cached_evidence'] = cache
        start = int(at, 16) if not at.startswith('EXTERNAL:') else None
        body_ranges = [r for match in cache['matches'] for r in match['body_ranges']]
        if end is not None:
            if start is None or int(address(end), 16) <= start:
                raise ValueError('--end must be above the entry address')
            body_ranges = [dict(start=at, end=hex(int(address(end), 16) - 1))]
        windows = [(int(r['start'], 16), int(r['end'], 16) + 1) for r in body_ranges]
        if not windows and start is not None:
            windows = [(start, start + 1)]
        patches = read_json(self.root / 'tools/gate/profile_patches.json')
        ranges = []
        if module == 'SONIC' and start is not None:
            for patch in patches['patches']:
                pstart = int(patch['va'], 16)
                if any(lo < pstart + patch['size'] and pstart < hi for lo, hi in windows):
                    ranges.append(patch)
        scope = ('explicit_range' if end is not None else
                 'cached_body_ranges' if body_ranges else 'entry_byte_only')
        result['profile'] = dict(scope=scope if module == 'SONIC' else 'not_applicable',
                                 body_ranges=body_ranges, overlaps=ranges,
                                 limitation='Cached bodies may be stale or disagree; without a body, supply --end from REA evidence. Entry-only results do not establish full-range patch status.')
        result['missing_facts'] = []
        if module == 'SONIC' and not body_ranges:
            result['missing_facts'].append('Function body extents: request analyze_function through the owned REA runner before concluding full-range profile status.')
        if function is None:
            result['missing_facts'].append('No function record in the SONIC direct-call inventory for this module/address.')
        result['unresolved_override_initializers'] = self.unresolved
        result['limits'] = [
            'Static source matches are navigation evidence, not a compile, binding or fidelity verdict.',
            'Override extraction recognizes literal push_back entries and direct hex-list loops; other initializer candidates are reported, not interpreted.',
            'Direct-call inventory omits unresolved indirect calls; missing evidence is not proof of absence.',
            'Cached REA files may use older profiles; inspect evidence identity and limitations before use.',
        ]
        return result


def cached_evidence(directory, at):
    found = []
    errors = []
    directory = Path(directory)
    for path in sorted(directory.glob('*.json')):
        try:
            raw = path.read_bytes()
            body = json.loads(raw)
            if isinstance(body, dict) and 'content' in body:
                blocks = [json.loads(b['text']) for b in body['content'] if b.get('type') == 'text']
            else:
                blocks = [body]
            for block in blocks:
                if not isinstance(block, dict):
                    continue
                params = block.get('parameters', {})
                if not isinstance(params, dict):
                    raise ValueError('REA parameters must be an object')
                values = [params[k] for k in ('procedure', 'address') if k in params]
                values += params.get('addresses', [])
                normalized = []
                for value in values:
                    try:
                        normalized.append(address(value))
                    except (ValueError, AttributeError):
                        pass
                if at in normalized:
                    bodies = []
                    def collect_bodies(node):
                        if isinstance(node, dict):
                            procedure = node.get('procedure', {})
                            if isinstance(procedure, dict) and procedure.get('address'):
                                if address(procedure['address']) == at:
                                    bodies.extend(procedure.get('body', {}).get('ranges', []))
                            for value in node.values():
                                collect_bodies(value)
                        elif isinstance(node, list):
                            for value in node:
                                collect_bodies(value)
                    collect_bodies(block.get('normalized_result', {}))
                    found.append(dict(path=str(path), sha256=hashlib.sha256(raw).hexdigest(),
                                      evidence_id=block.get('evidence_id'), operation=block.get('operation'),
                                      subject=block.get('subject'), parameters=params, body_ranges=bodies))
        except (ValueError, OSError, TypeError, AttributeError) as error:
            errors.append(dict(path=str(path), error=str(error)))
    return dict(matches=found, unreadable=errors, directory_present=directory.is_dir())
