"""Generator-domain checks use synthetic development seeds, never reserved seeds."""
import importlib.util
from pathlib import Path
import unittest


PATH = Path(__file__).resolve().parents[2] / 'tools/oracles/task_judges/reserved_inputs.py'
SPEC = importlib.util.spec_from_file_location('task_reserved_inputs', PATH)
INPUTS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(INPUTS)


class TaskReservedInputs(unittest.TestCase):
    def test_arena_domain_and_complete_release(self):
        for operations in INPUTS.arena_sequences(7, 12, 300):
            live, allocation = set(), 0
            for opcode, value in operations:
                if opcode == 0:
                    self.assertGreater(value, 0)
                    live.add(allocation)
                    allocation += 1
                else:
                    self.assertEqual(opcode, 1)
                    self.assertIn(value, live)
                    live.remove(value)
            self.assertFalse(live)

    def test_lifecycle_domain_and_allocation_indices(self):
        child_count = 0
        for operations in INPUTS.lifecycle_sequences(11, 12, 150):
            top_level, allocation = set(), 0
            for opcode, first, second, unused in operations:
                self.assertEqual(unused, 0)
                if opcode == 1:
                    self.assertIn(first, range(16))
                    self.assertIn(second, (0, 1, 2, 3, 4, 5, 6, 8))
                    top_level.add(allocation)
                    allocation += 1
                elif opcode == 2:
                    self.assertIn(first, range(16))
                    self.assertIn(second, top_level)
                    allocation += 1
                    child_count += 1
                else:
                    self.assertEqual(opcode, 3)
                    self.assertEqual(second, 0)
                    self.assertIn(first, top_level)
                    top_level.remove(first)
            self.assertFalse(top_level)
        self.assertGreater(child_count, 0)

    def test_generation_is_reproducible_and_seeded(self):
        for generate in (INPUTS.arena_sequences, INPUTS.lifecycle_sequences):
            self.assertEqual(generate(3, 2, 30), generate(3, 2, 30))
            self.assertNotEqual(generate(3, 2, 30), generate(4, 2, 30))

    def test_invalid_bounds_fail(self):
        for generate in (INPUTS.arena_sequences, INPUTS.lifecycle_sequences):
            for bounds in ((0, 10), (2, 0), (-1, 10), (2, -1)):
                with self.assertRaises(ValueError):
                    generate(5, *bounds)


if __name__ == '__main__':
    unittest.main()
