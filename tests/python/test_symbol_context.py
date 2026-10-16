"""Recovery-tool fixtures: module identity, graph joins, source evidence and REA envelopes."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[2] / 'tools/symbols'
sys.path.insert(0, str(TOOLS))
from source_index import ContextIndex, address


class SymbolContextTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        symbols = [dict(module=module, address=at, name=name, kind='function',
                        signature='(int value)', type='void', alternatives=[dict(name=name + 'Alias')])
                   for module, at, name in [('SONIC', '0x401000', 'First'),
                                           ('SONIC', '0x402000', 'Second'),
                                           ('INPUT', '0x401000', 'OtherModule')]]
        self.write('build/symbols/symbols.json', dict(symbols=symbols))
        self.write('reference/inventory/functions.json', dict(functions=[
            dict(address='0x401000', callees=['0x402000', 'EXTERNAL:0x1']),
            dict(address='0x402000', callees=[]),
        ]))
        self.write('tools/gate/profile_patches.json', dict(patches=[
            dict(va='0x401008', size=5, active=True, owner='loader'),
            dict(va='0x401010', size=1, active=False, owner='input'),
        ]))
        self.write('src/engine/flow.cpp', '''// FUNCTION: SONIC 0x401000 First
void first() {}
// STUB: SONIC 0x402000 Second waiting for recovery
void second();
/*
// FUNCTION: SONIC 0x404000 Fake
void fake();
*/
const char* text = R"(// FUNCTION: SONIC 0x405000 Fake)";
int legacy_406000(int value);
void caller() { legacy_407000(1); }
''')
        self.write('src/guest/overrides.cpp', '''std::vector<OverrideEntry> overrides() {
out.push_back({0x401000,"first",{},callback});
for(const auto entry:{0x402000U,0x403000U}){
    out.push_back({entry,"other",{},callback});
}
out.push_back({unknown,"unknown",{},callback});
// out.push_back({0x404000,"fake",{},callback});
const char* fake = "out.push_back({0x405000, invalid})";
}
''')
        body = dict(evidence_id='ev_fixture', operation='analyze_function',
                    parameters=dict(procedure='0x00401000'), subject=dict(name='sonic.exe'),
                    normalized_result=dict(procedure=dict(address='0x401000', body=dict(
                        ranges=[dict(start='0x401000', end='0x40100f')]))))
        self.write('reference/inventory-raw/arbitrary-label.json', dict(content=[
            dict(type='text', text=json.dumps(body))]))
        self.write('reference/inventory-raw/unrelated.json', dict(
            parameters=dict(procedure='0x402000'), normalized_result='mentions 0x401000'))

    def write(self, name, value):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(value if isinstance(value, str) else json.dumps(value))

    def test_lookup_identity_and_unknowns(self):
        index = ContextIndex(self.root)
        self.assertEqual(index.lookup('00401000')['symbol']['name'], 'First')
        self.assertEqual(index.lookup('401000', 'INPUT')['symbol']['name'], 'OtherModule')
        self.assertEqual(index.lookup('deadbeef')['symbol'], None)
        self.assertEqual(index.lookup('deadbeef')['native_status'], 'not_established')
        self.assertEqual(index.lookup('402000')['symbol']['alternatives'][0]['name'], 'SecondAlias')
        with self.assertRaises(ValueError):
            address('not-an-address')

    def test_source_matches_and_loop_expansion(self):
        index = ContextIndex(self.root)
        first = index.lookup('401000')
        self.assertEqual({e['kind'] for e in first['source_evidence']},
                         {'function_annotation', 'literal_override'})
        self.assertEqual(first['source_evidence'][0]['line'], 1)
        self.assertEqual(index.lookup('403000')['native_status'], 'loop_override')
        self.assertEqual(index.lookup('406000')['native_status'], 'legacy_declaration_candidate')
        for at in ['404000', '405000', '407000']:
            self.assertEqual(index.lookup(at)['source_evidence'], [])
        self.assertEqual(len(index.unresolved), 1)

    def test_bundle_cache_body_and_patch_intersection(self):
        result = ContextIndex(self.root).bundle('401000')
        self.assertEqual(result['callees'][0]['symbol']['name'], 'Second')
        self.assertEqual(result['callees'][1]['address'], 'EXTERNAL:0x1')
        self.assertEqual(result['callees'][1]['symbol'], None)
        self.assertEqual(result['profile']['scope'], 'cached_body_ranges')
        self.assertEqual(len(result['profile']['overlaps']), 1)
        self.assertEqual(result['cached_evidence']['matches'][0]['evidence_id'], 'ev_fixture')
        self.assertEqual(len(result['cached_evidence']['matches']), 1)
        second = ContextIndex(self.root).bundle('402000')
        self.assertEqual(second['callers'][0]['symbol']['name'], 'First')
        self.assertEqual(second['profile']['scope'], 'entry_byte_only')

    def test_explicit_range_missing_cache_and_bad_json(self):
        index = ContextIndex(self.root)
        result = index.bundle('401000', cache_dir=self.root / 'absent', end='401011')
        self.assertEqual(len(result['profile']['overlaps']), 2)
        self.assertFalse(result['cached_evidence']['directory_present'])
        with self.assertRaises(ValueError):
            index.bundle('401000', end='400fff')
        self.write('reference/inventory-raw/bad.json', '{broken')
        self.assertEqual(len(index.bundle('401000')['cached_evidence']['unreadable']), 1)
        other = index.bundle('401000', module='INPUT')
        self.assertIsNone(other['inventory'])
        self.assertEqual(other['profile']['scope'], 'not_applicable')

    def test_batch_cache_and_missing_extent_are_explicit(self):
        self.write('reference/inventory-raw/batch.json', dict(
            evidence_id='ev_batch', parameters=dict(addresses=['0x402000', '0x403000']),
            normalized_result=dict(items=[dict(address='0x402000', pseudocode='void second() {}')])))
        result = ContextIndex(self.root).bundle('402000')
        self.assertTrue(result['missing_facts'])
        self.assertEqual(result['profile']['scope'], 'entry_byte_only')
        self.assertIn('ev_batch', [m['evidence_id'] for m in result['cached_evidence']['matches']])
        self.write('src/guest/overrides.cpp', '')
        self.assertEqual(ContextIndex(self.root).lookup('402000')['native_status'], 'stub_annotation')
        self.write('reference/inventory-raw/bad-envelope.json', dict(parameters=None))
        self.assertEqual(len(ContextIndex(self.root).bundle('401000')['cached_evidence']['unreadable']), 1)

    def test_cli_without_original_binaries(self):
        for command, arguments in [('lookup.py', ['401000', '402000']),
                                   ('context_bundle.py', ['401000'])]:
            result = subprocess.run([sys.executable, str(TOOLS / command), '--root',
                                     str(self.root), *arguments], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(json.loads(result.stdout))
        result = subprocess.run([sys.executable, str(TOOLS / 'lookup.py'), '--root',
                                 str(self.root), 'invalid'], capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn('invalid hexadecimal address', result.stderr)


if __name__ == '__main__':
    unittest.main()
