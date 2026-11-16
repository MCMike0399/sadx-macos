"""Fold pinned Amy animation and weld declarations without executing game code."""
import struct

from gamma_declarations import fold
from sonic_actor_resources import digest

# The reached initializer writes 102 sixteen-byte PL_ACTION rows: the classic
# amy_action declaration counts 101, and the final row lands in the 16-byte gap
# before NPCAmyWeldInfo. The fold uses the bytes the source actually writes.
ANIMATION = (0x7cb6d0, 0x7ccb70, 0x3c54880, 102 * 16,
             'fb0a0c2d5fb78b2ba692311c2a8d9cc440e1e4f5cec2431de49d2e72f2ba91af')
WELD = (0x7ccb90, 0x7ccff7, 0x3c546d0, 18 * 24,
        '8fddcd3049ecf49e3ad000e9b4529c9aee7e5761b9819d7e03e6664a61f9c7bb')
ACTION_SLOT = 0x3aad0e0
OBJECT_SLOT = 0x38f6e70
CONSTRUCTED_ADDRESS = 0x3c5d538
DECLARATIONS = 80
RECIPES = 102


def recipes(exe, action_table, declarations):
    if len(declarations) != DECLARATIONS:
        raise ValueError('Amy action declaration count changed')
    table = struct.pack('<80I', *declarations)
    raw, proof = fold(exe, ANIMATION, {ACTION_SLOT: action_table}, {action_table: table})
    # Sentinel folding identifies declaration indices despite source pointer aliases.
    sentinels = [0x50000000 + 16 * i for i in range(DECLARATIONS)]
    sentinel_raw, _ = fold(exe, ANIMATION, {ACTION_SLOT: action_table},
                           {action_table: struct.pack('<80I', *sentinels)})
    rows = []
    for index in range(RECIPES):
        action, count, mode, next_action, transition, speed = \
            struct.unpack_from('<IBbHII', raw, index * 16)
        marker = struct.unpack_from('<I', sentinel_raw, index * 16)[0]
        if marker == CONSTRUCTED_ADDRESS:
            declaration = 'constructed'
        elif marker == 0:
            if action != 0:
                raise ValueError(f'Amy animation row {index} has unknown declaration {marker:#x}')
            declaration = 'known_null'
        elif 0x50000000 <= marker < 0x50000000 + DECLARATIONS * 16 and marker % 16 == 0:
            declaration = (marker - 0x50000000) // 16
        else:
            raise ValueError(f'Amy animation row {index} has unknown declaration {marker:#x}')
        rows.append({'row': index, 'declaration': declaration, 'object_count': count,
                     'mode': mode, 'next': next_action,
                     'transition_bits': hex(transition), 'speed_bits': hex(speed)})
    proof['sentinel_recipe_bytes_sha256'] = digest(sentinel_raw)
    proof['reconstructed_bytes_sha256'] = digest(raw)
    return rows, raw, proof


def weld(exe, object_table, declarations):
    if len(declarations) != 39:
        raise ValueError('Amy object declaration count changed')
    return fold(exe, WELD, {OBJECT_SLOT: object_table},
                {object_table: struct.pack('<39I', *declarations)})
