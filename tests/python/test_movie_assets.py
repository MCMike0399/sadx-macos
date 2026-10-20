import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import shutil

spec = importlib.util.spec_from_file_location(
    "movie_assets", Path(__file__).resolve().parents[2] / "tools/import/movie_assets.py")
movies = importlib.util.module_from_spec(spec)
spec.loader.exec_module(movies)


class MovieAssetTests(unittest.TestCase):
    def test_byte_exact_import_and_identity(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            system = root / "game/system"
            system.mkdir(parents=True)
            source = b"encoded media payload: importer copies, runtime validates decoding"
            (system / "re-us.mpg").write_bytes(source)
            (system / "sonic.exe").write_bytes(b"must never copy")
            first = movies.import_movies(root / "game", root / "pack", ["RE-US.MPG"])
            self.assertEqual((root / "pack/movies/RE-US.MPG").read_bytes(), source)
            self.assertEqual(first["clips"][0]["sha256"], first["clips"][0]["source_sha256"])
            self.assertFalse((root / "pack/movies/sonic.exe").exists())
            self.assertEqual(movies.import_movies(root / "game", root / "pack", ["RE-US.MPG"]), first)
            self.assertEqual(json.loads((root / "pack/movie-assets.json").read_text()), first)

    def test_missing_english_intro_does_not_substitute_japanese(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "game/system").mkdir(parents=True)
            (root / "game/system/RE-JP.MPG").write_bytes(b"different language")
            with self.assertRaisesRegex(ValueError, "missing movie asset: RE-US"):
                movies.import_movies(root / "game", root / "pack", ["RE-US.MPG"])
            self.assertFalse((root / "pack").exists())

    def test_unknown_resource_and_duplicate_case_fail(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            system = root / "game/system"
            system.mkdir(parents=True)
            with self.assertRaisesRegex(ValueError, "not in the pinned English profile"):
                movies.import_movies(root / "game", root / "pack", ["../sonic.exe"])
            (system / "RE-US.MPG").write_bytes(b"one")
            (system / "re-us.mpg").write_bytes(b"two")
            # On a case-insensitive filesystem the second name overwrites the first instead.
            if len(list(system.iterdir())) == 2:
                with self.assertRaisesRegex(ValueError, "ambiguous Windows asset"):
                    movies.import_movies(root / "game", root / "pack", ["RE-US.MPG"])

    def pinned_game(self):
        game = Path.cwd() / "reference/updated-pc-reference/game"
        if not (game / "sonic.exe").exists() or not (game / "sonicDX.ini").exists():
            self.skipTest("pinned import source is not installed; native runtime needs only imported assets")
        return game

    def test_pinned_source_content_inverse_and_raw_configuration(self):
        game = self.pinned_game()
        content, evidence = movies.movie_content(game / "sonic.exe", game / "sonicDX.ini")
        self.assertEqual(len(content["clips"]), 9)
        self.assertEqual(content["playlists"], [[index, -1] for index in range(9)])
        self.assertEqual([clip["mode"] for clip in content["clips"]], [1, 2, 1, 1, 1, 0, 1, 1, 1])
        self.assertEqual([clip["fade_frame"] for clip in content["clips"]],
                         [2147483647, 5920, 786, 1750, 1140, 870, 1350, 630, 1110])
        self.assertEqual([clip["asset_path"] for clip in content["clips"]],
                         ["movies/" + name for name in movies.MOVIES])
        self.assertTrue(all((clip["width"], clip["height"]) == (640, 480) for clip in content["clips"]))
        self.assertTrue(all(evidence[name] for name in evidence if name.startswith("inverse_")))
        self.assertEqual(content["source_sha256"], movies.SOURCE_SHA)
        self.assertNotIn("0x", json.dumps(content))
        self.assertEqual(set(content["configuration"]), {"bgm_enabled", "bgm_volume_percent"})
        self.assertNotIn("music_volume", content["configuration"])

    def test_wrong_original_source_version_rejected(self):
        game = self.pinned_game()
        before = movies.digest(game / "sonic.exe")
        with tempfile.TemporaryDirectory() as temporary:
            changed = Path(temporary) / "changed.exe"
            shutil.copyfile(game / "sonic.exe", changed)
            with changed.open("ab") as output:
                output.write(b"unsupported source identity")
            with self.assertRaisesRegex(ValueError, "pinned original PC executable"):
                movies.movie_content(changed, game / "sonicDX.ini")
        self.assertEqual(movies.digest(game / "sonic.exe"), before)

    def test_missing_configuration_is_not_guessed(self):
        game = self.pinned_game()
        with tempfile.TemporaryDirectory() as temporary:
            settings = Path(temporary) / "sonicDX.ini"
            settings.write_text("[sonicDX]\nbgm=1\n")
            with self.assertRaisesRegex(ValueError, "missing movie setting: bgmv"):
                movies.movie_content(game / "sonic.exe", settings)


if __name__ == "__main__":
    unittest.main()
