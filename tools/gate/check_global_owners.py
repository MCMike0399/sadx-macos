"""Fail when bytes of one source global have more than one native owner.

Scans owner structs in src/**/*.hpp for value members named *_at_<hex> (scalar,
optional or array forms). References (&) and pointers (*) are borrows, not
owners. Read-only .rdata constants (0x7db000..0x891b88) may be copied by value.
Member accesses inside initializers (owner.name_at_x) are not declarations;
call-argument snapshots are named *_from_<hex>. Scalars cover their type's
width, so a dword owner overlapping another struct's byte owner is reported;
arrays count only their first element (many are strided records);
SourceBytes<base,size> blocks count their whole extent.
"""
import pathlib,re,sys,collections
root=pathlib.Path(__file__).resolve().parents[2]
WIDTH={'uint8_t':1,'int8_t':1,'bool':1,'uint16_t':2,'int16_t':2,'uint32_t':4,'int32_t':4,'float':4,'uint64_t':8,'int64_t':8,'double':8}
def width(decl):
    m=re.search(r'SourceBytes<\s*0x([0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*>',decl)
    if m:return int(m.group(2),0)
    if 'std::array' in decl or 'std::vector' in decl:return 1
    m=re.search(r'std::optional<\s*(?:std::)?(\w+)\s*>',decl) or re.match(r'\s*(?:const\s+)?(?:std::)?(\w+)\s',decl)
    return WIDTH.get(m.group(1),1) if m else 1
owners=collections.defaultdict(set)
for path in sorted((root/'src').rglob('*.hpp')):
    struct=None
    for line in path.read_text().splitlines():
        code=line.split('//')[0]
        m=re.match(r'\s*(struct|class)\s+(\w+)',code)
        if m and '{' in code:struct=m.group(2)
        if struct is None:continue
        if '(' in code and ')' in code and not code.strip().startswith('std::'):
            if not re.search(r'std::(optional|array|vector|shared_ptr|function)',code):continue
        for decl in code.split(';'):
            if re.search(r'(&|\*)\s*\w+_at_',decl):continue
            w=width(decl)
            for name,address in re.findall(r'(?<![.\w])(\w+?_at_([0-9a-f]{6,8}))_?(?!\w)',decl):
                value=int(address,16)
                if 0x7db000<=value<0x891b88:continue
                for b in range(value,value+w):owners[b].add(f'{path.name}:{struct}.{name}')
        if code.strip().startswith('};'):struct=None
# Owner tables: every address literal between "// owner-table: Struct" and
# "// end owner-table" is a 4-byte global owned by that struct.
for path in sorted((root/'src').rglob('*.hpp')):
    text=path.read_text()
    for m in re.finditer(r'// owner-table: (\w+)(.*?)// end owner-table',text,re.S):
        for address in re.findall(r'0x([0-9a-f]{6,8})\b',m.group(2)):
            for b in range(int(address,16),int(address,16)+4):owners[b].add(f'{path.name}:{m.group(1)}.table_{address}')
duplicates={a:s for a,s in owners.items() if len({x.split('.')[0] for x in s})>1}
for a,s in sorted(duplicates.items()):print(f'{a:#x}: '+', '.join(sorted(s)))
print(f'{len(owners)} owned bytes, {len(duplicates)} with more than one owner struct')
sys.exit(1 if duplicates else 0)
