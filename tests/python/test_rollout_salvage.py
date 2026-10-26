"""Focused synthetic transcripts using the inspected response_item/tool wrapper format."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('extract', Path(__file__).resolve().parents[2] / 'tools/agents/extract_rollout_files.py')
extract = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(extract)
PREFIX = '/private/tmp/sadx-example-stage'


def custom(code):
    return {'type': 'response_item', 'payload': {'type': 'custom_tool_call',
            'name': 'exec', 'input': code, 'id': 'ctc_fixture', 'call_id': 'call_fixture'}}


def command(cmd, **kwargs):
    return custom('text(await tools.exec_command(' + json.dumps({'cmd': cmd, **kwargs}) + '));\n')


def heredoc(path, text, append=False):
    return f"cat {'>>' if append else '>'} '{path}' <<'EOF'\n{text}EOF\n"


class ExtractTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.session = self.root / 'rollout.jsonl'

    def run_records(self, records):
        self.session.write_text(''.join(json.dumps(r) + '\n' for r in records))
        return extract.extract(self.session, PREFIX, self.root)

    def output(self, path):
        return self.root / 'build/salvage/example-stage' / path

    def test_ordered_overwrite_append_and_exact_patch(self):
        patch = f'*** Begin Patch\n*** Update File: {PREFIX}/a b.txt\n@@\n-two\n+three\n*** End Patch\n'
        normal = {'type': 'response_item', 'payload': {'type': 'function_call',
                  'name': 'functions.exec_command', 'arguments': json.dumps({
                      'cmd': heredoc(PREFIX + '/a b.txt', 'two\n')})}}
        manifest = self.run_records([
            command(heredoc(PREFIX + '/a b.txt', 'old\n')), normal,
            custom('text(await tools.apply_patch(' + json.dumps(patch) + '));'),
            command(heredoc(PREFIX + '/a b.txt', 'four\n', True)),
            command('ls ' + PREFIX + '\nrg hello ' + PREFIX + '/a'),
        ])
        self.assertEqual(manifest['verdict'], 'complete')
        self.assertEqual(self.output('files/a b.txt').read_text(), 'three\nfour\n')
        self.assertEqual(manifest['source_sha256'], hashlib.sha256(self.session.read_bytes()).hexdigest())
        self.assertEqual(manifest['files'][0]['sha256'], hashlib.sha256(b'three\nfour\n').hexdigest())
        self.assertEqual(manifest['operations'][0]['record'], 1)
        self.assertEqual(manifest['operations'][0]['call_id'], 'call_fixture')

    def test_actual_unquoted_cmd_key_and_literal_braces(self):
        cmd = heredoc(PREFIX + '/a', '{foo: value}\n$(touch /tmp/never)\n')
        manifest = self.run_records([custom('text(await tools.exec_command({cmd:' + json.dumps(cmd) + ',max_output_tokens:1000}));')])
        self.assertEqual(manifest['verdict'], 'complete')
        self.assertEqual(self.output('files/a').read_text(), '{foo: value}\n$(touch /tmp/never)\n')

    def test_no_shell_python_or_js_execution(self):
        sentinel = self.root / 'must-not-exist'
        records = [
            command(f'python3 - <<\'PY\'\nfrom pathlib import Path\nPath("{PREFIX}/a").write_text("x")\nPath("{sentinel}").touch()\nPY\n'),
            command(f'touch {sentinel}; echo x > {PREFIX}/a'),
            custom(f'text(await tools.exec_command({{cmd: "touch {sentinel}"}})); evil("{PREFIX}");'),
            command(f'cat > {PREFIX}/a <<EOF\n$(touch {sentinel})\nEOF\n'),
        ]
        manifest = self.run_records(records)
        self.assertEqual(manifest['verdict'], 'incomplete')
        self.assertEqual(len(manifest['unsupported']), 4)
        self.assertFalse(sentinel.exists())
        self.assertEqual(manifest['files'], [])
        for item in manifest['unsupported']:
            self.assertTrue(self.output(item['evidence']).is_file())

    def test_incomplete_and_append_without_base(self):
        result = self.run_records([command(heredoc(PREFIX + '/a', 'x\n', True)),
                                   command(f"cat > {PREFIX}/b <<'EOF'\nnever closed\n")])
        self.assertEqual(len(result['unsupported']), 2)
        self.assertEqual(result['files'], [])

    def test_traversal_and_outside_patch_transaction(self):
        patch = f'*** Begin Patch\n*** Add File: {PREFIX}/valid\n+x\n*** Add File: /outside\n+y\n*** End Patch\n'
        result = self.run_records([command(heredoc(PREFIX + '/../escape', 'x\n')),
                                   custom('text(await tools.apply_patch(' + json.dumps(patch) + '));'),
                                   command(heredoc('/outside', 'unrelated\n'))])
        self.assertEqual(result['files'], [])
        self.assertEqual(len(result['unsupported']), 2)
        self.assertFalse(self.output('escape').exists())

    def test_patch_missing_and_ambiguous_context(self):
        patch = f'*** Begin Patch\n*** Update File: {PREFIX}/a\n@@\n-x\n+y\n*** End Patch\n'
        result = self.run_records([command(heredoc(PREFIX + '/a', 'x\nx\n')),
                                   custom('text(await tools.apply_patch(' + json.dumps(patch) + '));')])
        self.assertEqual(result['verdict'], 'incomplete')
        self.assertEqual(self.output('files/a').read_text(), 'x\nx\n')

    def test_copy_is_prerequisite_and_read_only_is_not(self):
        result = self.run_records([command(f'cp src/changed.cpp {PREFIX}/base.cpp\ncat {PREFIX}/base.cpp\nls {PREFIX}\n')])
        self.assertEqual(len(result['unsupported']), 1)
        self.assertIn('provenance', result['unsupported'][0]['reason'])
        self.assertEqual(result['files'], [])

    def test_workdir_and_cd_do_not_hide_relative_edit(self):
        result = self.run_records([command(heredoc('a', 'x\n'), workdir=PREFIX),
                                   command(f'cd {PREFIX}\nsed -i s/x/y/ a\n')])
        self.assertEqual(self.output('files/a').read_text(), 'x\n')
        self.assertEqual(len(result['unsupported']), 2)

    def test_stale_output_is_refused(self):
        self.run_records([])
        with self.assertRaises(FileExistsError):
            extract.extract(self.session, PREFIX, self.root)

    def test_symlink_output_ancestor_is_refused(self):
        elsewhere = self.root / 'elsewhere'
        elsewhere.mkdir()
        (self.root / 'build').symlink_to(elsewhere, target_is_directory=True)
        with self.assertRaises(OSError):
            self.run_records([command(heredoc(PREFIX + '/a', 'x\n'))])
        self.assertEqual(list(elsewhere.iterdir()), [])

    def test_malformed_records_incomplete(self):
        self.session.write_text('{broken\n')
        result = extract.extract(self.session, PREFIX, self.root)
        self.assertEqual(result['verdict'], 'incomplete')

    def test_overlapping_patch_context_is_ambiguous(self):
        patch = f'*** Begin Patch\n*** Update File: {PREFIX}/a\n@@\n-x\n-x\n+y\n*** End Patch\n'
        result = self.run_records([command(heredoc(PREFIX + '/a', 'x\nx\nx\n')),
                                   custom('text(await tools.apply_patch(' + json.dumps(patch) + '));')])
        self.assertEqual(result['verdict'], 'incomplete')
        self.assertEqual(self.output('files/a').read_text(), 'x\nx\nx\n')

    def test_dynamic_destination_is_not_silently_ignored(self):
        result = self.run_records([command('echo x > "$DEST/file"'),
                                   command("python3 - <<'PY'\np.write_text('changed')\nPY\n")])
        self.assertEqual(len(result['unsupported']), 2)

    def test_read_only_python_is_unrelated(self):
        result = self.run_records([command("python3 - <<'PY'\nprint('inspection only')\nPY\n")])
        self.assertEqual(result['unsupported'], [])

    def test_patch_missing_base_and_context(self):
        patch = f'*** Begin Patch\n*** Update File: {PREFIX}/a\n@@\n-missing\n+new\n*** End Patch\n'
        result = self.run_records([custom('text(await tools.apply_patch(' + json.dumps(patch) + '));'),
                                   command(heredoc(PREFIX + '/a', 'different\n')),
                                   custom('text(await tools.apply_patch(' + json.dumps(patch) + '));')])
        self.assertEqual(len(result['unsupported']), 2)
        self.assertEqual(self.output('files/a').read_text(), 'different\n')

    def test_normal_function_exec_and_patch_add_delete(self):
        add = f'*** Begin Patch\n*** Add File: {PREFIX}/a\n+hello\n*** End Patch\n'
        delete = f'*** Begin Patch\n*** Delete File: {PREFIX}/a\n*** End Patch\n'
        record = {'type': 'response_item', 'payload': {'type': 'function_call',
                  'name': 'functions.exec', 'arguments': json.dumps({'code':
                      'text(await tools.apply_patch(' + json.dumps(add) + '));'})}}
        result = self.run_records([record, {'type': 'response_item', 'payload': {
            'type': 'custom_tool_call', 'name': 'apply_patch', 'input': delete}}])
        self.assertEqual(result['verdict'], 'complete')
        self.assertEqual(result['files'], [])

    def test_shell_control_and_glob_cannot_claim_complete(self):
        result = self.run_records([
            command('if false; then\n' + heredoc(PREFIX + '/a', 'conditional\n') + 'fi\n'),
            command(heredoc(PREFIX + '/*.txt', 'glob\n'))])
        self.assertEqual(result['verdict'], 'incomplete')
        self.assertEqual(len(result['unsupported']), 3)
        self.assertFalse(self.output('files/*.txt').exists())

    def test_dynamic_js_and_lookalike_prefix(self):
        result = self.run_records([custom('const x = "' + PREFIX + '"; text(await tools.exec_command({cmd: x}));'),
                                   command(heredoc(PREFIX + '-other/a', 'x\n'))])
        self.assertEqual(len(result['unsupported']), 2)
        self.assertEqual(result['files'], [])


if __name__ == '__main__':
    unittest.main()
