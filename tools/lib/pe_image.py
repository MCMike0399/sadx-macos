"""Read source-bound PE32 bytes. Guest addresses never become host pointers."""
import hashlib
import pathlib
import struct


class PEImage:
    def __init__(self, path):
        self.path = pathlib.Path(path).resolve()
        self.data = self.path.read_bytes()
        self.sha256 = hashlib.sha256(self.data).hexdigest()
        if self.data[:2] != b'MZ':
            raise ValueError('Not an MZ image')
        pe = self.unpack_offset('<I', 60)[0]
        if self.slice(pe, 4) != b'PE\0\0':
            raise ValueError('Not a PE image')
        machine, count, self.timestamp, _, _, optional_size, _ = self.unpack_offset('<HHIIIHH', pe + 4)
        if machine != 0x14c or optional_size < 224:
            raise ValueError('Requires an i386 PE32 image')
        optional = pe + 24
        if self.unpack_offset('<H', optional)[0] != 0x10b:
            raise ValueError('Requires PE32')
        self.base = self.unpack_offset('<I', optional + 28)[0]
        self.entry = self.base + self.unpack_offset('<I', optional + 16)[0]
        self.header_size = self.unpack_offset('<I', optional + 60)[0]
        self.import_directory = self.unpack_offset('<II', optional + 104)
        self.sections = []
        for i in range(count):
            entry = optional + optional_size + 40 * i
            raw = self.slice(entry, 40)
            size, rva, raw_size, offset = struct.unpack_from('<4I', raw, 8)
            self.slice(offset, raw_size)
            self.sections.append(dict(name=raw[:8].rstrip(b'\0').decode('ascii'),
                                      rva=rva, virtual_size=size, raw_size=raw_size,
                                      file_offset=offset))

    def slice(self, offset, size):
        if offset < 0 or size < 0 or offset > len(self.data) - size:
            raise ValueError(f'File range out of bounds: {offset:#x} + {size:#x}')
        return self.data[offset:offset + size]

    def unpack_offset(self, fmt, offset):
        return struct.unpack(fmt, self.slice(offset, struct.calcsize(fmt)))

    def file_offset(self, va, size=1):
        rva = va - self.base
        matches = [s for s in self.sections
                   if s['rva'] <= rva and rva + size <= s['rva'] + s['raw_size']]
        if len(matches) != 1:
            raise ValueError(f'Address is not uniquely file-backed: {va:#x} + {size:#x}')
        section = matches[0]
        return section['file_offset'] + rva - section['rva']

    def read(self, va, size):
        return self.slice(self.file_offset(va, size), size)

    def unpack(self, fmt, va):
        return struct.unpack(fmt, self.read(va, struct.calcsize(fmt)))

    def string(self, va):
        offset = self.file_offset(va)
        section = next(s for s in self.sections
                       if s['file_offset'] <= offset < s['file_offset'] + s['raw_size'])
        end = self.data.find(b'\0', offset, section['file_offset'] + section['raw_size'])
        if end < 0:
            raise ValueError('Unterminated PE string')
        return self.data[offset:end].decode('ascii')

    def imports(self):
        rva, size = self.import_directory
        if not rva:
            return []
        imports = []
        for delta in range(0, size - 19, 20):
            original, _, _, name, first = self.unpack('<5I', self.base + rva + delta)
            if not any((original, name, first)):
                return imports
            symbols = []
            thunk = self.base + (original or first)
            while True:
                value, = self.unpack('<I', thunk)
                if not value:
                    break
                symbols.append({'ordinal': value & 0xffff} if value & 0x80000000
                               else {'name': self.string(self.base + value + 2)})
                thunk += 4
            imports.append({'dll': self.string(self.base + name), 'symbols': symbols})
        raise ValueError('Import table has no terminator in declared directory')

    def describe(self):
        return dict(path=str(self.path), sha256=self.sha256, size=len(self.data),
                    image_base=hex(self.base), entry_point=hex(self.entry),
                    coff_timestamp=self.timestamp, sections=self.sections, imports=self.imports())
