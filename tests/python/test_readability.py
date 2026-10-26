"""Readability fixtures plus CLI mutants against an actual committed baseline."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

GATE = Path(__file__).resolve().parents[2] / "tools/gate"
sys.path.insert(0, str(GATE))
from check_readability import BASELINE, LEGACY_METRICS, METRICS, measure, snapshot, validate


class ReadabilityTests(unittest.TestCase):
    def test_constructs(self):
        counts = measure("int word_at_abcdef = 0x401000; call(0x40c090, 0Xff);\n"
                         "L3bc: goto L3bc;\nLAB_00401000: work();\n")
        self.assertEqual(counts, dict(zip(METRICS, (3, 1, 1, 1, 2, 0, 1, 0, 0, 0))))

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
        counts = measure(source)
        self.assertEqual(counts.pop("address_comments"), 1)
        self.assertFalse(any(counts.values()))

    def test_multiline_parentheses_and_long_comment(self):
        counts = measure("for (int i = 0;\ni < 2;\n++i) { tick(); }\n//" + "x" * 119)
        self.assertEqual(counts["multi_statement_lines"], 0)
        self.assertEqual(counts["long_lines"], 1)
        self.assertEqual(measure("a(); b(); c();")["multi_statement_lines"], 1)

    def test_escaped_comments_and_multiline_raw_strings(self):
        counts = measure('// continued \\\n0x401000 goto L3bc;\n')
        self.assertEqual(counts.pop("address_comments"), 1)
        self.assertFalse(any(counts.values()))
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
            validate({"schema": 3, "files": {}})

    def test_decimal_boundaries_and_complete_numeric_tokens(self):
        source = """auto a = 65535U;
auto b = 65536;
auto c = 65'536ULL;
auto d = -4294967295L;
auto e = 65536uz;
auto f = 65536.0f;
auto g = 1e65536;
auto h = 0x10000;
auto i = 0200000;
auto j = 0b10000000000000000;
auto k = 65536_pixels;
int value65536 = 0;
auto l = .65536;
auto m = 65536e-2;
auto n = 0x1p+65536;
auto o = 0x10000_tag;
"""
        counts = measure(source)
        self.assertEqual(counts["large_decimal_literals"], 4)
        self.assertEqual(counts["hex_literals"], 2)

    def test_named_constexpr_only_exempts_literal_debt(self):
        source = """static constexpr std::uint32_t
kOpaqueWhite =
    0xFFFFFFFFU;
inline constexpr std::array<unsigned, 2> kColors{
    16777215U, 0xFFFFFFFFU
};
constexpr unsigned kValues[] = {65536, 0x10000};
constexpr unsigned kMask(
    0xFFFFFFFFU);
constexpr unsigned kSize(65536);
constexpr unsigned color() { return 0x10000; }
constexpr auto large() { return 65536; }
constexpr unsigned parameter(unsigned value = 65536);
void step() {
    if constexpr (true) { use(65536); }
}
const auto unnamed = 65536;
auto after = 0x10000;
"""
        counts = measure(source)
        self.assertEqual(counts["large_decimal_literals"], 4)
        self.assertEqual(counts["hex_literals"], 2)
        # Naming a value never exempts unrelated metrics in its initializer.
        self.assertEqual(measure("constexpr auto kValue = fp.to_float(x);")
                         ["emulator_arithmetic"], 1)

    def test_emulator_calls_renamed_contexts_and_overlapping_matches(self):
        source = """void run(X87Context &arithmetic) {
    arithmetic.add(x, y);
    arithmetic -> multiply_float_memory(x, y);
    fp . from_float(x);
    fp.multiply_float_memory(x, y);
    other.multiply_float_memory(x, y);
    multiply_float_memory(x, y);
    fingerprint.update();
    fp32.update();
    auto method = fp.from_float;
}
"""
        self.assertEqual(measure(source)["emulator_arithmetic"], 7)

    def test_address_comment_lines_and_annotation_boundaries(self):
        source = """// FUNCTION: SONIC 0x401000 Tick
