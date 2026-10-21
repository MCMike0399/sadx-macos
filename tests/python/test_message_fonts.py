"""Source-record inverse and MSG24 width rules; no game asset is copied to the repository."""
import importlib.util
import json
from pathlib import Path
import unittest
from tempfile import TemporaryDirectory

STAGE=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('message_fonts',STAGE/'tools/import/message_fonts.py')
fonts=importlib.util.module_from_spec(spec);spec.loader.exec_module(fonts)

class MessageFonts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source=fonts.REPO/'reference/updated-pc-reference/game'
        cls.temporary=TemporaryDirectory(prefix='sadx-message-font-test-')
        cls.output=Path(cls.temporary.name)
        fonts.import_message_fonts(cls.source,cls.output)
    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()
    def test_inverse_metadata_and_bitmap(self):
        raw=bytes(range(88))*3
        glyphs,metadata=fonts.decode_records(raw)
        self.assertEqual(len(glyphs),3)
        self.assertEqual(b''.join(a+b for a,b in zip(metadata,glyphs)),raw)
        self.assertEqual(glyphs[0],raw[16:88])
    def test_incomplete_record_rejected(self):
        for n in (1,16,72,87,89):
            with self.assertRaises(ValueError):fonts.decode_records(bytes(n))
    def test_blank_width_is_twelve(self):
        self.assertEqual(fonts.generated_width(bytes(72)),12)
    def test_each_column_bit(self):
        for column in range(24):
            bitmap=bytearray(72);bitmap[23*3+column//8]=128>>(column%8)
            self.assertEqual(fonts.generated_width(bitmap),column+1)
    def test_rightmost_across_rows(self):
        bitmap=bytearray(72);bitmap[0]=128;bitmap[4]=1;bitmap[71]=32
        self.assertEqual(fonts.generated_width(bitmap),19)
    def test_actual_source_inverse(self):
        proof=json.loads((self.output/'message-fonts-evidence.json').read_text())
        for entry in proof['fonts']:
            raw=(self.source/'system'/entry['filename']).read_bytes()
            glyphs,metadata=fonts.decode_records(raw)
            self.assertEqual(fonts.digest(raw),entry['sha256'])
            self.assertEqual([x.hex() for x in metadata],entry['record_metadata'])
            self.assertEqual(b''.join(a+b for a,b in zip(metadata,glyphs)),raw)
    def test_payload_matches_source_records(self):
        data=json.loads((self.output/'message-fonts.json').read_text())
        self.assertEqual(data['version'],1)
        for family,filename in [('single','FONTDATA1.BIN'),('wide','FONTDATA0.BIN')]:
            raw=(self.source/'system'/filename).read_bytes()
            glyphs,_=fonts.decode_records(raw);entry=data['fonts'][family]
            self.assertEqual((self.output/entry['glyph_file']).read_bytes(),b''.join(glyphs))
            self.assertEqual(entry['glyph_sha256'],fonts.digest(b''.join(glyphs)))
            self.assertEqual(entry['fallback_bytes'],raw[:72].hex())
    def test_actual_saving_advance(self):
        data=json.loads((self.output/'message-fonts.json').read_text())
        self.assertEqual(data['saving_text'],'Now Saving...')
        self.assertEqual(sum(data['widths'][ord(c)-32]+1 for c in data['saving_text']),128)

if __name__=='__main__':unittest.main()
