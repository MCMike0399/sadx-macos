"""Adversarial source fixtures for ownership/provenance migration."""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'tools/gate'))
from check_provenance import check, legacy_snapshot, legacy_owners


class ProvenanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        (self.root / 'src/engine').mkdir(parents=True)
        self.path = self.root / 'src/engine/frame.hpp'
        self.path.write_text('')
        self.database = {'symbols': [
            {'module': 'SONIC', 'address': '0x900000', 'kind': 'global', 'name': 'Counter', 'size': 4,
             'alternatives': [{'kind': 'global', 'name': 'OldCounter', 'size': 4}]},
            {'module': 'SONIC', 'address': '0x900002', 'kind': 'global', 'name': 'HighWord', 'size': 2},
            {'module': 'SONIC', 'address': '0x401000', 'kind': 'function', 'name': 'Tick', 'size': None},
            {'module': 'INPUT', 'address': '0x10000000', 'kind': 'global', 'name': 'Pad', 'size': 4},
        ]}

    def run_check(self, text, baseline=None):
        self.path.write_text(text)
        return check(self.root, self.database, baseline or {'identifiers': {}, 'owners': []})[0]

    def test_named_owner_alias_function_and_patch(self):
        errors = self.run_check('''struct Frame {
// GLOBAL: SONIC 0x900000 OldCounter size=4
std::optional<std::uint32_t> counter;
};
// FUNCTION: SONIC 0x401000 Tick
void tick() {}
// PATCH: SPEEDRUN +0x10f0 -> 0x7899e0
void limit_frame() {}
''')
        self.assertEqual(errors, [])

    def test_duplicate_owner_mutant_and_partial_overlap(self):
        original = '''struct Frame {
// GLOBAL: SONIC 0x900000 Counter size=4
std::uint32_t counter;
};
'''
        self.assertEqual(self.run_check(original), [])
        duplicate = original + original.replace('Frame', 'Other').replace('counter;', 'other;')
        errors = self.run_check(duplicate)
        self.assertEqual(sum('overlapping GLOBAL' in e for e in errors), 4)
        errors = self.run_check(original + '''struct High {
// GLOBAL: SONIC 0x900002 HighWord size=2
std::uint16_t high;
};
''')
        self.assertEqual(sum('overlapping GLOBAL' in e for e in errors), 2)

    def test_legacy_overlap_and_new_identifier(self):
        legacy = 'struct Old {\nstd::optional<std::uint32_t> word_at_900000;\n};\n'
        self.path.write_text(legacy)
        baseline = legacy_snapshot(self.root)
        self.assertEqual(len(legacy_owners(self.root)), 4)
        self.assertEqual(self.run_check(legacy, baseline), [])
        errors = self.run_check(legacy + '''struct New {
// GLOBAL: SONIC 0x900000 Counter size=4
std::uint32_t counter;
};
''', baseline)
        self.assertEqual(sum('overlapping GLOBAL' in e for e in errors), 4)
        self.assertTrue(any('new legacy identifier' in e for e in self.run_check(legacy)))

    def test_legacy_tables_and_blocks(self):
        text = '''struct Old {
SourceBytes<0x900000,8> block_at_900000;
std::uint32_t& borrowed_at_900100;
};
// owner-table: Other
0x900200
// end owner-table
'''
        self.path.write_text(text)
        owners = legacy_owners(self.root)
        self.assertEqual(len(owners), 12)
        self.assertNotIn(('SONIC', 0x900100), owners)

    def test_invalid_names_kinds_extents_and_declarations(self):
        cases = [
            ('// GLOBAL: SONIC 0x900000 Invented size=4\nint counter;', 'absent from'),
            ('// GLOBAL: SONIC 0x401000 Tick size=4\nint counter;', 'wrong kind'),
            ('// GLOBAL: SONIC 0x900000 Counter\nint counter;', 'needs size='),
            ('// GLOBAL: SONIC 0x900000 Counter size=8\nint counter;', 'invalid source size'),
            ('// GLOBAL: SONIC 0x900000 Counter size=0\nint counter;', 'invalid source size'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\nint& counter;', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\nint a, counter;', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\nint counter=0, second=0;', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\ntypedef int counter;', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\nauto counter = &original;', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\nauto counter = std::ref(original);', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\nconst extern int counter;', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\nint counter=0; int second;', 'owning value'),
            ('// GLOBAL: SONIC 0x900000 Counter size=4\n\nint counter;', 'immediately precede'),
            ('// GLOBAL: SONIC bad Counter size=4\nint counter;', 'malformed'),
        ]
        for source, expected in cases:
            with self.subTest(source=source):
                self.assertTrue(any(expected in e for e in self.run_check(source)))

    def test_duplicate_function_and_patch(self):
        entry = '// FUNCTION: SONIC 0x401000 Tick\nvoid tick() {}\n'
        self.assertTrue(any('duplicate source entry' in e for e in self.run_check(entry + entry)))
        patch = '// PATCH: SPEEDRUN +0x10f0 → 0x7899e0\nvoid limit() {}\n'
        self.assertTrue(any('duplicate PATCH' in e for e in self.run_check(patch + patch)))

    def test_removed_legacy_allowances_must_be_retired(self):
        errors = self.run_check('', {'identifiers': {'src/engine/frame.hpp': ['word_at_900000']}, 'owners': []})
        self.assertTrue(any('stale legacy allowance' in e for e in errors))

    def test_cosmetic_rename_cannot_discard_source_mapping(self):
        self.path.write_text('struct Old {\nstd::uint32_t word_at_900000;\n};\n')
        baseline = legacy_snapshot(self.root)
        baseline['identifiers'] = {}
        renamed = 'struct Frame {\nstd::uint32_t counter;\n};\n'
        self.assertTrue(any('lost source ownership' in e for e in self.run_check(renamed, baseline)))
        annotated = renamed.replace('std::uint32_t', '// GLOBAL: SONIC 0x900000 Counter size=4\nstd::uint32_t')
        self.assertEqual(self.run_check(annotated, baseline), [])
        self.assertTrue(any('lost source ownership' in e for e in self.run_check('/*\n' + annotated + '*/', baseline)))

    def test_callable_values_are_not_function_entries(self):
        for annotation in ['FUNCTION: SONIC 0x401000 Tick', 'PATCH: SPEEDRUN +0x10f0 -> 0x7899e0']:
            for declaration in ['std::function<void()> callback;', 'void (*callback)();']:
                with self.subTest(annotation=annotation, declaration=declaration):
                    errors = self.run_check('// ' + annotation + '\n' + declaration)
                    self.assertTrue(any('function declaration' in e for e in errors))

    def test_stub_reason_and_member_function(self):
        for reason in ['', ' awaiting native recovery', ' calls reference adapter; native fails loudly']:
            annotation = '// STUB: SONIC 0x401000 Tick' + reason + '\n'
            for declaration in ['void tick();', 'struct Seams {\n' + annotation + 'void tick();\n};']:
                source = declaration if declaration.startswith('struct') else annotation + declaration
                with self.subTest(reason=reason, declaration=declaration):
                    self.assertEqual(self.run_check(source), [])

    def test_stub_reason_cannot_hide_invalid_symbol_or_declaration(self):
        for declaration in ['std::function<void()> tick;', 'void (*tick)();', 'int tick;']:
            errors = self.run_check('// STUB: SONIC 0x401000 Tick unrecovered\n' + declaration)
            self.assertTrue(any('function declaration' in e for e in errors))
        for body in ['SONIC 0x401000 Invented waiting for recovery',
                     'SONIC 0x900000 Counter waiting for recovery',
                     'INPUT 0x401000 Tick waiting for recovery']:
            self.assertTrue(any('absent from' in e for e in self.run_check(
                '// STUB: ' + body + '\nvoid tick();')))

    def test_stub_reason_preserves_other_grammar_and_unique_entry(self):
        stub = '// STUB: SONIC 0x401000 Tick unrecovered\nvoid tick();\n'
        for duplicate in [stub, '// FUNCTION: SONIC 0x401000 Tick\nvoid tick() {}\n']:
            self.assertTrue(any('duplicate source entry' in e for e in self.run_check(stub + duplicate)))
        for kind in ['FUNCTION', 'LIBRARY', 'GLOBAL', 'DATA']:
            self.assertTrue(any('malformed' in e for e in self.run_check(
                '// ' + kind + ': SONIC 0x401000 Tick free text\nvoid tick();')))

    def test_commented_global_is_not_an_owner(self):
        self.assertEqual(self.run_check('/*\n// GLOBAL: SONIC 0x900000 Counter size=4\nint counter;\n*/'), [])

    def test_same_va_in_distinct_annotated_modules(self):
        self.database['symbols'].append({'module': 'SONIC', 'address': '0x10000000',
                                         'kind': 'global', 'name': 'Separate', 'size': 4})
        self.assertEqual(self.run_check('''// GLOBAL: INPUT 0x10000000 Pad size=4
std::uint32_t pad;
// GLOBAL: SONIC 0x10000000 Separate size=4
std::uint32_t separate;
'''), [])


if __name__ == '__main__':
    unittest.main()
