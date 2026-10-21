#!/usr/bin/env python3
"""Decode selected owned JP/US WMA voice resources for the native player."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from menu_voice import required_voices, publish_json
from music_assets import AUDIO_EXTENSIONS, float_wave_frames
from regular_textures import profile_evidence
from pe_image import PEImage
from extract import SOURCE_SHA256

sha=lambda value:hashlib.sha256(value).hexdigest()


def import_voice_assets(game,output,selected=None,*,languages=(0,1),ffmpeg=None,mod_directories=None):
    game,output=Path(game),Path(output)
    if output.resolve()==game.resolve() or game.resolve() in output.resolve().parents:
        raise ValueError('voice output must be outside the owned install')
    image=PEImage(game/'sonic.exe')
    if image.sha256!=SOURCE_SHA256:
        raise ValueError('voice assets require the pinned executable SHA256')
    profile=profile_evidence(game,{'groups':[],'lists':[],'localizations':[]},mod_directories)
    directories=[game/'mods'/name for name in profile['enabled_mods']] if mod_directories is None else [Path(value) for value in mod_directories]
    for directory in directories:
        if any(path.is_file() and path.suffix.casefold() in AUDIO_EXTENSIONS for path in directory.rglob('*')):
            raise ValueError('voice replacement overlay requires explicit import support')
    decoder=ffmpeg or os.environ.get('SADX_FFMPEG') or shutil.which('ffmpeg')
    if not decoder or not Path(decoder).is_file():
        raise ValueError('voice WMA import requires ffmpeg (pass ffmpeg= or SADX_FFMPEG)')
    selected=list(dict.fromkeys(required_voices(image) if selected is None else selected));languages=list(dict.fromkeys(languages))
    if any(type(value) is not int or not 0<=value<2049 for value in selected):
        raise ValueError('selected voice is outside the source duration catalog')
    if any(type(value) is not int or value not in (0,1) for value in languages):
        raise ValueError('voice language requires JP0 or non-JP/US1 resource identity')
    clips=[]
    for language in languages:
        label='JP' if language==0 else 'US'
        directory=game/('system/SoundData/VOICE_'+label+'/WMA')
        children=list(directory.iterdir())
        for voice in selected:
            filename=f'{voice:04d}.wma'
            matches=[path for path in children if path.is_file() and path.name.casefold()==filename]
            if len(matches)!=1:
                raise ValueError('source voice missing or ambiguous: '+label+'/'+filename)
            source=matches[0]
            relative=Path('voice')/label.lower()/f'{voice:04d}.wav'
            destination=output/relative
            if destination.is_symlink() or destination.parent.is_symlink() or (output/'voice').is_symlink():
                raise ValueError('voice destination cannot redirect through a symlink')
            destination.parent.mkdir(parents=True,exist_ok=True)
            descriptor,temporary_name=tempfile.mkstemp(prefix='.voice-copy-',suffix='.wav',dir=destination.parent)
            os.close(descriptor);temporary=Path(temporary_name)
            command=[str(decoder),'-nostdin','-hide_banner','-loglevel','error','-i',str(source),
                     '-map','0:a:0','-vn','-ac','2','-ar','44100','-c:a','pcm_f32le',
                     '-fflags','+bitexact','-flags:a','+bitexact','-y',str(temporary)]
            source_hash=sha(source.read_bytes())
            try:
                result=subprocess.run(command,capture_output=True,text=True)
                if result.returncode:
                    raise ValueError('voice decoder failed: '+result.stderr.strip())
                frames=float_wave_frames(temporary)
                if sha(source.read_bytes())!=source_hash:
                    raise ValueError('owned voice changed during decode')
                os.replace(temporary,destination)
            finally:temporary.unlink(missing_ok=True)
            clips.append({'language':language,'voice':voice,
                          'source_path':'SYSTEM\\SoundData\\VOICE_'+label+'\\WMA\\'+filename,
                          'pcm_path':relative.as_posix(),'frames':frames,
                          'source_sha256':source_hash,'pcm_sha256':sha(destination.read_bytes())})
    metadata={'version':1,'clips':clips}
    proof={'source_exe_sha256':image.sha256,'profile':profile,'decoder_sha256':sha(Path(decoder).read_bytes()),
           'format':'stereo44100Hzfloat32PCM-WAVE','clips':clips}
    output.mkdir(parents=True,exist_ok=True)
    publish_json(output/'voice-assets.json',metadata)
    publish_json(output/'voice-assets-evidence.json',proof)
    return metadata
