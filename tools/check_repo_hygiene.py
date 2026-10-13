#!/usr/bin/env python3
"""Fail when the git index (or, with --worktree, the tracked files) contains anything that
must never be committed: executables, DLLs, game data, disc images, archives, or any file
over 2 MiB. Runs in `make check` and is cheap enough for a pre-commit hook."""
import pathlib,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
FORBIDDEN_SUFFIXES={'.exe','.dll','.bin','.cue','.iso','.pvm','.pvr','.prs','.wma','.adx','.7z','.zip','.dylib','.a','.o','.snc'}
FORBIDDEN_PREFIXES=('game/','reference/python/','reference/updated-pc-reference/game/','reference/windows-transfer/','reference/sadx-mod-loader/')
LIMIT=2*1024*1024
def main():
    tracked=subprocess.run(['git','ls-files','-z','--cached'],cwd=ROOT,capture_output=True).stdout.decode().split('\0')
    bad=[]
    for rel in tracked:
        if not rel:continue
        p=ROOT/rel
        if rel.startswith(FORBIDDEN_PREFIXES):bad.append((rel,'forbidden path'));continue
        if p.suffix.lower() in FORBIDDEN_SUFFIXES:bad.append((rel,'forbidden type'));continue
        if p.exists() and p.stat().st_size>LIMIT:bad.append((rel,f'{p.stat().st_size} bytes > 2 MiB'))
    for rel,why in bad:print(f'HYGIENE: {rel}: {why}')
    print(f'{len(tracked)-1} tracked files, {len(bad)} violations')
    return 1 if bad else 0
if __name__=='__main__':sys.exit(main())
