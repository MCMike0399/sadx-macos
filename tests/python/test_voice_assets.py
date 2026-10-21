import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import sys
sys.path.insert(0,str(Path.cwd()/'tools/import'))
SOURCE=Path(__file__).resolve().parents[2]/'tools/import/voice_assets.py'
spec=importlib.util.spec_from_file_location('voice_assets',SOURCE);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def wave():
    fmt=struct.pack('<HHIIHH',3,2,44100,352800,8,32);samples=struct.pack('<4f',0.25,-0.25,0,0)
    body=b'WAVE'+b'fmt '+struct.pack('<I',16)+fmt+b'data'+struct.pack('<I',len(samples))+samples
    return b'RIFF'+struct.pack('<I',len(body))+body

class VoiceAssets(unittest.TestCase):
    def fixture(self,root,selected=(1,),languages=(0,1),result=0,overlay=False,wrong_source=False):
        game=root/'game';mod=root/'mod';mod.mkdir();decoder=root/'ffmpeg';decoder.write_bytes(b'decoder')
        if overlay:(mod/'clip.wma').write_bytes(b'overlay')
        for label in ['JP','US']:
            directory=game/('system/SoundData/VOICE_'+label+'/WMA');directory.mkdir(parents=True)
            (directory/'0001.WMA').write_bytes(('owned-'+label).encode())
        def decode(command,**unused):
            Path(command[-1]).write_bytes(wave())
            return type('Result',(),{'returncode':result,'stderr':'decode error'})()
        image=type('Image',(),{'sha256':'wrong' if wrong_source else module.SOURCE_SHA256})()
        with patch.object(module,'PEImage',return_value=image),patch.object(module,'profile_evidence',return_value={}),patch.object(module.subprocess,'run',side_effect=decode):
            return module.import_voice_assets(game,root/'output',selected,languages=languages,ffmpeg=decoder,mod_directories=[mod])
    def test_language_resources_remain_distinct(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);r=self.fixture(root)
            self.assertEqual([(c['language'],c['voice'],c['frames']) for c in r['clips']],[(0,1,2),(1,1,2)])
            self.assertEqual(r['clips'][0]['source_path'],'SYSTEM\\SoundData\\VOICE_JP\\WMA\\0001.wma')
            self.assertEqual(r['clips'][1]['source_path'],'SYSTEM\\SoundData\\VOICE_US\\WMA\\0001.wma')
            self.assertNotEqual(r['clips'][0]['source_sha256'],r['clips'][1]['source_sha256'])
            self.assertEqual((root/'output'/r['clips'][0]['pcm_path']).read_bytes(),wave())
    def test_duplicates_are_not_decoded_as_distinct_owners(self):
        with tempfile.TemporaryDirectory() as temp:
            r=self.fixture(Path(temp),selected=(1,1),languages=(1,1));self.assertEqual(len(r['clips']),1)
    def test_selected_voice_boundaries_are_explicit(self):
        for selected in [(-1,),(2049,),(True,)]:
            with tempfile.TemporaryDirectory() as temp:
                with self.assertRaisesRegex(ValueError,'catalog'):self.fixture(Path(temp),selected=selected)
    def test_language_requires_actual_source_resource_identity(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'language'):self.fixture(Path(temp),languages=(2,))
    def test_missing_file_is_not_success(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'missing or ambiguous'):self.fixture(Path(temp),selected=(0,))
    def test_decoder_failure_is_not_success(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'decoder failed'):self.fixture(Path(temp),result=1)
    def test_wrong_source_identity_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'SHA256'):self.fixture(Path(temp),wrong_source=True)
    def test_replacement_overlay_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'overlay'):self.fixture(Path(temp),overlay=True)

if __name__=='__main__':unittest.main()
