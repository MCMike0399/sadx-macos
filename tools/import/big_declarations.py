"""Fold pinned Big animation and weld declarations without executing game code."""
import struct

from gamma_declarations import fold
from sonic_actor_resources import digest

ANIMATION = (0x7cd340, 0x7ce81c, 0x3c556a0, 105 * 16,
             '9dc8562c5480f5d0df7e11284f968599a842524c2162ac37b26e2543c5598f17')
WELD = (0x7ce860, 0x7cec89, 0x3c55500, 17 * 24,
        '5c453b2fc83ed5a9815fd71b9e9cf75171aba0db2e47c5c02e764a213b53f919')
ACTION_SLOT = 0x3aad104
OBJECT_SLOT = 0x38f6e98
CONSTRUCTED_ADDRESS = 0x3c5d520
DECLARATIONS = 90
RECIPES = 105


def recipes(exe, action_table, declarations):
    if len(declarations) != DECLARATIONS:
        raise ValueError('Big action declaration count changed')
    table = struct.pack('<90I', *declarations)
    raw, proof = fold(exe, ANIMATION, {ACTION_SLOT: action_table}, {action_table: table})
    # Sentinel folding identifies declaration indices despite source pointer aliases.
    sentinels = [0x50000000 + 16 * i for i in range(DECLARATIONS)]
    sentinel_raw, _ = fold(exe, ANIMATION, {ACTION_SLOT: action_table},
                           {action_table: struct.pack('<90I', *sentinels)})
    rows = []
    for index in range(RECIPES):
        action, count, mode, next_action, transition, speed = \
            struct.unpack_from('<IBbHII', raw, index * 16)
        marker = struct.unpack_from('<I', sentinel_raw, index * 16)[0]
        if marker == CONSTRUCTED_ADDRESS:
            declaration = 'constructed'
        elif marker == 0:
            if action != 0:
                raise ValueError(f'Big animation row {index} has unknown declaration {marker:#x}')
            declaration = 'known_null'
        elif 0x50000000 <= marker < 0x50000000 + DECLARATIONS * 16 and marker % 16 == 0:
            declaration = (marker - 0x50000000) // 16
        else:
            raise ValueError(f'Big animation row {index} has unknown declaration {marker:#x}')
        rows.append({'row': index, 'declaration': declaration, 'object_count': count,
                     'mode': mode, 'next': next_action,
                     'transition_bits': hex(transition), 'speed_bits': hex(speed)})
    proof['sentinel_recipe_bytes_sha256'] = digest(sentinel_raw)
    proof['reconstructed_bytes_sha256'] = digest(raw)
    return rows, raw, proof


def weld(exe, object_table, declarations):
    if len(declarations) != 46:
        raise ValueError('Big object declaration count changed')
    return fold(exe, WELD, {OBJECT_SLOT: object_table},
                {object_table: struct.pack('<46I', *declarations)})
