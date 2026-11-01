"""Signature discrimination and real Git CLI migration/ratchet mutants."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

GATE = Path(__file__).resolve().parents[2] / 'tools/gate'
sys.path.insert(0, str(GATE))
from check_address_dispatch import (BASELINE, REGISTRY, VERSION, census, scan)
from check_readability import METRICS, measure

PATH = 'src/engine/fixture.cpp'
FIRST = 'void dispatch(unsigned target, unsigned color) { transport(target, color); }'
LATER = 'void call(Rt& rt, Frame& f, Fp& fp, unsigned callee, unsigned site) { rt.external(callee, site); }'


def spec(definition=LATER, helper='call', target=4, minimum=5, maximum=5):
    return dict(path=PATH, helper=helper, definition=definition,
                definition_name_offset=definition.index(helper), target_argument=target,
                min_args=minimum, max_args=maximum, receivers=[''],
                evidence='Synthetic fixture: named target forwarded to transport/external.')


class SignatureTests(unittest.TestCase):
    def test_first_and_later_targets_not_other_hex_arguments(self):
        signatures = [spec(FIRST, 'dispatch', 1, 2, 2), spec()]
        source = FIRST + '\n' + LATER + '''
dispatch(4900000, 0xffffffff);
call(rt,f,fp, kAlias, 0x49aa00);
call(rt,f,fp, selected ? 0x49a9b0 : 4900000, mask(0xffff));
paint(color, 0xffffffff, 0xffff);
'''
        row, sites = census(source, PATH, signatures)
        self.assertEqual(row['dispatch_sites'], 3)
        self.assertEqual([s['target_expression'] for s in sites],
                         ['4900000', 'kAlias', 'selected ? 0x49a9b0 : 4900000'])
        self.assertIn('paint', row['candidate_names'])
        self.assertEqual(sites[0]['line'], 3)
        self.assertEqual(sites[0]['column'], 1)

    def test_multiline_nested_and_each_site_once(self):
        source = LATER + '''
call(
 rt, f, fp,
 choose(a, table[index(1,2)], Thing{1,2}),
 call(rt,f,fp, variable, 0x49aa00));
'''
        row, sites = census(source, PATH, [spec()])
        self.assertEqual(row['dispatch_sites'], 2)
        self.assertEqual(sites[0]['target_expression'], 'choose(a, table[index(1,2)], Thing{1,2})')
        self.assertEqual(sites[1]['target_expression'], 'variable')

    def test_comments_raw_strings_and_constexpr_are_not_exemptions(self):
        source = LATER + r'''
// call(rt,f,fp, fake, 0);
/* call(rt,f,fp, fake, 0); */
auto text = "call(rt,f,fp, fake, 0)";
auto raw = u8R"tag(call(rt,f,fp, fake, 0); " )tag";
constexpr unsigned kAlias = 0x49a9b0;
constexpr unsigned kColor = 0xffffffff;
constexpr unsigned kMask = 0xffff;
call(rt, f, fp, /* target */ kAlias, 0xffffffff);
'''
        row, sites = census(source, PATH, [spec()])
        self.assertEqual(row['dispatch_sites'], 1)
        self.assertEqual(sites[0]['target_expression'], 'kAlias')
        self.assertEqual(measure('constexpr unsigned kColor=0xffffffff;')["hex_literals"], 0)
        self.assertEqual(measure('constexpr unsigned kMask=0xffff;')["hex_literals"], 0)

    def test_no_generic_name_or_member_proof(self):
        row, sites = census('call(0x49a9b0, mask); invoke(alias); paint(0xffffffff);', PATH, [])
        self.assertEqual(sites, [])
        self.assertEqual(row['candidate_names'], {'call': 1, 'invoke': 1, 'paint': 1})
        row, sites = census(LATER + '\nsemantic.call(1,2,3,4,0xffffffff);', PATH, [spec()])
        self.assertEqual(sites, [])
        self.assertEqual(row['candidate_names']['call'], 1)
        row, sites = census(LATER + '\n(object).call(1,2,3,4,5); ::call(1,2,3,4,5);', PATH, [spec()])
        self.assertEqual(sites, [])
        self.assertEqual(row['candidate_names']['call'], 2)

    def test_changed_moved_and_spoofed_definitions_fail(self):
        for source in (LATER.replace('callee', 'color'), '// ' + LATER,
                       'auto raw=R"tag(' + LATER + ')tag";', LATER + '\n' + LATER):
            with self.subTest(source=source), self.assertRaises(ValueError):
                census(source, PATH, [spec()])
        with self.assertRaises(ValueError):
            scan({'src/engine/moved.cpp': LATER}, {'signatures': [spec()]})

    def test_new_same_name_overloads_and_lambdas_require_review(self):
        for addition in ('void call(int a,int b,int c,int d,int e) {}',
                         'void call(int a,int b,int c,int d,int e) noexcept {}',
                         'struct Painter { void call(int a,int b,int c,int d,int e) const {} };',
                         'auto call(int a,int b,int c,int d,int e) -> int { return d; }',
                         'auto call = [&](int a,int b,int c,int d,int e) { paint(d); };'):
            with self.subTest(addition=addition), self.assertRaises(ValueError):
                census(LATER + '\n' + addition, PATH, [spec()])

    def test_template_commas_fail_closed_or_preserve_parenthesized_target(self):
        contract = spec(maximum=7)
        for expression in ('call(rt,f,fp,choose<int,int>(a,b),site);',
                           'call(choose<int,int>(a,b),f,fp,target,site);'):
            with self.subTest(expression=expression), self.assertRaisesRegex(ValueError, 'ambiguous angle'):
                census(LATER + '\n' + expression, PATH, [contract])
        _, sites = census(LATER + '\ncall(rt,f,fp,(choose<int,int>(a,b)),site);', PATH, [contract])
        self.assertEqual(sites[0]['target_expression'], '(choose<int,int>(a,b))')

    def test_arity_bounds_and_nesting_fail_closed(self):
        for expression in ('call(rt,f,fp,x);', 'call(rt,f,fp,x,0,extra);',
                           'call(rt,f,fp,x,0;', 'call(rt,f,fp,' + '(' * 130 + 'x' + ')' * 130 + ',0);',
                           'call(rt,f,fp,' + ' ' * 65536 + 'x,0);'):
            with self.subTest(expression=expression[:80]), self.assertRaises(ValueError):
                census(LATER + '\n' + expression, PATH, [spec()])


class CliTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / PATH
        self.source.parent.mkdir(parents=True)
        self.original = LATER + '\ncall(rt,f,fp,kAlias,site);\n'
        self.source.write_text(self.original)
        registry = self.root / REGISTRY
        registry.parent.mkdir(parents=True)
        registry.write_text(json.dumps(dict(schema=1, measurement=VERSION, signatures=[spec()])))
        # Preserve real schema-2 allowance bytes, including a stricter old metric.
        self.legacy = self.root / 'reference/readability-baseline.json'
        self.legacy.parent.mkdir()
        self.legacy.write_text(json.dumps({'schema': 2, 'files': {PATH: dict.fromkeys(METRICS, 0)}}))
        self.legacy_bytes = self.legacy.read_bytes()
        self.git('init', '-q')
        self.commit()

    def git(self, *args):
        return subprocess.run(['git', *args], cwd=self.root, check=True, capture_output=True, text=True)

    def commit(self):
        self.git('add', '.')
        self.git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid',
                 'commit', '-qm', 'fixture')

    def check(self, *args):
        return subprocess.run([sys.executable, str(GATE / 'check_address_dispatch.py'),
                               '--root', str(self.root), *args], capture_output=True, text=True)

    def migrate(self):
        result = self.check('--migrate-signature-v1')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('MEASUREMENT CHANGE', result.stdout)
        self.assertEqual(self.legacy.read_bytes(), self.legacy_bytes)
        self.commit()

    def test_dirty_migration_cannot_seed_dispatch_debt(self):
        self.assertEqual(self.check().returncode, 1)
        for addition in ('call(rt,f,fp,4900000,site);\n',):
            self.source.write_text(self.original + addition)
            result = self.check('--migrate-signature-v1', '--write-baseline')
            self.assertEqual(result.returncode, 1, result.stdout)
            self.assertIn('immutable HEAD migration', result.stdout)
            self.assertFalse((self.root / BASELINE).exists())
        self.source.write_text(self.original)
        extra = self.source.with_name('new.cpp')
        extra.write_text('void update_player() { advance_animation(); }')
        self.assertEqual(self.check('--migrate-signature-v1').returncode, 0)
        extra.unlink()
        # A dirty reduction cannot lower the migration's original HEAD allowances.
        self.source.write_text(LATER)
        self.assertEqual(self.check('--migrate-signature-v1').returncode, 0)
        row = json.loads((self.root / BASELINE).read_text())['files'][PATH]
        self.assertEqual(row['dispatch_sites'], 1)
        self.assertEqual(self.legacy.read_bytes(), self.legacy_bytes)

    def test_forged_initial_migration_and_registry_cannot_seed_dirty_debt(self):
        self.assertEqual(self.check('--migrate-signature-v1').returncode, 0)
        baseline = self.root / BASELINE
        data = json.loads(baseline.read_text())
        data['files'][PATH]['dispatch_sites'] = 9
        data['files'][PATH]['candidate_names']['new_helper'] = 9
        baseline.write_text(json.dumps(data))
        self.source.write_text(self.original + 'new_helper(variable);\n')
        result = self.check('--migrate-signature-v1', '--write-baseline')
        self.assertEqual(result.returncode, 1)
        self.assertIn('baseline increase against HEAD', result.stdout)
        baseline.unlink()
        registry = self.root / REGISTRY
        data = json.loads(registry.read_text())
        new_definition = 'void new_helper(unsigned target) { owner(target); }'
        new_spec = spec(new_definition, 'new_helper', 1, 1, 1)
        data['signatures'].append(new_spec)
        registry.write_text(json.dumps(data))
        self.source.write_text(self.original + new_definition + '\nnew_helper(variable);')
        result = self.check('--migrate-signature-v1')
        self.assertEqual(result.returncode, 1)
        self.assertIn('changed/moved: new_helper', result.stdout)

    def test_forged_working_baseline_and_new_files_rejected(self):
        self.migrate()
        baseline = self.root / BASELINE
        data = json.loads(baseline.read_text())
        data['files'][PATH]['dispatch_sites'] = 2
        baseline.write_text(json.dumps(data))
        self.source.write_text(self.original + 'call(rt,f,fp,target,site);\n')
        for options in ((), ('--write-baseline',), ('--migrate-signature-v1',)):
            result = self.check(*options)
            self.assertEqual(result.returncode, 1, result.stdout)
            self.assertIn('baseline increase against HEAD', result.stdout)
        self.source.write_text(self.original)
        self.assertEqual(self.check('--write-baseline').returncode, 1)
        self.git('restore', BASELINE)
        self.source.with_name('new.cpp').write_text('void update_player() { advance_animation(); }')
        self.assertEqual(self.check().returncode, 0)

    def test_rename_move_new_helper_and_registry_changes_require_review(self):
        self.migrate()
        self.source.write_text(self.original.replace('call', 'renamed'))
        self.assertIn('changed/moved', self.check().stdout)
        self.source.write_text(self.original)
        self.source.rename(self.source.with_name('moved.cpp'))
        self.assertIn('removed/moved', self.check().stdout)
        self.source.with_name('moved.cpp').rename(self.source)
        self.source.write_text(self.original + '\nvoid new_helper(unsigned pc) { owner(pc); }\nnew_helper(alias);')
        self.assertEqual(self.check().returncode, 0)
        self.assertIn('report only', self.check().stdout)
        self.source.write_text(self.original)
        registry = self.root / REGISTRY
        data = json.loads(registry.read_text())
        data['signatures'][0]['target_argument'] = 5
        registry.write_text(json.dumps(data))
        self.assertIn('named migration required', self.check().stdout)

    def test_lowered_ratchet_and_legacy_ratchets_cannot_be_laundered(self):
        self.migrate()
        self.source.write_text(LATER + '\n')
        self.assertEqual(self.check('--write-baseline').returncode, 0)
        self.commit()
        self.source.write_text(self.original)
        self.assertEqual(self.check('--write-baseline').returncode, 1)
        # Separate census never grants or rewrites any legacy allowance. Run the
        # original CLI as integration must: old hex debt still fails independently.
        self.source.write_text(LATER + '\npaint(0x49a9b0);\n')
        result = subprocess.run([sys.executable, str(GATE / 'check_readability.py'),
                                 '--root', str(self.root), '--write-baseline'],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn('hex_literals 1 > 0', result.stdout)
        self.assertEqual(self.legacy.read_bytes(), self.legacy_bytes)

    def test_source_archive_enforces_supplied_baseline_without_writing_it(self):
        self.migrate()
        self.root.joinpath('.git').rename(self.root / 'saved-git')
        baseline = self.root / BASELINE
        original = baseline.read_bytes()
        self.assertEqual(self.check().returncode, 0)
        for options in (('--write-baseline',), ('--migrate-signature-v1',)):
            self.assertEqual(self.check(*options).returncode, 1)
            self.assertEqual(baseline.read_bytes(), original)
        self.source.write_text(self.original + 'call(rt,f,fp,alias,site);')
        result = self.check()
        self.assertEqual(result.returncode, 1)
        self.assertIn('archive baseline', result.stdout)

    def test_new_semantic_module_is_reported_while_known_dispatch_still_blocks(self):
        self.migrate()
        self.source.with_name('player.cpp').write_text(
            'void update_player() { advance_animation(); }')
        report = self.root / 'report.json'
        result = self.check('--report', str(report))
        self.assertEqual(result.returncode, 0, result.stdout)
        rows = json.loads(report.read_text())['files']['src/engine/player.cpp']['candidate_names']
        self.assertEqual(rows, {'update_player': 1, 'advance_animation': 1})
        self.source.write_text(self.original + 'call(rt,f,fp,alias,site);')
        self.assertEqual(self.check().returncode, 1)


if __name__ == '__main__':
    unittest.main()
