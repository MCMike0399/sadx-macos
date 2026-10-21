#!/usr/bin/env python3
"""Decode selected owned JP/US WMA voice resources for the native player."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from music_assets import AUDIO_EXTENSIONS, float_wave_frames
from regular_textures import profile_evidence
from pe_image import PEImage
from extract import SOURCE_SHA256

sha=lambda value:hashlib.sha256(value).hexdigest()


def import_voice_assets(game,output,selected=(1,),*,languages=(0,1),ffmpeg=None,mod_directories=None):
    game,output=Path(game),Path(output)
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
    selected=list(dict.fromkeys(selected));languages=list(dict.fromkeys(languages))
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
            destination=output/relative;destination.parent.mkdir(parents=True,exist_ok=True)
            command=[str(decoder),'-nostdin','-hide_banner','-loglevel','error','-i',str(source),
                     '-map','0:a:0','-vn','-ac','2','-ar','44100','-c:a','pcm_f32le',
                     '-fflags','+bitexact','-flags:a','+bitexact','-y',str(destination)]
            result=subprocess.run(command,capture_output=True,text=True)
            if result.returncode:
                raise ValueError('voice decoder failed: '+result.stderr.strip())
            frames=float_wave_frames(destination)
            clips.append({'language':language,'voice':voice,
                          'source_path':'SYSTEM\\SoundData\\VOICE_'+label+'\\WMA\\'+filename,
                          'pcm_path':relative.as_posix(),'frames':frames,
                          'source_sha256':sha(source.read_bytes()),'pcm_sha256':sha(destination.read_bytes())})
    metadata={'version':1,'clips':clips}
    proof={'source_exe_sha256':image.sha256,'profile':profile,'decoder_sha256':sha(Path(decoder).read_bytes()),
           'format':'stereo44100Hzfloat32PCM-WAVE','clips':clips}
    output.mkdir(parents=True,exist_ok=True)
    (output/'voice-assets.json').write_text(json.dumps(metadata,indent=2)+'\n')
    (output/'voice-assets-evidence.json').write_text(json.dumps(proof,indent=2)+'\n')
    return metadata
