"""Closure must inspect unreferenced members, not just the current app imports."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/gate'))
from check_native_closure import problems_for, symbols


class NativeClosureTests(unittest.TestCase):
    def test_project_references_resolve_across_members_and_platform(self):
        archive = 'menu.o:\n__ZN4sadx4menuEv T 0 0\n__ZN4sadx5clockEv U 0 0\n_malloc U 0 0\n'
        platform = '__ZN4sadx5clockEv T 0 0\n_mach_absolute_time U 0 0\n'
        self.assertEqual(problems_for([archive, platform], ['src/game/menu.cpp']), [])

    def test_unreferenced_ground_acceleration_member_still_fails(self):
        # Ordinary static linking ignores this member when no caller pulls it in.
        ground = '__ZN4sadx30get_player_acceleration_groundEv'
        archive = 'used.o:\n__ZN4sadx4menuEv T 0 0\nunused.o:\n' + ground + ' U 0 0\n'
        errors = problems_for([archive], ['src/player/sonic_player.cpp'])
        self.assertEqual(errors, ['unresolved project symbol: ' + ground])

    def test_forbidden_sources_and_unicorn_even_when_resolved(self):
        for source in ('src/guest/player_acceleration_ground.cpp', 'src/hle/runtime.cpp',
                       'src/loader/pe.cpp', 'src/game/../guest.cpp', '/src/game/menu.cpp'):
            with self.subTest(source=source):
                self.assertTrue(problems_for([], [source]))
        self.assertTrue(problems_for(['_uc_open T 0 0\n'], ['src/game/menu.cpp']))
        self.assertEqual(problems_for([], ['src/math/d3dx_math.cpp']), [])

    def test_archive_headers_are_not_definitions(self):
        self.assertEqual(symbols('archive.a(member.o):\n_name U 0 0\n_other S 10 0\n'),
                         ({'_other'}, {'_name'}))


if __name__ == '__main__':
    unittest.main()
