"""Fold pinned Knuckles animation and weld declarations without executing game code."""
import capstone
import struct

from gamma_declarations import fold
from sonic_actor_resources import digest

# InitKnucklesAnimData writes 115 sixteen-byte PL_ACTION rows into knuckles_action;
# InitKnucklesWeldInfo writes KnucklesWeldInfo's 30 twenty-four-byte join rows.
# The initializer body is linear MOV/XOR/push/pop/ret, so its end is the first ret.
ANIMATION_START = 0x7c7d40
ANIMATION_DESTINATION = 0x3c532a0
ANIMATION_ROWS = 115
ANIMATION_SHA256 = '7d96b99ca2b5bc9a53dd8ac91643116c3df22a4ee22149f6607556cc6705bde5'
WELD_START = 0x7c94d0
WELD_DESTINATION = 0x3c52d68
WELD_ROWS = 30
WELD_SHA256 = 'af60b28534eb2fc80a819016ab344fa66bfbb7765d9cad2eec8bb92226b96acd'
ACTION_SLOT = 0x3a7cf28
OBJECT_SLOT = 0x3a7cf1c
CONSTRUCTED_ADDRESS = 0x3c5d528
# ___KNUCKLES_ACTIONS and ___KNUCKLES_OBJECTS export spacing gives 90 and 74 words.
DECLARATIONS = 90
RECIPES = 115
OBJECT_DECLARATIONS = 74


def first_return_end(exe, start):
    """Return the end of a linear declaration writer: one past its first ret."""
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    for instruction in decoder.disasm(exe.read(start, 0x4000), start):
        if instruction.mnemonic == 'ret':
            return instruction.address + instruction.size
    raise ValueError(f'Knuckles declaration initializer has no return at {start:#x}')


def initializer(exe, start, destination, row_bytes, rows, expected):
    end = first_return_end(exe, start)
    if digest(exe.read(start, end - start)) != expected:
        raise ValueError(f'Knuckles declaration initializer bytes changed at {start:#x}')
    return (start, end, destination, rows * row_bytes, expected)


def recipes(exe, action_table, declarations):
    if len(declarations) != DECLARATIONS:
        raise ValueError('Knuckles action declaration count changed')
    table = struct.pack('<%dI' % DECLARATIONS, *declarations)
    animation = initializer(exe, ANIMATION_START, ANIMATION_DESTINATION, 16, RECIPES,
                            ANIMATION_SHA256)
    raw, proof = fold(exe, animation, {ACTION_SLOT: action_table}, {action_table: table})
    # Sentinel folding identifies declaration indices despite source pointer aliases.
    sentinels = [0x50000000 + 16 * i for i in range(DECLARATIONS)]
    sentinel_raw, _ = fold(exe, animation, {ACTION_SLOT: action_table},
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
                raise ValueError(f'Knuckles animation row {index} has unknown declaration {marker:#x}')
            declaration = 'known_null'
        elif 0x50000000 <= marker < 0x50000000 + DECLARATIONS * 16 and marker % 16 == 0:
            declaration = (marker - 0x50000000) // 16
        else:
            raise ValueError(f'Knuckles animation row {index} has unknown declaration {marker:#x}')
        rows.append({'row': index, 'declaration': declaration, 'object_count': count,
                     'mode': mode, 'next': next_action,
                     'transition_bits': hex(transition), 'speed_bits': hex(speed)})
    proof['sentinel_recipe_bytes_sha256'] = digest(sentinel_raw)
    proof['reconstructed_bytes_sha256'] = digest(raw)
    return rows, raw, proof


def weld(exe, object_table, declarations):
    if len(declarations) != OBJECT_DECLARATIONS:
        raise ValueError('Knuckles object declaration count changed')
    spec = initializer(exe, WELD_START, WELD_DESTINATION, 24, WELD_ROWS, WELD_SHA256)
    return fold(exe, spec, {OBJECT_SLOT: object_table},
                {object_table: struct.pack('<%dI' % OBJECT_DECLARATIONS, *declarations)})
