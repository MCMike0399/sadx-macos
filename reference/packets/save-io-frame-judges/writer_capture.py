"""Parse the fixed before-source native writer capture without emulator imports."""
import re


def save_bytes(value):
    if len(value) != 2784 or re.fullmatch(r'[0-9a-fA-F]+', value) is None:
        raise ValueError('Writer snapshot must contain all 1392 encoded bytes')
    return value


def native_cases(text):
    headers = list(re.finditer(r'^writer:([^\n]*)$', text, re.MULTILINE))
    if [header[1] for header in headers] != [str(i) for i in range(39)]:
        raise ValueError('Writer capture requires exactly ordered IDs 0 through 38')
    result = []
    for index, header in enumerate(headers):
        end = headers[index + 1].start() if index + 1 < len(headers) else len(text)
        lines = text[header.end():end].splitlines()
        snapshots, written = [], None
        terminal, outcome = False, False
        cursor = 0
        while cursor < len(lines):
            line = lines[cursor]
            prefix, separator, rest = line.partition(':')
            if separator and prefix.isdigit():
                if terminal:
                    raise ValueError('Writer snapshot appears after its terminal snapshot')
                event = int(prefix)
                values = [int(value) & 0xffffffff for value in rest.split(',')]
                if len(values) != 14 or cursor + 2 >= len(lines):
                    raise ValueError('Writer snapshot has incomplete scalar or byte payload')
                encoded = save_bytes(lines[cursor + 1])
                allocation = lines[cursor + 2].split(',')
                if allocation[-1] != '' or len(allocation) != 15:
                    raise ValueError('Writer retained allocation marker is incomplete')
                if any(not value.isdigit() or not 0 <= int(value) <= 255
                       for value in allocation[:-1]):
                    raise ValueError('Writer retained allocation marker is malformed')
                # Validate allocation shape only. Its pre-format contents remain
                # unknown to the original observer and are not newly compared.
                if event == 99:
                    if not outcome:
                        raise ValueError('Writer terminal snapshot requires a return or failure marker')
                    terminal = True
                elif outcome:
                    raise ValueError('Writer callback snapshot appears after its return or failure')
                snapshots.append((event, values, encoded))
                cursor += 3
                continue
            if not terminal:
                if prefix == 'written':
                    if written is not None:
                        raise ValueError('Duplicate writer byte payload')
                    written = save_bytes(rest)
                elif prefix in ('return', 'invalid') or line == 'fault':
                    if outcome:
                        raise ValueError('Duplicate writer return or failure marker')
                    if prefix == 'return':
                        int(rest)
                    outcome = True
            cursor += 1
        if not terminal:
            raise ValueError('Writer scenario lacks its complete terminal snapshot')
        result.append({'snapshots': snapshots, 'written': written})
    return result
