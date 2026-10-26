"""Fold pinned, linear Gamma content declarations without executing game code.

This is a deliberately bounded declaration reader, not an x86 interpreter: only
MOV, self-XOR and balanced register preservation are accepted. Register known-bit
masks survive narrow writes; indeterminate values cannot become asset bytes.
"""
import struct

import capstone
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM, X86_OP_REG
from sonic_actor_resources import REG as SONIC_REG, bss, digest

REG = {**SONIC_REG, **{short: (root, 16, 0) for root, short in
                       [('esi', 'si'), ('edi', 'di'), ('ebp', 'bp'), ('esp', 'sp')]}}

ANIMATION = (0x7ca300, 0x7cb560, 0x3c53fa8, 91 * 16,
             '4ff64cf262910383a7a78e195fdec89ac1fa1910375b17c26c9da3b95a529622')
WELD = (0x7cb560, 0x7cb66b, 0x3c54558, 5 * 24,
        'ed7c9e93f9ad8429caaab524a555121b033644b81a71d9e05c8e35f05a38ecd0')


def fold(exe, spec, slots, tables):
    start, end, destination, size, expected = spec
    code = exe.read(start, end - start)
    if digest(code) != expected:
        raise ValueError(f'Gamma declaration initializer bytes changed at {start:#x}')
    origin = bss(exe, destination, size)
    result = bytearray(size)
    registers = {root: (0, 0) for root, _, _ in REG.values()}
    saved = []
    reads, writes = [], []
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(code, start))
    if not instructions or sum(i.size for i in instructions) != len(code):
        raise ValueError('incomplete Gamma declaration body')

    def read_register(register):
        root, width, shift = REG[decoder.reg_name(register)]
        value, known = registers[root]
        mask = (1 << width) - 1
        if (known >> shift) & mask != mask:
            raise ValueError('Gamma declaration reads an indeterminate register')
        return (value >> shift) & mask

    def write_register(register, value):
        root, width, shift = REG[decoder.reg_name(register)]
        mask = ((1 << width) - 1) << shift
        previous, known = registers[root]
        registers[root] = ((previous & ~mask) | ((value << shift) & mask), known | mask)

    def value_of(operand):
        if operand.type == X86_OP_REG:
            return read_register(operand.reg)
        if operand.type == X86_OP_IMM:
            return operand.imm & ((1 << (8 * operand.size)) - 1)
        if operand.type != X86_OP_MEM or operand.mem.index or operand.mem.segment:
            raise ValueError('unsupported Gamma declaration source')
        address = operand.mem.disp
        if operand.mem.base:
            address += read_register(operand.mem.base)
        if address in slots and operand.size == 4:
            value = slots[address]
        else:
            matches = [data[address - base:address - base + operand.size]
                       for base, data in tables.items()
                       if base <= address and address + operand.size <= base + len(data)]
            if len(matches) != 1:
                raise ValueError(f'Gamma declaration reads outside declared tables at {address:#x}')
            value = int.from_bytes(matches[0], 'little')
        reads.append({'address': hex(address), 'size': operand.size, 'value': hex(value)})
        return value

    for ordinal, instruction in enumerate(instructions):
        ops = instruction.operands
        if instruction.mnemonic in ('push', 'pop'):
            if len(ops) != 1 or ops[0].type != X86_OP_REG or ops[0].size != 4:
                raise ValueError('unsupported Gamma declaration preservation')
            root, _, _ = REG[decoder.reg_name(ops[0].reg)]
            if instruction.mnemonic == 'push':
                saved.append((root, registers[root]))
            else:
                if not saved or saved[-1][0] != root:
                    raise ValueError('unbalanced Gamma declaration preservation')
                registers[root] = saved.pop()[1]
            continue
        if instruction.mnemonic == 'ret':
            if saved or ordinal != len(instructions) - 1 or ops:
                raise ValueError('incomplete Gamma declaration return')
            continue
        if (instruction.mnemonic == 'xor' and len(ops) == 2 and
                ops[0].type == X86_OP_REG and ops[1].type == X86_OP_REG and
                ops[0].reg == ops[1].reg):
            write_register(ops[0].reg, 0)
            continue
        if instruction.mnemonic != 'mov' or len(ops) != 2:
            raise ValueError('Gamma initializer contains behavior beyond static declarations')
        value = value_of(ops[1])
        target = ops[0]
        if target.type == X86_OP_REG:
            write_register(target.reg, value)
            continue
        if (target.type != X86_OP_MEM or target.mem.base or target.mem.index or
                target.mem.segment):
            raise ValueError('unsupported Gamma declaration destination')
        offset = target.mem.disp - destination
        if offset < 0 or offset + target.size > size:
            raise ValueError('Gamma initializer writes outside declared owner')
        raw = (value & ((1 << (8 * target.size)) - 1)).to_bytes(target.size, 'little')
        result[offset:offset + target.size] = raw
        writes.append({'instruction': hex(instruction.address),
                       'destination': hex(target.mem.disp), 'bytes': raw.hex()})
    if instructions[-1].mnemonic != 'ret':
        raise ValueError('Gamma declaration has no final return')
    return bytes(result), {'address': hex(start), 'size': len(code), 'sha256': expected,
                           'initial_zero_fill': origin, 'reads': reads, 'writes': writes,
                           'origin': 'static declaration folding; not original BSS payload bytes'}


def recipes(exe, action_table, declarations):
    table = struct.pack('<78I', *declarations)
    raw, proof = fold(exe, ANIMATION, {0x3ab916c: action_table}, {action_table: table})
    # Sentinel folding identifies declaration indices despite source pointer aliases.
    sentinels = [0x50000000 + 16 * i for i in range(78)]
    sentinel_raw, _ = fold(exe, ANIMATION, {0x3ab916c: action_table},
                         {action_table: struct.pack('<78I', *sentinels)})
    rows = []
    for index in range(91):
        action, count, mode, next_action, transition, speed = struct.unpack_from('<IBbHII', raw, index * 16)
        marker = struct.unpack_from('<I', sentinel_raw, index * 16)[0]
        if marker == 0x3c5d530:
            declaration = 'constructed'
        elif 0x50000000 <= marker < 0x50000000 + 78 * 16 and marker % 16 == 0:
            declaration = (marker - 0x50000000) // 16
        else:
            raise ValueError(f'Gamma animation row {index} has unknown declaration {marker:#x}')
        rows.append({'row': index, 'declaration': declaration, 'object_count': count,
                     'mode': mode, 'next': next_action,
                     'transition_bits': hex(transition), 'speed_bits': hex(speed)})
    proof['sentinel_recipe_bytes_sha256'] = digest(sentinel_raw)
    proof['reconstructed_bytes_sha256'] = digest(raw)
    return rows, raw, proof
