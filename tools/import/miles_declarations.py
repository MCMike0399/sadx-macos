"""Fold pinned Miles animation and weld declarations without executing game code."""
import struct

from gamma_declarations import fold
from sonic_actor_resources import digest

# InitTailsAnimData writes 136 sixteen-byte PL_ACTION rows into TailsAnimData;
# InitTailsWeldInfo writes 30 rows into TailsWeldInfo.
ANIMATION = (0x7c51b0, 0x7c6d3f, 0x3c49d90, 136 * 16,
             '88b11e2d6fa00963f970ac48d614538cd50286bce6c2baeba24a015ddb1e2a11')
WELD = (0x7c6d80, 0x7c7560, 0x3c4a610, 30 * 24,
        '51b53c3796bad3cbc2b48639e2355029d771dcb6e376b785f51968223abf8aca')
ACTION_SLOT = 0x3a7cf20
OBJECT_SLOT = 0x3aad124
CONSTRUCTED_ADDRESS = 0x3c5d540
# ___MILES_ACTIONS and ___MILES_OBJECTS export spacing gives 114 and 72 words.
DECLARATIONS = 114
RECIPES = 136
OBJECT_DECLARATIONS = 72


def recipes(exe, action_table, declarations):
    if len(declarations) != DECLARATIONS:
        raise ValueError('Miles action declaration count changed')
    table = struct.pack('<%dI' % DECLARATIONS, *declarations)
    raw, proof = fold(exe, ANIMATION, {ACTION_SLOT: action_table}, {action_table: table})
    # Sentinel folding identifies declaration indices despite source pointer aliases.
    sentinels = [0x50000000 + 16 * i for i in range(DECLARATIONS)]
    sentinel_raw, _ = fold(exe, ANIMATION, {ACTION_SLOT: action_table},
                           {action_table: struct.pack('<%dI' % DECLARATIONS, *sentinels)})
    rows = []
    for index in range(RECIPES):
        action, count, mode, next_action, transition, speed = \
            struct.unpack_from('<IBbHII', raw, index * 16)
        marker = struct.unpack_from('<I', sentinel_raw, index * 16)[0]
        if marker == CONSTRUCTED_ADDRESS:
            declaration = 'constructed'
        elif marker == 0:
            if action != 0:
                raise ValueError(f'Miles animation row {index} has unknown declaration {marker:#x}')
            declaration = 'known_null'
        elif 0x50000000 <= marker < 0x50000000 + DECLARATIONS * 16 and marker % 16 == 0:
            declaration = (marker - 0x50000000) // 16
        else:
            raise ValueError(f'Miles animation row {index} has unknown declaration {marker:#x}')
        rows.append({'row': index, 'declaration': declaration, 'object_count': count,
                     'mode': mode, 'next': next_action,
                     'transition_bits': hex(transition), 'speed_bits': hex(speed)})
    proof['sentinel_recipe_bytes_sha256'] = digest(sentinel_raw)
    proof['reconstructed_bytes_sha256'] = digest(raw)
    return rows, raw, proof


def weld(exe, object_table, declarations):
    if len(declarations) != OBJECT_DECLARATIONS:
        raise ValueError('Miles object declaration count changed')
    return fold(exe, WELD, {OBJECT_SLOT: object_table},
                {object_table: struct.pack('<%dI' % OBJECT_DECLARATIONS, *declarations)})
