import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest
import wave

spec = importlib.util.spec_from_file_location(
    "import_audio", Path(__file__).resolve().parents[2] / "tools/lib/import_audio.py")
audio = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audio)


def pcm():
    buffer = io.BytesIO()
    with wave.open(buffer, "wb") as output:
        output.setparams((1, 2, 22050, 0, "NONE", ""))
        output.writeframes(struct.pack("<hhh", 0, 1200, -1200))
    return buffer.getvalue()


def archive(name=b"B00_00_00.WAV", payload=None):
    payload = pcm() if payload is None else payload
    return (b"archive  V2.2\0\0\0" + struct.pack("<I", 1) +
            struct.pack("<III", 32, 33 + len(name), len(payload)) +
            name + b"\0" + payload)


class ImportAudioTests(unittest.TestCase):
    def test_exact_effect_and_source_identity(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "source").mkdir()
            (root / "source/bank.dat").write_bytes(archive())
            records = audio.import_audio(root / "source", root / "output", ["bank.dat"])
            self.assertEqual((root / "output/bank/b00_00_00.wav").read_bytes(), pcm())
            self.assertEqual(records[0]["entry"], 0)
            self.assertEqual(records[0]["frames"], 3)
            self.assertEqual(len(records[0]["source_sha256"]), 64)
            self.assertEqual(audio.import_audio(root / "source", root / "output", ["bank.dat"]), records)

    def test_bad_archive_never_scans_for_riff(self):
        for bad in (b"RIFF" + bytes(100), archive()[:31], archive(b"../escape.wav"),
                    archive(payload=b"compressed"), archive()[:-1]):
            with self.subTest(bad=bad[:35]), self.assertRaises(ValueError):
                audio.dat_entries(bad)

    def test_wma_is_explicit_dependency(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "sample.wma").write_bytes(b"not-wma")
            with self.assertRaisesRegex(ValueError, "needs --ffmpeg"):
                audio.import_audio(root, root / "out", ["sample.wma"])

    def test_reject_source_escape(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            root.joinpath("source").mkdir()
            with self.assertRaisesRegex(ValueError, "outside sounddata"):
                audio.import_audio(root / "source", root / "out", ["../other.dat"])


if __name__ == "__main__":
    unittest.main()