// GLOBAL: SONIC 0x3abdc7c Mode size=4
// STUB: SONIC 0x402000 Next awaiting recovery
// DATA: SONIC 0x403000 Table
// LIBRARY: SONIC 0x404000 Helper
// PATCH: SPEEDRUN +0x10f0 -> 0x7899e0
work(); // 0x79500e then 0x795011
/* 0x795014
   0x795017 */ /* 0x79501a */
work(); // FUNCTION: SONIC 0x405000 FakeInline
// FUNCTION: arbitrary prose 0x406000
// ordinary note at LAB_00401000
const char* text = "// 0x407000";
auto raw = R"tag(// 0x408000)tag";
// small mask 0xff
"""
        self.assertEqual(measure(source)["address_comments"], 6)
        self.assertEqual(measure('// FUNCTION: SONIC 0x401000 Tick \\\n// 0x402000\n')
                         ["address_comments"], 1)
        self.assertEqual(measure('// FUNCTION: SONIC 0x401000 0x402000\n')
                         ["address_comments"], 1)

    def test_new_metrics_ignore_comments_and_strings(self):
        source = r'''// 65536 fp.add() X87Context multiply_float_memory()
/* 65536 fp.add() X87Context */
auto text = "65536 fp.add() X87Context // 0x401000";
auto raw = u8R"tag(65536 fp.add() X87Context // 0x401000)tag";
'''
        for metric in ("large_decimal_literals", "emulator_arithmetic", "address_comments"):
            self.assertEqual(measure(source)[metric], 0)

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

    def test_new_metric_mutants_cannot_be_blessed(self):
        mutants = {
            "large_decimal_literals": "use(16777215U);\n",
            "emulator_arithmetic": "fp.multiply_float_memory(x, y);\n",
            "address_comments": "work(); // 0x79500e\n",
        }
        for metric, mutation in mutants.items():
            with self.subTest(metric=metric), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                source = root / "src/engine/system.cpp"
                source.parent.mkdir(parents=True)
                source.write_text("work();\n")
                self.git(root, "init", "-q")
                self.assertEqual(self.check(root, "--write-baseline").returncode, 0)
                self.commit(root)
                source.write_text(mutation)
                result = self.check(root)
                self.assertEqual(result.returncode, 1, result.stdout)
                self.assertIn(f"{metric} 1 > 0", result.stdout)
                self.assertEqual(self.check(root, "--write-baseline").returncode, 1)

                baseline = root / BASELINE
                data = json.loads(baseline.read_text())
                data["files"]["src/engine/system.cpp"][metric] = 1
                baseline.write_text(json.dumps(data))
                self.assertIn("baseline increase against HEAD", self.check(root).stdout)
                # Even removing the source regression cannot bless a raised allowance.
                source.write_text("work();\n")
                self.assertEqual(self.check(root, "--write-baseline").returncode, 1)
                self.git(root, "restore", BASELINE)
                source.with_name("new.cpp").write_text(mutation)
                self.assertEqual(self.check(root).returncode, 1)
                self.assertEqual(self.check(root, "--write-baseline").returncode, 1)

    def test_removing_constexpr_exemption_is_a_cli_regression(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/engine/colors.cpp"
            source.parent.mkdir(parents=True)
            original = "constexpr unsigned kColor =\n    0xFFFFFFFFU;\nconstexpr auto kSize = 65536;\n"
            source.write_text(original)
            self.git(root, "init", "-q")
            self.assertEqual(self.check(root, "--write-baseline").returncode, 0)
            self.commit(root)
            source.write_text(original.replace("constexpr", "const"))
            result = self.check(root)
            self.assertEqual(result.returncode, 1)
            self.assertIn("hex_literals 1 > 0", result.stdout)
            self.assertIn("large_decimal_literals 1 > 0", result.stdout)

    @staticmethod
    def git(root, *args):
        return subprocess.run(["git", *args], cwd=root, check=True,
                              capture_output=True, text=True)

    def commit(self, root):
        self.git(root, "add", ".")
        self.git(root, "-c", "user.name=Readability Fixture",
                 "-c", "user.email=fixture@example.invalid", "commit", "-qm", "fixture")

    @staticmethod
    def check(root, *args):
        return subprocess.run([sys.executable, str(GATE / "check_readability.py"),
                               "--root", str(root), *args], capture_output=True, text=True)

    def test_legacy_migration_seeds_head_not_working_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/engine/system.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("use(65536);\nfp.add(x,y);\nwork(); // 0x79500e\n")
            baseline = root / BASELINE
            baseline.parent.mkdir()
            baseline.write_text(json.dumps({"schema": 1, "files": {
                "src/engine/system.cpp": dict.fromkeys(LEGACY_METRICS, 0)}}))
            self.git(root, "init", "-q")
            self.commit(root)
            original = source.read_text()
            for mutation in ("use(65536);\n", "fp.add(x,y);\n", "work(); // 0x795011\n"):
                source.write_text(original + mutation)
                self.assertEqual(self.check(root).returncode, 1)
                self.assertEqual(self.check(root, "--write-baseline").returncode, 1)
            source.write_text(original)
            self.assertEqual(self.check(root, "--write-baseline").returncode, 0)
            data = json.loads(baseline.read_text())
            self.assertEqual(data["schema"], 2)
            for metric in ("large_decimal_literals", "emulator_arithmetic", "address_comments"):
                self.assertEqual(data["files"]["src/engine/system.cpp"][metric], 1)
            # A forged schema-2 migration allowance is still compared with HEAD source.
            data["files"]["src/engine/system.cpp"]["emulator_arithmetic"] = 2
            baseline.write_text(json.dumps(data))
            self.assertIn("baseline increase against HEAD", self.check(root).stdout)
            data["files"]["src/engine/system.cpp"]["emulator_arithmetic"] = 1
            baseline.write_text(json.dumps(data))
            self.commit(root)
            source.write_text("work();\n")
            self.assertEqual(self.check(root, "--write-baseline").returncode, 0)
            self.commit(root)
            source.write_text(original)
            self.assertEqual(self.check(root).returncode, 1)
            self.assertEqual(self.check(root, "--write-baseline").returncode, 1)

    def test_source_archive_enforces_supplied_baseline(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/engine/system.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("work();\n")
            self.assertEqual(self.check(root, "--write-baseline").returncode, 0)
            for mutation in ("use(65536);\n", "fp.add(x,y);\n", "work(); // 0x795011\n"):
                source.write_text(mutation)
                self.assertEqual(self.check(root).returncode, 1)
                self.assertEqual(self.check(root, "--write-baseline").returncode, 1)
            # No source history means a schema-1 archive cannot safely seed allowances.
            baseline = root / BASELINE
            baseline.write_text(json.dumps({"schema": 1, "files": {
                "src/engine/system.cpp": dict.fromkeys(LEGACY_METRICS, 0)}}))
            self.assertIn("migration requires Git HEAD", self.check(root).stdout)

    def test_migration_of_tracked_sources_missing_from_legacy_baseline(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/engine/unlisted.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("use(65536);\n")
            baseline = root / BASELINE
            baseline.parent.mkdir()
            baseline.write_text(json.dumps({"schema": 1, "files": {}}))
            self.git(root, "init", "-q")
            self.commit(root)
            self.assertEqual(self.check(root).returncode, 0)
            # HEAD seeds only the new metrics; old unlisted allowances stay zero.
            source.write_text("use(0x10000);\n")
            self.assertIn("hex_literals 1 > 0", self.check(root).stdout)
            source.write_text("use(65536);\nuse(65536);\n")
            self.assertIn("large_decimal_literals 2 > 1", self.check(root).stdout)
            source.write_text("use(65536);\n")
            extra = source.with_name("new.cpp")
            extra.write_text("use(65536);\n")
            self.assertIn("large_decimal_literals 1 > 0", self.check(root).stdout)
            extra.unlink()
            self.assertEqual(self.check(root, "--write-baseline").returncode, 0)



if __name__ == "__main__":
    unittest.main()
