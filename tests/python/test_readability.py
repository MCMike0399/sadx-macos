"""Readability fixtures plus CLI mutants against an actual committed baseline."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

GATE = Path(__file__).resolve().parents[2] / "tools/gate"
sys.path.insert(0, str(GATE))
from check_readability import BASELINE, METRICS, measure, snapshot, validate


class ReadabilityTests(unittest.TestCase):
    def test_constructs(self):
        counts = measure("int word_at_abcdef = 0x401000; call(0x40c090, 0Xff);\n"
                         "L3bc: goto L3bc;\nLAB_00401000: work();\n")
        self.assertEqual(counts, dict(zip(METRICS, (3, 1, 1, 1, 2, 0, 1))))

    def test_comments_strings_and_for_headers(self):
        source = r'''// FUNCTION: SONIC 0x401000 Tick
/* 0x402000; goto LAB_00401000; */
const char* text = "0x403000; goto L3bc;";
const char* raw = R"tag(0x404000; "goto L3bc;")tag";
char quote = '\'';
for (int i = 0; i < 3; ++i) {
    tick();
}
'''
        self.assertFalse(any(measure(source).values()))

    def test_multiline_parentheses_and_long_comment(self):
        counts = measure("for (int i = 0;\ni < 2;\n++i) { tick(); }\n//" + "x" * 119)
        self.assertEqual(counts["multi_statement_lines"], 0)
        self.assertEqual(counts["long_lines"], 1)
        self.assertEqual(measure("a(); b(); c();")["multi_statement_lines"], 1)

    def test_escaped_comments_and_multiline_raw_strings(self):
        self.assertFalse(any(measure('// continued \\\n0x401000 goto L3bc;\n').values()))
        self.assertFalse(any(measure('auto text = R"xyz(\n0x401000; goto L3bc;\n)xyz";').values()))
        self.assertEqual(measure("float value = constant(0x401000);")["address_calls"], 0)

    def test_literals_and_token_boundaries(self):
        counts = measure("auto a = 0xff'ffU;\nauto b = 0x1.2p-4f;\n"
                         "auto c = 0X12;\nint hexadecimal0x123 = 0;\n")
        self.assertEqual(counts["hex_literals"], 3)
        self.assertEqual(measure("auto a = 0x12'34;\nauto b = 0x56'78;\n")["hex_literals"], 2)

    def test_scope_and_schema(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("engine/deep/system.cpp", "cpu/test.hpp", "hle/test.cpp",
                         "guest/test.cpp", "platform/mac/test.mm", "loader/test.cpp"):
                path = root / "src" / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("auto value = 0x401000;\n")
            self.assertEqual(set(snapshot(root)), {"src/engine/deep/system.cpp", "src/cpu/test.hpp"})
        with self.assertRaises(ValueError):
            validate({"schema": 1, "files": {"src/engine/a.cpp": {}}})
        with self.assertRaises(ValueError):
            validate({"schema": 2, "files": {}})

    def test_cli_mutants_and_baseline_ratchet(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/engine/clean.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("int value = 1;\n")

            def git(*args):
                return subprocess.run(["git", *args], cwd=root, check=True,
                                      capture_output=True, text=True)

            def check(*args):
                return subprocess.run([sys.executable, str(GATE / "check_readability.py"),
                                       "--root", str(root), *args], capture_output=True, text=True)

            git("init", "-q")
            git("config", "user.name", "Readability Fixture")
            git("config", "user.email", "fixture@example.invalid")
            self.assertEqual(check("--write-baseline").returncode, 0)
            git("add", ".")
            git("commit", "-qm", "fixture baseline")
            self.assertEqual(check().returncode, 0)
            # Required mutant: introducing a literal in a clean file fails.
            source.write_text("int value = 0x401000;\n")
            result = check()
            self.assertEqual(result.returncode, 1)
            self.assertIn("hex_literals 1 > 0", result.stdout)
            self.assertEqual(check("--write-baseline").returncode, 1)
            # Editing the working baseline cannot bless an increase.
            baseline = root / BASELINE
            data = json.loads(baseline.read_text())
            data["files"]["src/engine/clean.cpp"]["hex_literals"] = 1
            baseline.write_text(json.dumps(data))
            self.assertIn("baseline increase against HEAD", check().stdout)
            source.write_text("int value = 1;\n")
            self.assertEqual(check().returncode, 1)
            git("restore", BASELINE)
            extra = source.with_name("new.cpp")
            extra.write_text("auto value = 0x123;\n")
            self.assertEqual(check().returncode, 1)
            extra.write_text("int value = 2;\n")
            self.assertEqual(check().returncode, 0)


if __name__ == "__main__":
    unittest.main()
