import importlib.util,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/import'))
from test_voice_assets import wave
import menu_voice
spec=importlib.util.spec_from_file_location('private_voice_assets',Path(__file__).resolve().parents[2]/'tools/import/voice_assets.py');voice=importlib.util.module_from_spec(spec);spec.loader.exec_module(voice)
class Publication(unittest.TestCase):
 def test_payload_and_manifests_hardlinks_do_not_modify_owned_files(self):
  with tempfile.TemporaryDirectory() as t:
   root=Path(t);game=root/'game';decoder=root/'ffmpeg';decoder.write_bytes(b'decoder');mod=root/'mod';mod.mkdir()
   directory=game/'system/SoundData/VOICE_US/WMA';directory.mkdir(parents=True);source=directory/'0037.WMA';source.write_bytes(b'actual-owned-source');before=source.stat()
   output=root/'output';(output/'voice/us').mkdir(parents=True);(output/'voice/us/0037.wav').hardlink_to(source);(output/'voice-assets.json').hardlink_to(source);(output/'voice-assets-evidence.json').hardlink_to(source)
   def decode(command,**unused):Path(command[-1]).write_bytes(wave());return type('R',(),{'returncode':0,'stderr':''})()
   with patch.object(voice,'PEImage',return_value=type('I',(),{'sha256':voice.SOURCE_SHA256})()),patch.object(voice,'profile_evidence',return_value={}),patch.object(voice.subprocess,'run',side_effect=decode):
    result=voice.import_voice_assets(game,output,[37],languages=[1],ffmpeg=decoder,mod_directories=[mod])
   self.assertEqual(result['clips'][0]['voice'],37);self.assertEqual(source.read_bytes(),b'actual-owned-source');self.assertEqual(source.stat().st_ino,before.st_ino);self.assertEqual(source.stat().st_mtime_ns,before.st_mtime_ns)
   self.assertEqual((output/'voice/us/0037.wav').read_bytes(),wave())
 def test_failed_decoder_preserves_previous_pcm(self):
  with tempfile.TemporaryDirectory() as t:
   root=Path(t);game=root/'game';directory=game/'system/SoundData/VOICE_US/WMA';directory.mkdir(parents=True);(directory/'0037.wma').write_bytes(b'owned');decoder=root/'decoder';decoder.write_bytes(b'x');mod=root/'mod';mod.mkdir();output=root/'out';(output/'voice/us').mkdir(parents=True);target=output/'voice/us/0037.wav';target.write_bytes(b'previous')
   def decode(command,**unused):Path(command[-1]).write_bytes(b'partial');return type('R',(),{'returncode':1,'stderr':'actual error'})()
   with patch.object(voice,'PEImage',return_value=type('I',(),{'sha256':voice.SOURCE_SHA256})()),patch.object(voice,'profile_evidence',return_value={}),patch.object(voice.subprocess,'run',side_effect=decode):
    with self.assertRaises(ValueError):voice.import_voice_assets(game,output,[37],languages=[1],ffmpeg=decoder,mod_directories=[mod])
   self.assertEqual(target.read_bytes(),b'previous');self.assertEqual([p.name for p in target.parent.iterdir()],['0037.wav'])
if __name__=='__main__':unittest.main()
