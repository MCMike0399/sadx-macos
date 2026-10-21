import importlib.util,json,struct,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/import'))
import menu_voice,save_files
class Image:
    sha256=menu_voice.SOURCE_SHA256
    def read(self,address,size):
        row=(address-menu_voice.TABLE)//36
        if not 0<=row<9 or size!=36:raise ValueError('out-of-table read')
        return struct.pack('<9i',*(row*9+request for request in range(9)))
class MenuRows(unittest.TestCase):
    def test_complete_rows_inverse(self):
        data,proof=menu_voice.declarations(Image(),range(9))
        self.assertEqual(len(data['entries']),81)
        self.assertEqual({(e['character'],e['request']) for e in data['entries']},{(r,q) for r in range(9) for q in range(9)})
        self.assertEqual([e['voice'] for e in data['entries']],list(range(81)))
        self.assertEqual(len(proof['rows']),9);self.assertTrue(proof['round_trip_exact'])
    def test_import_defaults_all_rows(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);game=root/'game';game.mkdir()
            with patch.object(menu_voice,'PEImage',return_value=Image()):
                metadata=menu_voice.import_menu_voice(game,root/'out')
            self.assertEqual(len(metadata['entries']),81)
    def test_required_clips_follow_catalog(self):self.assertEqual(menu_voice.required_voices(Image()),list(range(81)))
    def test_invalid_rows_rejected(self):
        for row in [-1,9,True]:
            with self.assertRaises(ValueError):menu_voice.declarations(Image(),[row])
    def test_publish_hardlink_does_not_mutate_source(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);source=root/'owned';source.write_bytes(b'owned');before=source.stat();out=root/'manifest.json';out.hardlink_to(source)
            menu_voice.publish_json(out,{'version':1})
            self.assertEqual(source.read_bytes(),b'owned');self.assertEqual(source.stat().st_ino,before.st_ino);self.assertEqual(source.stat().st_mtime_ns,before.st_mtime_ns)
    def test_manifest_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);source=root/'owned';source.write_bytes(b'owned');out=root/'manifest';out.symlink_to(source)
            with self.assertRaises(ValueError):menu_voice.publish_json(out,{})
class EmptySaveSnapshot(unittest.TestCase):
    def test_absent_savedata_writes_empty_manifest_and_preserves_progress(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);game=root/'game';game.mkdir();out=root/'assets';(out/'saves').mkdir(parents=True);prior=out/'saves/SonicDX01.snc';prior.write_bytes(b'existing imported/native progress')
            self.assertEqual(save_files.import_save_files(game,out),{'version':1,'files':[]})
            self.assertEqual(json.loads((out/'save-files.json').read_text()),{'version':1,'files':[]})
            self.assertEqual(prior.read_bytes(),b'existing imported/native progress');self.assertFalse((game/'SAVEDATA').exists())
    def test_malformed_savedata_remains_error(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);game=root/'game';game.mkdir();(game/'SAVEDATA').write_bytes(b'not directory')
            with self.assertRaises(ValueError):save_files.import_save_files(game,root/'out')
    def test_savedata_symlink_remains_error(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);game=root/'game';game.mkdir();real=root/'real';real.mkdir();(game/'SAVEDATA').symlink_to(real)
            with self.assertRaises(ValueError):save_files.import_save_files(game,root/'out')
    def test_ambiguous_savedata_remains_error(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);game=root/'game';game.mkdir()
            with patch.object(Path,'iterdir',return_value=iter([game/'SAVEDATA',game/'savedata'])):
                with self.assertRaises(ValueError):save_files.import_save_files(game,root/'out')
if __name__=='__main__':unittest.main()
