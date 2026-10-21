"""Protect owned source bytes and previous good outputs during music decode."""
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
from test_music_assets import Image, module, wave


class MusicPublication(unittest.TestCase):
    def exercise(self, root, *, mode='success', link=None):
        game=root/'game';source=game/'system/SoundData/BGM/WMA'
        source.mkdir(parents=True)
        owned=source/'song101.wma';owned.write_bytes(b'owned-source')
        decoder=root/'decoder';decoder.write_bytes(b'fixture decoder')
        output=root/'out';music=output/'music';music.mkdir(parents=True)
        target=music/'101-song101.wav';target.write_bytes(b'previous-good')
        manifest=output/'music-assets.json';manifest.write_bytes(b'previous-manifest')
        if link=='clip-hardlink':target.unlink();os.link(owned,target)
        elif link=='manifest-hardlink':manifest.unlink();os.link(owned,manifest)
        elif link=='clip-symlink':target.unlink();target.symlink_to(owned)
        elif link=='directory-symlink':
            target.unlink();music.rmdir();music.symlink_to(source,target_is_directory=True)
        elif link=='manifest-symlink':manifest.unlink();manifest.symlink_to(owned)
        before=(owned.read_bytes(),owned.stat().st_ino)
        old_target=target.read_bytes() if target.exists() else None
        old_manifest=manifest.read_bytes()
        image=Image();image.data[module.MENU_REQUESTS]=struct.pack('<3i',102,101,72)
        def decode(command, **kwargs):
            Path(command[-1]).write_bytes(b'invalid' if mode=='malformed' else wave())
            return type('Result',(),{'returncode':int(mode=='failure'),'stderr':'codec failure'})()
        try:
            with patch.object(module,'PEImage',return_value=image),patch.object(module,'profile_evidence',return_value={}),patch.object(module.subprocess,'run',side_effect=decode):
                result=module.import_music_assets(game,output,(101,),ffmpeg=decoder,mod_directories=[],menu_request_count=3)
            self.assertEqual(result['clips'][0]['song'],101)
            self.assertEqual(target.read_bytes(),wave())
            self.assertNotEqual(target.stat().st_ino,owned.stat().st_ino)
            self.assertNotEqual(manifest.stat().st_ino,owned.stat().st_ino)
        except ValueError:
            self.assertEqual(manifest.read_bytes(),old_manifest)
            if old_target is not None:
                self.assertEqual(target.read_bytes(),old_target)
            raise
        finally:
            self.assertEqual((owned.read_bytes(),owned.stat().st_ino),before)
            self.assertEqual(list(music.glob('.music-copy-*')),[])

    def test_owned_source_hardlinks_are_replaced_using_fresh_inodes(self):
        for link in ('clip-hardlink','manifest-hardlink'):
            with self.subTest(link=link),tempfile.TemporaryDirectory() as temp:
                self.exercise(Path(temp),link=link)

    def test_decoder_failure_and_invalid_pcm_preserve_previous_good_files(self):
        for mode in ('failure','malformed'):
            with self.subTest(mode=mode),tempfile.TemporaryDirectory() as temp:
                with self.assertRaises(ValueError):self.exercise(Path(temp),mode=mode)

    def test_source_and_manifest_symlinks_rejected_without_owned_source_mutation(self):
        for link in ('clip-symlink','directory-symlink','manifest-symlink'):
            with self.subTest(link=link),tempfile.TemporaryDirectory() as temp:
                with self.assertRaisesRegex(ValueError,'symlink'):self.exercise(Path(temp),link=link)

    def test_output_redirect_into_owned_install_is_rejected_before_parsing(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);game=root/'game';game.mkdir()
            redirect=root/'redirect';redirect.symlink_to(game,target_is_directory=True)
            with self.assertRaisesRegex(ValueError,'outside the owned install'):
                module.import_music_assets(game,redirect/'nested')

    def test_output_inside_owned_install_is_rejected_before_parsing(self):
        with tempfile.TemporaryDirectory() as temp:
            game=Path(temp)/'game';game.mkdir()
            for output in (game,game/'output'):
                with self.assertRaisesRegex(ValueError,'outside the owned install'):
                    module.import_music_assets(game,output)


if __name__=='__main__':unittest.main()
