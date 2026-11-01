"""Observe the stopped Sonic original without mapping missing memory or changing inputs.

Original memory faults, defined-prefix equality, native tagged boundaries and
old/current native equality are independent result dimensions. An observed fault
is never a successful original return or a full gameplay pass.
"""
import struct


def established_null_player_fault(fixture, fault):
    """Recognize the diagnosed reached instruction, not every null-player input."""
    return (fixture[0] == 2 and fixture[35] == 0 and fixture[30] != 0 and
            fault['kind'] == 'read_unmapped' and fault['pc'] == 0x49aa50 and
            fault['address'] == 6 and fault['size'] == 2 and fault['ebp'] == 0)


def defined_prefix(machine, fixture, locations, boundary=0):
    """The inherited Sonic field projection, read only from mapped original state."""
    def get(address, fmt):
        return struct.unpack(fmt, machine.mem_read(address, struct.calcsize(fmt)))

    task, player, world, info = (getattr(locations, name) for name in ('TASK', 'PLAYER', 'WORLD', 'INFO'))
    values = {2: get(task + 4, '<H')[0], 3: get(task, '<B')[0],
              4: get(task + 6, '<H')[0], 5: get(player + 6, '<H')[0],
              6: get(player + 0x7e, '<H')[0], 16: get(player + 0x1c, '<I')[0],
              23: get(info + 3, '<B')[0], 24: get(info + 0x34, '<I')[0]}
    for start, address in ((7, player + 0x38), (10, task + 0x14), (13, world + 0x1c)):
        for index, value in enumerate(get(address, '<3I')):
            values[start + index] = value
    if fixture[0] == 2:
        values[19] = int(get(0x3abdf60, '<I')[0] == task)
        values[20] = int(get(0x3abdf64, '<I')[0] == (player if fixture[35] else 0))
        published_mode = get(0x3c55d45, '<B')[0]
        values[21] = int(published_mode != 0xcc)
        values[22] = published_mode if published_mode != 0xcc else 0
        values[25] = boundary
    return values


class SonicFaultObserver:
    def __init__(self, original, locations):
        import unicorn
        from unicorn.x86_const import UC_X86_REG_EBP, UC_X86_REG_EIP
        self.original, self.locations = original, locations
        self.faults, self.writes = [], []
        self.unicorn = unicorn
        self.pc_register, self.base_register = UC_X86_REG_EIP, UC_X86_REG_EBP
        machine = original.machine

        def invalid(m, access, address, size, value, user):
            self.faults.append(dict(kind='read_unmapped' if access == unicorn.UC_MEM_READ_UNMAPPED
                                    else 'unexpected_memory_fault', access=access,
                                    pc=m.reg_read(self.pc_register), address=address, size=size,
                                    value=value, ebp=m.reg_read(self.base_register)))
            return False

        observed = [(locations.TASK, 64), (locations.PLAYER, 512), (locations.WORLD, 64),
                    (locations.INFO, 64), (0x3abdf60, 8), (0x3c55d45, 1)]

        def write(m, access, address, size, value, user):
            if any(begin <= address and address + size <= begin + length for begin, length in observed):
                self.writes.append(dict(pc=m.reg_read(self.pc_register), address=address,
                                        size=size, value=value))

        machine.hook_add(unicorn.UC_HOOK_MEM_INVALID, invalid)
        machine.hook_add(unicorn.UC_HOOK_MEM_WRITE, write)

    def run(self, fixture):
        from unicorn.x86_const import UC_X86_REG_FPCW, UC_X86_REG_FPSW
        self.faults.clear()
        self.writes.clear()
        try:
            values, boundary = self.original.run(fixture)
            return dict(original_outcome='established_observer_stop' if boundary else 'returned',
                        defined_prefix=values, semantic_boundary=boundary, faults=[],
                        writes=list(self.writes), fault_contract=False)
        except self.unicorn.UcError as error:
            machine = self.original.machine
            values = defined_prefix(machine, fixture, self.locations)
            values[17] = machine.reg_read(UC_X86_REG_FPSW) & 63
            values[18] = machine.reg_read(UC_X86_REG_FPCW)
            ranges = [('task', self.locations.TASK, 64), ('player', self.locations.PLAYER, 512),
                      ('world', self.locations.WORLD, 64), ('info', self.locations.INFO, 64),
                      ('actor_current', 0x3abdf60, 8), ('published_mode', 0x3c55d45, 1)]
            observed = dict(original_outcome='observed_memory_fault', error=str(error),
                            defined_prefix=values,
                            semantic_boundary=machine.reg_read(self.pc_register),
                            faults=list(self.faults), writes=list(self.writes),
                            original_fp_flags=machine.reg_read(UC_X86_REG_FPSW) & 63,
                            original_control=machine.reg_read(UC_X86_REG_FPCW), fault_contract=False)
            observed['mapped_state_hex'] = {name: bytes(machine.mem_read(address, size)).hex()
                                           for name, address, size in ranges}
            if len(self.faults) != 1 or not established_null_player_fault(fixture, self.faults[0]):
                observed['original_outcome'] = 'unexpected_error'
                return observed
            observed['fault_contract'] = True
            return observed


def compare_fault_contract(observed, old, current):
    """Compare defined words independently; native equality never excuses a prefix difference."""
    if observed['original_outcome'] != 'observed_memory_fault' or not observed['fault_contract']:
        raise ValueError('No established original fault contract')
    outputs = current['output']
    if len(outputs) != 40 or len(old['output']) != 40:
        raise ValueError('Incomplete native Sonic result')
    differences = [index for index, expected in observed['defined_prefix'].items()
                   if index != 17 and outputs[index] != expected]
    boundary_equal = (old['return_code'] == current['return_code'] == 0 and
                      outputs[0] == 2 and outputs[1] == observed['semantic_boundary'])
    publication_observed = any(write['address'] == 0x3abdf64 and write['size'] == 4 and
                               write['value'] == 0 for write in observed.get('writes', []))
    equality = old == current
    partial = outputs[17] != observed['original_fp_flags']
    return dict(original_outcome='observed_memory_fault', defined_prefix_differences=differences,
                native_boundary_correspondence=boundary_equal, complete_native_equality=equality,
                written_null_publication_observed=publication_observed,
                source_native_fp_partial=partial,
                qualified_bounded_fault_contract=not differences and boundary_equal and equality and publication_observed,
                original_returned=False)
