"""Read-only ISO9660 lister/extractor for MODE1/2352 raw BIN images (or plain 2048 ISOs).

usage: python3 -I iso_list.py list  IMAGE
       python3 -I iso_list.py get   IMAGE /PATH/IN/ISO OUTFILE
Prints "size\tpath" lines. Prefers the Joliet SVD when present (long names);
falls back to the primary volume descriptor (names with ;1 stripped).
"""
import hashlib
import struct
import sys

SECTOR_RAW = 2352
SECTOR_DATA = 2048


class Image:
    def __init__(self, path):
        self.f = open(path, 'rb')
        head = self.f.read(16)
        self.raw = head[:12] == b'\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00'
        self.sector = SECTOR_RAW if self.raw else SECTOR_DATA
        self.offset = 16 if self.raw else 0

    def read_sector(self, lba, count=1):
        out = bytearray()
        for i in range(count):
            self.f.seek((lba + i) * self.sector + self.offset)
            out += self.f.read(SECTOR_DATA)
        return bytes(out)

    def read_extent(self, lba, size):
        sectors = (size + SECTOR_DATA - 1) // SECTOR_DATA
        return self.read_sector(lba, sectors)[:size]


def find_descriptors(img):
    pvd = None
    svd = None
    lba = 16
    while True:
        s = img.read_sector(lba)
        if s[1:6] != b'CD001':
            break
        t = s[0]
        if t == 1 and pvd is None:
            pvd = s
        elif t == 2 and s[88:91] in (b'%/@', b'%/C', b'%/E'):
            svd = s
        elif t == 255:
            break
        lba += 1
    return pvd, svd


def parse_dir_records(data, joliet):
    pos = 0
    while pos < len(data):
        ln = data[pos]
        if ln == 0:
            # skip to next sector boundary
            pos = (pos // SECTOR_DATA + 1) * SECTOR_DATA
            continue
        rec = data[pos:pos + ln]
        extent = struct.unpack_from('<I', rec, 2)[0]
        size = struct.unpack_from('<I', rec, 10)[0]
        flags = rec[25]
        nlen = rec[32]
        name = rec[33:33 + nlen]
        if nlen == 1 and name in (b'\x00', b'\x01'):
            pos += ln
            continue
        if joliet:
            name = name.decode('utf-16-be', 'replace')
        else:
            name = name.decode('ascii', 'replace')
            if ';' in name:
                name = name.split(';', 1)[0]
        yield name, extent, size, bool(flags & 2)
        pos += ln


def walk(img, desc, joliet):
    root = desc[156:190]
    extent = struct.unpack_from('<I', root, 2)[0]
    size = struct.unpack_from('<I', root, 10)[0]
    entries = []

    def rec(lba, sz, prefix):
        data = img.read_extent(lba, sz)
        for name, ext, s, isdir in parse_dir_records(data, joliet):
            path = prefix + '/' + name
            if isdir:
                rec(ext, s, path)
            else:
                entries.append((path, ext, s))

    rec(extent, size, '')
    return entries


def main():
    cmd, image = sys.argv[1], sys.argv[2]
    img = Image(image)
    pvd, svd = find_descriptors(img)
    if pvd is None:
        sys.exit('no ISO9660 PVD found')
    desc, joliet = (svd, True) if svd is not None else (pvd, False)
    vol = pvd[40:72].decode('ascii', 'replace').strip()
    volsz = struct.unpack_from('<I', pvd, 80)[0]
    print(f'# raw={img.raw} joliet={svd is not None} volume_id={vol!r} volume_sectors={volsz}', file=sys.stderr)
    entries = walk(img, desc, joliet)
    if cmd == 'list':
        for path, ext, s in sorted(entries):
            print(f'{s}\t{path}')
        print(f'# files={len(entries)} bytes={sum(e[2] for e in entries)}', file=sys.stderr)
    elif cmd == 'get':
        want, out = sys.argv[3], sys.argv[4]
        limit = int(sys.argv[5]) if len(sys.argv) > 5 else None
        for path, ext, s in entries:
            if path.lower() == want.lower():
                data = img.read_extent(ext, s if limit is None else min(s, limit))
                with open(out, 'wb') as fh:
                    fh.write(data)
                print(f'{hashlib.sha256(data).hexdigest()}  {len(data)}  {path}')
                return
        sys.exit(f'not found: {want}')
    elif cmd == 'head':
        want, n = sys.argv[3], int(sys.argv[4])
        for path, ext, s in entries:
            if path.lower() == want.lower():
                data = img.read_extent(ext, min(s, n))
                print(f'{path} size={s} lba={ext}')
                print(data.hex(' '))
                print(data.decode('latin-1').replace('\n', '\\n'))
                return
        sys.exit(f'not found: {want}')


main()
