"""Declaration/selection fixtures independent of the local game and symbol sources."""
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'tools/symbols'))
from build_symbol_db import MACROS, merge, parse_header, seeds


class SymbolTests(unittest.TestCase):
    def test_all_macro_forms(self):
        declarations = {
            'FunctionPointer': 'int, Tick, (int x, void (*cb)(int, int)), 0x401000',
            'VoidFunc': 'Idle, 0x401010',
            'ThiscallFunctionPointer': 'int, Method, (void* self), 0x401020',
            'FastcallFunctionPointer': 'int, Fast, (int a, int b), 0x401030',
            'StdcallFunctionPointer': 'int, System, (int a), 0x401040',
            'UsercallFunc': 'int, User, (int a, int b), (a, b), 0x401050, rEAX, rECX, stack4',
            'UsercallFuncVoid': 'UserVoid, (int a), (a), 0x401060, rEAX',
            'TaskFunc': 'RunTask, 0x401070',
            'ObjectFunc': 'RunObject, 0x401080',
            'CamFunc': 'RunCamera, 0x401090',
            'CamAdjustFunc': 'AdjustCamera, 0x4010a0',
            'DataPointer': 'float, Factor, 0x900000',
            'DataArray': 'NJS_VECTOR*, Points, 0x900010, 12',
        }
        self.assertEqual(set(declarations), MACROS)
        entries, _ = parse_header('\n'.join(f'{m}({a});' for m, a in declarations.items()), 'fixture.h', 10)
        self.assertEqual([e['macro'] for e in entries], list(declarations))
        self.assertEqual(entries[0]['signature'], '(int x, void (*cb)(int, int))')
        self.assertEqual(entries[5]['address'], '0x401050')
        self.assertEqual(entries[6]['type'], 'void')
        self.assertEqual(entries[7]['signature'], '(task* tp)')
        self.assertEqual(entries[-2]['size'], 4)
        self.assertEqual(entries[-1]['size'], 48)

    def test_comments_definitions_types_and_unknown_sizes(self):
        text = '''#define VoidFunc(NAME, ADDRESS) FunctionPointer(void, NAME, (), ADDRESS)
// VoidFunc(Comment, 0x400001);
/* DataPointer(int, Hidden, 0x900001); */
struct Record { int x; struct Nested { float y; }; };
enum class Mode { Idle, Busy };
DataArray(Record, Records, 0x900000, 2);
DataArray(int, Counts, 0x900100, NUM_COUNTS);
'''
        entries, types = parse_header(text, 'fixture.h', 10)
        self.assertEqual([e['name'] for e in entries], ['Records', 'Counts'])
        self.assertTrue(all(e['size'] is None for e in entries))
        self.assertEqual({t['name'] for t in types}, {'Record', 'Nested', 'Mode'})
        self.assertIn('int x;', types[0]['declaration'])
        with self.assertRaisesRegex(ValueError, 'unresolved VoidFunc address'):
            parse_header('VoidFunc(Unknown, ADDRESS);', 'fixture.h', 10)

    def test_precedence_keeps_alternatives_and_conflicts(self):
        classic, _ = parse_header('VoidFunc(OldName, 0x401000);', 'classic.h', 20)
        new, _ = parse_header('VoidFunc(SymbolName, 0x401000);', 'new.h', 10)
        selected, conflicts = merge(classic + new)
        self.assertEqual(selected[0]['name'], 'SymbolName')
        self.assertEqual(selected[0]['alternative_names'], ['OldName'])
        self.assertEqual(len(conflicts), 1)
        override = dict(new[0], name='ReviewedName', priority=0, evidence_id='fixture')
        selected, conflicts = merge(classic + new + [override])
        self.assertEqual(selected[0]['name'], 'ReviewedName')
        self.assertEqual(conflicts[0]['reason'], 'project override')

    def test_seeds_preserve_callbacks_and_disambiguate_names(self):
        entries, _ = parse_header('VoidFunc(Tick, 0x401000); VoidFunc(Tick, 0x401010); '
                                 'DataPointer(int, Counter, 0x900000);', 'fixture.h', 10)
        original = '0x401000\tfunction\tConfirmedCallback\n0x402000\tfunction\n'
        result = seeds(entries, original)
        self.assertIn('0x401000\tfunction\tConfirmedCallback\n', result)
        self.assertIn('0x402000\tfunction\n', result)
        self.assertIn('0x900000\tlabel\tCounter\n', result)
        duplicate = seeds(entries, '')
        self.assertIn('Tick_401000', duplicate)
        self.assertIn('Tick_401010', duplicate)


if __name__ == '__main__':
    unittest.main()
