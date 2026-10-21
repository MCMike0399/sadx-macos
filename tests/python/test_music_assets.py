import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('music_assets',ROOT/'tools/import/music_assets.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class Image:
    sha256=module.SOURCE_SHA256
    def __init__(self):
        self.data={module.SONGS:b''.join(struct.pack('<Ii',0x2000+i*32,i%2) for i in range(125)),
                   module.JAPANESE_DURATIONS:struct.pack('<2049H',*range(2049)),
                   module.ENGLISH_DURATIONS:struct.pack('<2049H',*[65535-i for i in range(2049)]),
                   module.MENU_REQUESTS:struct.pack('<i',102)}
        self.data.update({0x2000+i*32:('song'+str(i)).encode()+b'\0' for i in range(125)})
    def read(self,address,size):
        for start,data in self.data.items():
            if start<=address and address+size<=start+len(data):return data[address-start:address-start+size]
        raise ValueError('outside fixture source')
    def string(self,address):return self.data[address][:-1].decode()

def wave(frames=3):
    fmt=struct.pack('<HHIIHH',3,2,44100,352800,8,32)
    samples=struct.pack('<'+str(frames*2)+'f',*[0.25]*(frames*2))
    content=b'WAVE'+b'fmt '+struct.pack('<I',len(fmt))+fmt+b'data'+struct.pack('<I',len(samples))+samples
    return b'RIFF'+struct.pack('<I',len(content))+content

class MusicAssets(unittest.TestCase):
    def test_catalog_inverse_and_uint16_duration_width(self):
        catalog,proof=module.read_catalog(Image())
        self.assertEqual(len(catalog['songs']),125)
        self.assertEqual(catalog['songs'][102],{'name':'song102','loop':0})
        self.assertEqual(catalog['menu_requests'],[102])
        self.assertEqual(catalog['japanese_voice_durations'][-1],2048)
        self.assertEqual(catalog['english_voice_durations'][0],65535)
        self.assertEqual(proof['voice_duration_element_bytes'],2)
        self.assertTrue(all(row['inverse_exact'] for row in proof['records']))
        self.assertNotIn('address',str(catalog))
    def test_wrong_source_and_unmapped_request_rejected(self):
        image=Image();image.sha256='wrong'
        with self.assertRaisesRegex(ValueError,'SHA256'):module.read_catalog(image)
        image=Image();image.data[module.MENU_REQUESTS]=struct.pack('<i',125)
        with self.assertRaisesRegex(ValueError,'mapping'):module.read_catalog(image)
    def test_source_music_name_does_not_escape_asset_directory(self):
        image=Image();image.data[0x2000]=b'../outside\0'
        with self.assertRaisesRegex(ValueError,'source path'):module.read_catalog(image)
    def test_float_wave_validation(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'test.wav';path.write_bytes(wave())
            self.assertEqual(module.float_wave_frames(path),3)
            path.write_bytes(wave()[:-1])
            with self.assertRaisesRegex(ValueError,'truncated'):module.float_wave_frames(path)
    def test_wrong_pcm_rate_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'test.wav';data=bytearray(wave());struct.pack_into('<I',data,24,22050);path.write_bytes(data)
            with self.assertRaisesRegex(ValueError,'44100'):module.float_wave_frames(path)
    def import_fixture(self,root,decoder_result=0,selected=(102,),overlay=False):
        game=root/'game';source=game/'system/SoundData/BGM/WMA';source.mkdir(parents=True)
        (source/'song102.WMA').write_bytes(b'owned-source')
        mod=root/'mod';mod.mkdir()
        if overlay:(mod/'replacement.wma').write_bytes(b'overlay')
        decoder=root/'decoder';decoder.write_bytes(b'fixture decoder')
        def decode(command,**kwargs):
            Path(command[-1]).write_bytes(wave())
            return type('Result',(),{'returncode':decoder_result,'stderr':'codec failure'})()
        with patch.object(module,'PEImage',return_value=Image()),patch.object(module,'profile_evidence',return_value={}),patch.object(module.subprocess,'run',side_effect=decode):
            return module.import_music_assets(game,root/'out',selected,ffmpeg=decoder,mod_directories=[mod])
    def test_import_preserves_source_hash_and_native_pcm_shape(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);result=self.import_fixture(root,selected=(102,102))
            self.assertEqual(len(result['clips']),1)
            clip=result['clips'][0]
            self.assertEqual(clip['frames'],3)
            self.assertEqual(clip['source_path'],'SYSTEM\\SoundData\\BGM\\WMA\\song102.wma')
            self.assertEqual(clip['source_sha256'],module.sha(b'owned-source'))
            self.assertEqual((root/'out'/clip['pcm_path']).read_bytes(),wave())
    def test_audio_overlay_is_not_silently_ignored(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'overlay'):self.import_fixture(Path(temp),overlay=True)
    def test_decoder_failure_is_not_success(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'decoder failed'):self.import_fixture(Path(temp),decoder_result=1)
    def test_absent_selected_track_and_invalid_index_are_explicit(self):
        for selected in [(0,),(-1,),(125,)]:
            with tempfile.TemporaryDirectory() as temp:
                with self.assertRaisesRegex(ValueError,'missing or ambiguous|unsupported selected'):self.import_fixture(Path(temp),selected=selected)

if __name__=='__main__':unittest.main()
