"""Fingerprint and scope regressions for constexpr aggregate review accounting."""
import json
import hashlib
import copy
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

GATE = Path(__file__).resolve().parents[2] / "tools/gate"
sys.path.insert(0, str(GATE))
from check_constexpr_tables import census, declarations, reference_problems, review_problems


def registry(current):
    return {"schema": 1, "tables": {
        key: {"sha256": row["sha256"], "classification": "open_review",
              "purpose": "Fixture awaiting semantic review",
              "evidence": [{"path": key.split("::")[0], "line": row["line"],
                            "sha256": row["sha256"]}],
              "verification": [key.split("::")[0]]}
        for key, row in current.items()}, "retired": {}}


class ConstexprTableTests(unittest.TestCase):
    def test_arrays_custom_aggregates_and_scalar_colors(self):
        source = """constexpr unsigned kMask = 0xFFFFFFFFU;
constexpr std::uint32_t kWhite{0xFFFFFFFFU};
constexpr unsigned long kLongMask{0xFFFFFFFFUL};
constexpr std::array<unsigned, 2> kColors{0xFFFFFFU, 0xFFFFFFFFU};
constexpr unsigned operations[] = {0x49a9b0, 0x49aa00};
constexpr auto inferred = std::array{1, 2};
constexpr Matrix identity{{1.0f, 0.0f}};
constexpr unsigned function(unsigned value = 1);
"""
        rows = declarations(source)
        self.assertEqual([row["declaration"] for row in rows],
                         ["kColors", "operations", "inferred", "identity"])
        self.assertEqual([row["raw_hex_tokens"] for row in rows], [2, 2, 0, 0])

    def test_multiline_nested_and_masked_delimiters(self):
        source = '''// constexpr int fake[] = {0x123456};
constexpr std::array<const char *, 2>
names{{R"tag(}; constexpr int fake[] = {0xabcdef};)tag", "{;}}"}};
/* constexpr int more[] = {0x234567}; */
constexpr int real[] = {1, 2};
'''
        rows = declarations(source)
        self.assertEqual([row["declaration"] for row in rows], ["names", "real"])
        self.assertEqual(rows[0]["line"], 2)
        self.assertEqual(rows[0]["raw_hex_tokens"], 0)

    def test_fingerprint_preserves_string_values_and_changed_table(self):
        before = declarations('constexpr const char* names[] = {"Sonic"};')[0]
        after = declarations('constexpr const char* names[] = {"Gamma"};')[0]
        self.assertNotEqual(before["sha256"], after["sha256"])
        moved = declarations('\n\nconstexpr const char* names[] = {"Sonic"};')[0]
        self.assertEqual(before["sha256"], moved["sha256"])

    def test_parenthesized_braced_arrays_and_custom_values_are_reviewed(self):
        rows = declarations('constexpr std::array<const char*,2> names({"Sonic","Gamma"});\n'
                            'constexpr Matrix identity(Matrix{{1.0f,0.0f}});')
        self.assertEqual([row['declaration'] for row in rows], ['names', 'identity'])
        changed = declarations('constexpr std::array<const char*,2> names({"Sonic","Amy"});')[0]
        self.assertNotEqual(rows[0]['sha256'], changed['sha256'])
        self.assertEqual(declarations('constexpr std::array<int,2> function(Type value);'), [])
        with self.assertRaises(ValueError):
            declarations('constexpr std::array<int,2> unknown(other);')

    def test_unreviewed_changed_renamed_and_deleted_are_visible(self):
        row = declarations("constexpr int modes[] = {1, 2};")[0]
        key = "src/engine/blend.cpp::modes"
        current = {key: row}
        data = registry(current)
        self.assertEqual(review_problems(current, data), [])
        changed = {key: declarations("constexpr int modes[] = {1, 3};")[0]}
        self.assertIn("changed aggregate requires review", review_problems(changed, data)[0])
        renamed = {"src/engine/blend.cpp::other": row}
        self.assertEqual(len(review_problems(renamed, data)), 2)
        self.assertIn("stale aggregate review", review_problems({}, data)[0])
        self.assertIn("unreviewed aggregate", review_problems(current, registry({}))[0])

    def test_open_instruction_debt_is_not_accepted_numeric_exception(self):
        row = declarations("constexpr Operation operations[] = {{0x49a9b0, 4}};")[0]
        key = "src/player/animation.hpp::operations"
        data = registry({key: row})
        data["tables"][key]["classification"] = "open_migration"
        self.assertEqual(review_problems({key: row}, data), [])
        self.assertEqual(data["tables"][key]["classification"], "open_migration")
        data["tables"][key]["purpose"] = ""
        with self.assertRaises(ValueError):
            review_problems({key: row}, data)

    def test_history_cannot_shrink_or_forget_retired_declarations(self):
        key = "src/engine/blend.cpp::modes"
        old = registry({key: declarations("constexpr int modes[] = {1, 2};")[0]})
        new = registry({})
        self.assertIn("removed review requires preserved lineage",
                      review_problems({}, new, old)[0])
        new["retired"][key] = {"review": copy.deepcopy(old['tables'][key]),
                               "reason": "Replaced by named functions", "evidence": ["diff"],
                               "verification": ['tests/unit/render.cpp'], "outcome": 'migrated',
                               "successor": {'path': 'src/engine/new.cpp', 'symbol': 'functions'}}
        self.assertEqual(review_problems({}, new, old), [])
        self.assertIn("retired review history changed", review_problems({}, registry({}), new)[0])

    def test_cli_new_table_does_not_inherit_exemption(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/player/animation.hpp"
            source.parent.mkdir(parents=True)
            source.write_text("constexpr unsigned colors[] = {0xFFFFFFFF};\n")
            path = root / "reference/constexpr-table-review.json"
            path.parent.mkdir()
            data = registry(census(root))
            data['tables'][next(iter(data['tables']))]['evidence'][0]['sha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
            path.write_text(json.dumps(data))
            command = [sys.executable, str(GATE / "check_constexpr_tables.py"),
                       "--root", str(root)]
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            source.write_text(source.read_text() +
                              "constexpr Operation recipe[] = {{0x49a9b0, 0x49aa00}};\n")
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn("unreviewed aggregate", result.stdout)

    def test_stale_evidence_and_verification_paths_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            current = {"src/engine/modes.hpp::modes":
                       declarations("constexpr int modes[] = {1};")[0]}
            data = registry(current)
            self.assertEqual(len(reference_problems(root, data)), 2)
            source = root / "src/engine/modes.hpp"
            source.parent.mkdir(parents=True)
            source.write_text("constexpr int modes[] = {1};")
            self.assertEqual(reference_problems(root, data), [])
            data["tables"][next(iter(current))]["evidence"][0]["line"] = 100
            self.assertIn("invalid source evidence line", reference_problems(root, data)[0])

    def test_changed_in_range_consumer_invalidates_review(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / 'src/engine/modes.cpp'
            path.parent.mkdir(parents=True)
            path.write_text('constexpr int modes[] = {1};\nconsume_as_data(modes);')
            current = {'src/engine/modes.cpp::modes': declarations(path.read_text())[0]}
            data = registry(current)
            evidence = data['tables'][next(iter(current))]['evidence'][0]
            evidence.update(line=2, sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            self.assertEqual(reference_problems(root, data), [])
            path.write_text('constexpr int modes[] = {1};\ndispatch_recipe(modes);')
            self.assertEqual(review_problems(current, data), [])
            self.assertIn('changed source/consumer evidence', reference_problems(root, data)[0])

    def test_retirement_requires_real_evidence_successor_and_prior_disposition(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            key = 'src/engine/modes.cpp::modes'
            old = registry({key: declarations('constexpr int modes[] = {1};')[0]})
            old['tables'][key]['classification'] = 'open_migration'
            new = registry({})
            new['retired'][key] = {
                'review': copy.deepcopy(old['tables'][key]), 'outcome': 'migrated',
                'reason': 'Replaced by a named behavior',
                'evidence': [{'path': 'review.md', 'line': 1, 'sha256': '0' * 64}],
                'verification': ['review.md'],
                'successor': {'path': 'src/engine/new.cpp', 'symbol': 'blend', 'sha256': '0' * 64}}
            self.assertEqual(review_problems({}, new, old), [])
            self.assertTrue(reference_problems(root, new))
            (root / 'review.md').write_text('Reviewed migration and preserved judge coverage.')
            successor = root / 'src/engine/new.cpp'
            successor.parent.mkdir(parents=True)
            successor.write_text('void blend() {}')
            entry = new['retired'][key]
            entry['evidence'][0]['sha256'] = hashlib.sha256((root / 'review.md').read_bytes()).hexdigest()
            entry['successor']['sha256'] = hashlib.sha256(successor.read_bytes()).hexdigest()
            self.assertEqual(reference_problems(root, new), [])
            entry['review']['classification'] = 'numeric_encoding'
            self.assertIn('removed review requires preserved lineage', review_problems({}, new, old)[0])


if __name__ == "__main__":
    unittest.main()
