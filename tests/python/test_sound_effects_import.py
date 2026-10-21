import importlib.util
from pathlib import Path
import struct
import unittest

spec=importlib.util.spec_from_file_location('sound_effects_import',Path(__file__).resolve().parents[2]/'tools/import/sound_effects.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class Image:
    sha256=module.SOURCE_SHA256
    def __init__(self):self.words=[n for tone in range(63,0,-1) for n in (tone,1)]+[-1,-1]
    def read(self,address,size):
        if address==0x910090 and size==512:return struct.pack('<128i',*self.words)
        raise AssertionError((address,size))
    def unpack(self,fmt,address):
        if address==0x910090:return tuple(self.words)
        if address==0x910290:return (8,)
        raise AssertionError(address)
class SoundEffectImportTests(unittest.TestCase):
    def test_all_records_inverse(self):
        image=Image();tables=module.extract_tables(image)
        self.assertEqual(len(tables['bank_thresholds']),64)
        raw=b''.join(struct.pack('<ii',row['tone'],row['bank']) for row in tables['bank_thresholds'])
        self.assertEqual(raw,image.read(0x910090,512));self.assertEqual(tables['alternating_pan'],8)
    def test_reject_other_source(self):
        image=Image();image.sha256='other'
        with self.assertRaises(ValueError):module.extract_tables(image)
    def test_reject_missing_terminal(self):
        image=Image();image.words[-2:]=[0,1]
        with self.assertRaises(ValueError):module.extract_tables(image)
    def test_reject_wrong_order(self):
        image=Image();image.words[2]=image.words[0]
        with self.assertRaises(ValueError):module.extract_tables(image)
    def test_reject_bad_slot(self):
        image=Image();image.words[1]=16
        with self.assertRaises(ValueError):module.extract_tables(image)
class WaveMetadataTests(unittest.TestCase):
    @staticmethod
    def wave(loops=None,first=0,last=1,before=False,alignment=2):
        def chunk(name,data):return name+struct.pack('<I',len(data))+data+bytes(len(data)&1)
        fmt=chunk(b'fmt ',struct.pack('<HHIIHH',1,1,22050,44100,alignment,16))
        sample=b''
        if loops is not None:
            raw=bytearray(36);struct.pack_into('<I',raw,28,loops&0xffffffff)
            raw+=struct.pack('<6I',0,0,first,last,0,0);sample=chunk(b'smpl',raw)
        payload=b'WAVE'+(sample+fmt if before else fmt+sample)+chunk(b'data',bytes(16))
        return b'RIFF'+struct.pack('<I',len(payload))+payload
    def test_absent_sampler_preserves_constructor_count(self):
        result=module.extract_wave_metadata(self.wave())
        self.assertEqual(result['loop_count'],0)
        self.assertIsNone(result['loop_start']);self.assertIsNone(result['loop_end'])
    def test_loop_offsets_are_source_bytes(self):
        result=module.extract_wave_metadata(self.wave(1,2,7,alignment=4))
        self.assertEqual((result['loop_count'],result['loop_start'],result['loop_end']),(1,8,28))
    def test_sampler_before_format_is_not_found(self):
        result=module.extract_wave_metadata(self.wave(1,2,7,before=True))
        self.assertEqual(result['loop_count'],0);self.assertIsNone(result['loop_start'])
    def test_invalid_loop_range_keeps_written_offsets(self):
        result=module.extract_wave_metadata(self.wave(1,7,2))
        self.assertEqual((result['loop_count'],result['loop_start'],result['loop_end']),(0,14,4))
    def test_signed_count_and_wrapping_offsets(self):
        result=module.extract_wave_metadata(self.wave(-1,0x80000000,0x80000001))
        self.assertEqual((result['loop_count'],result['loop_start'],result['loop_end']),(-1,0,2))
    def test_multiple_loops_are_rejected(self):
        with self.assertRaises(ValueError):module.extract_wave_metadata(self.wave(2))
if __name__=='__main__':unittest.main()
