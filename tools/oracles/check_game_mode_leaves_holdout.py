#!/usr/bin/env python3
"""Run an additional RNG cohort through the unchanged game-mode leaf evaluator.

The stock judge keeps its fixed default seed and comparisons. This qualification
adapter changes only the generator's RNG seed; it never changes vectors after
generation, original execution, native outputs or comparison rules.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import sys
from types import SimpleNamespace

import verify_original_game_mode_leaves as original


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('system', type=Path)
    parser.add_argument('native_library', type=Path)
    parser.add_argument('--seed', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()

    def cohort_random(stock_seed):
        if stock_seed != 0x40c090:
            raise RuntimeError('stock generator seed changed; review this qualification adapter')
        return random.Random(args.seed)

    previous_random, previous_argv = original.random, sys.argv
    original.random = SimpleNamespace(Random=cohort_random)
    sys.argv = [original.__file__, str(args.exe), str(args.system),
                str(args.native_library), '--output', str(args.output)]
    try:
        result = original.main()
    finally:
        original.random, sys.argv = previous_random, previous_argv

    report = json.loads(args.output.read_text())
    report['additional_cohort'] = {
        'seed': args.seed,
        'stock_evaluator_sha256': hashlib.sha256(Path(original.__file__).read_bytes()).hexdigest(),
        'adapter_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'comparison_changes': 'none',
        'limits': 'Stock native boundary exclusions remain explicit. Original FP flags are not compared.'}
    args.output.write_text(json.dumps(report, indent=1) + '\n')
    return result


if __name__ == '__main__':
    raise SystemExit(main())
