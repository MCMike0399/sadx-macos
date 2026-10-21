#!/usr/bin/env python3
"""Import the PC music/voice timing catalog and decode requested owned WMA assets."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from menu_voice import publish_json
from regular_textures import profile_evidence
from pe_image import PEImage
from extract import SOURCE_SHA256

SONGS = 0x910298
JAPANESE_DURATIONS = 0x910680
ENGLISH_DURATIONS = 0x911688
MENU_REQUESTS = 0x7ee8f8
AUDIO_EXTENSIONS = {'.wma','.wav','.adx','.ogg','.mp3','.flac','.aif','.aiff','.aac','.m4a'}
sha = lambda raw: hashlib.sha256(raw).hexdigest()


def read_catalog(image, menu_request_count=1):
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('music catalog requires the pinned executable SHA256')
    if type(menu_request_count) is not int or not 1 <= menu_request_count <= 3:
        raise ValueError('menu request prefix exceeds recovered caller indices')
    raw = image.read(SONGS, 125 * 8)
    songs, strings, inverse = [], {}, bytearray()
    for index in range(125):
        name_address, loop = struct.unpack_from('<Ii', raw, index * 8)
        if not name_address:
            raise ValueError('null source music name')
        name = image.string(name_address)
        name_raw = name.encode('ascii') + b'\0'
        if not name or any(character in name for character in '/\\.:'):
            raise ValueError('music catalog contains an unrecovered source path')
        if image.read(name_address, len(name_raw)) != name_raw:
            raise ValueError('music name inverse mismatch')
        songs.append({'name':name,'loop':loop})
        strings[hex(name_address)] = name_raw.hex()
        inverse += struct.pack('<Ii',name_address,loop)
    jp_raw = image.read(JAPANESE_DURATIONS, 2049 * 2)
    en_raw = image.read(ENGLISH_DURATIONS, 2049 * 2)
    japanese = list(struct.unpack('<2049H',jp_raw))
    english = list(struct.unpack('<2049H',en_raw))
    # Title and FileSelect prove request indices0 and2, so retain their whole prefix.
    # The rest of the unbounded source request array is not assigned a guessed size.
    request_raw = image.read(MENU_REQUESTS,menu_request_count*4)
    requests = list(struct.unpack(f'<{menu_request_count}i',request_raw))
    if any(song < 0 or song >= len(songs) for song in requests):
        raise ValueError('title music mapping is outside the source song catalog')
    if bytes(inverse) != raw or struct.pack('<2049H',*japanese) != jp_raw or struct.pack('<2049H',*english) != en_raw:
        raise ValueError('music catalog inverse mismatch')
    ranges = [(SONGS,bytes(inverse)),(JAPANESE_DURATIONS,jp_raw),(ENGLISH_DURATIONS,en_raw),(MENU_REQUESTS,request_raw)]
    metadata = {'songs':songs,'japanese_voice_durations':japanese,
                'english_voice_durations':english,'menu_requests':requests}
    proof = {'source_sha256':image.sha256,'source_ranges':{f'{hex(address)}-{hex(address+len(data))}':sha(data) for address,data in ranges},
             'records': [{'address':hex(address),'bytes':data.hex(),'inverse_exact':True} for address,data in ranges],
             'strings':strings,'voice_duration_element_bytes':2,
             'scope':f'125songs,2049uint16durations per language,proved menu request prefix of {menu_request_count} entries.'}
    return metadata,proof


def float_wave_frames(path):
    raw = Path(path).read_bytes()
    if raw[:4] != b'RIFF' or raw[8:12] != b'WAVE':
        raise ValueError('decoded music is not a RIFF WAVE')
    offset, format_data, data = 12, None, None
    while offset + 8 <= len(raw):
        kind,size = struct.unpack_from('<4sI',raw,offset)
        payload = raw[offset+8:offset+8+size]
        if len(payload) != size:raise ValueError('truncated decoded music chunk')
        if kind == b'fmt ':format_data=payload
        elif kind == b'data':data=payload
        offset += 8 + size + (size & 1)
    if format_data is None or len(format_data) < 16 or data is None:
        raise ValueError('decoded music lacks format or samples')
    kind,channels,rate,byte_rate,align,bits=struct.unpack_from('<HHIIHH',format_data)
    if kind == 65534 and len(format_data) >= 40:
        kind=struct.unpack_from('<I',format_data,24)[0]
    if (kind,channels,rate,byte_rate,align,bits) != (3,2,44100,352800,8,32) or not data or len(data)%8:
        raise ValueError('decoded music must be stereo44100Hzfloat32PCM')
    return len(data)//8


def import_music_assets(game, output, selected=(102,), *, ffmpeg=None, mod_directories=None,
                        menu_request_count=1):
    game,output=Path(game),Path(output)
    if output.resolve()==game.resolve() or game.resolve() in output.resolve().parents:
        raise ValueError('music output must be outside the owned install')
    if output.is_symlink():
        raise ValueError('music output cannot redirect through a symlink')
    catalog,proof=read_catalog(PEImage(game/'sonic.exe'),menu_request_count)
    profile=profile_evidence(game,{'groups':[],'lists':[],'localizations':[]},mod_directories)
    if mod_directories is None:
        directories=[game/'mods'/name for name in profile['enabled_mods']]
    else:directories=[Path(path) for path in mod_directories]
    for directory in directories:
        if any(path.is_file() and path.suffix.casefold() in AUDIO_EXTENSIONS for path in directory.rglob('*')):
            raise ValueError('music replacement overlay requires explicit import support')
    decoder=ffmpeg or os.environ.get('SADX_FFMPEG') or shutil.which('ffmpeg')
    if not decoder:
        raise ValueError('WMA import requires ffmpeg (pass ffmpeg= or SADX_FFMPEG)')
    decoder=Path(decoder)
    if not decoder.is_file():raise ValueError('music decoder executable is missing')
    for filename in ('music-assets.json','music-assets-evidence.json'):
        if (output/filename).is_symlink():
            raise ValueError('music manifest destination cannot be a symlink')
    clips=[]
    directory=game/'system/SoundData/BGM/WMA'
    children=list(directory.iterdir())
    if (output/'music').is_symlink():
        raise ValueError('music destination cannot redirect through a symlink')
    (output/'music').mkdir(parents=True,exist_ok=True)
    for song in dict.fromkeys(selected):
        if type(song) is not int or not 0<=song<len(catalog['songs']):raise ValueError('unsupported selected music index')
        name=catalog['songs'][song]['name']
        matches=[path for path in children if path.name.casefold()==(name+'.wma').casefold() and path.is_file()]
        if len(matches)!=1:raise ValueError('source music missing or ambiguous: '+name)
        source=matches[0]
        relative=Path('music')/(str(song)+'-'+name+'.wav')
        destination=output/relative
        if destination.is_symlink():
            raise ValueError('music destination cannot redirect through a symlink')
        descriptor,temporary_name=tempfile.mkstemp(prefix='.music-copy-',suffix='.wav',dir=destination.parent)
        os.close(descriptor);temporary=Path(temporary_name)
        command=[str(decoder),'-nostdin','-hide_banner','-loglevel','error','-i',str(source),
                 '-map','0:a:0','-vn','-ac','2','-ar','44100','-c:a','pcm_f32le',
                 '-fflags','+bitexact','-flags:a','+bitexact','-y',str(temporary)]
        source_hash=sha(source.read_bytes())
        try:
            completed=subprocess.run(command,capture_output=True,text=True)
            if completed.returncode:
                raise ValueError('music decoder failed: '+completed.stderr.strip())
            frames=float_wave_frames(temporary)
            if sha(source.read_bytes())!=source_hash:
                raise ValueError('owned music changed during decode')
            os.replace(temporary,destination)
        finally:temporary.unlink(missing_ok=True)
        clips.append({'song':song,'source_path':'SYSTEM\\SoundData\\BGM\\WMA\\'+name+'.wma',
                      'pcm_path':relative.as_posix(),'frames':frames,
                      'source_sha256':source_hash,'pcm_sha256':sha(destination.read_bytes())})
    metadata={'version':1,'catalog':catalog,'clips':clips}
    proof['profile']=profile
    proof['decoder_sha256']=sha(decoder.read_bytes())
    proof['decoded_clips']=clips
    publish_json(output/'music-assets.json',metadata)
    publish_json(output/'music-assets-evidence.json',proof)
    return metadata
