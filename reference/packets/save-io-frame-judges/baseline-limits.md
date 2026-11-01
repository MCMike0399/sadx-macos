# Before-source baseline

Base 353d528bbb1a50e8cb0e4cdeaf7657dc9ad8607d is the immutable source anchor.
All src files remain unchanged. source-freeze.json hashes the complete tracked
source/test/oracle/helper closure before judge edits. New observation code wraps
ctypes calls and observes evaluator values without replacing any comparison.
The ten unchanged evaluators ran default cohorts, no exclusions removed.
Raw probe library and deterministic gzip ABI streams remain ignored under
build/save-io-judges. original-baseline.json pins every stream and evaluator.
Source function returns and comparison operands are observed separately from the
complete native argument arrays and returns. No claim of full source memory or
FP equivalence beyond each unchanged evaluator's actual comparisons.

Initial compile named nonexistent bootstrap.cpp (exit 1); corrected name is
game_bootstrap.cpp. First load attempt failed on an unused exported X87 function
for all ten scripts. Probe-only export/dead stripping fixed the library without
source changes. First load-selected rerun omitted --input-dll and failed before
cases; the final unchanged run uses the pinned local DLL. Initial failures remain
in build/save-io-judges logs/initial-command-failures.json. No semantic source
failure was suppressed. Narrow compiler invocations only, no archive or gate.

All 53 inherited raw corpora are absent in this isolated checkout. Their exact
paths and expected hashes remain in source-freeze.json; the inherited judge
manifest is preserved. No incomplete full refresh is authorized by this receipt.
