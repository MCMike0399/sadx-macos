#!/usr/bin/env python3
"""Enforce the layered layout of src/ (docs/ARCHITECTURE.md): a file may include headers
only from its own layer or a lower one. Dependencies that are known debt are listed in
tools/gate/layer_exceptions.tsv (one `<including file>\t<included header>` per line) and
must be removed from that file when they are fixed; an exception that no longer matches
anything is itself an error, so the list cannot rot."""
import pathlib,re,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
# Lowest first. A layer may include itself and anything before it.
LAYERS=['cpu','loader','math','engine','input','player','game','runtime','guest','hle','platform','apps']
INCLUDE=re.compile(r'^#include "([a-z0-9_/]+\.hpp)"',re.M)
def layer_of(rel):
    return rel.parts[0]
def main():
    exceptions=set()
    exc_file=ROOT/'tools/gate/layer_exceptions.tsv'
    for line in exc_file.read_text().splitlines():
        if line.strip() and not line.startswith('#'):exceptions.add(tuple(line.split('\t')[:2]))
    used=set();bad=[];unknown=[]
    for f in sorted((ROOT/'src').rglob('*')):
        if f.suffix not in('.cpp','.hpp','.mm'):continue
        rel=f.relative_to(ROOT/'src');a=layer_of(rel)
        if a not in LAYERS:unknown.append(str(rel));continue
        for inc in INCLUDE.findall(f.read_text()):
            b=inc.split('/')[0]
            if b not in LAYERS:bad.append((str(rel),inc,'unknown layer'));continue
            if LAYERS.index(b)<=LAYERS.index(a):continue
            key=(f'src/{rel}',inc)
            if key in exceptions:used.add(key);continue
            bad.append((str(rel),inc,f'{a} may not include {b}'))
    for rel,inc,why in bad:print(f'LAYER: src/{rel} includes {inc}: {why}')
    for rel in unknown:print(f'LAYER: src/{rel}: directory is not a layer ({", ".join(LAYERS)})')
    for key in sorted(exceptions-used):print(f'LAYER: stale exception {key[0]} -> {key[1]} (fixed? remove it from layer_exceptions.tsv)')
    n=len(bad)+len(unknown)+len(exceptions-used)
    print(f'{len(LAYERS)} layers, {len(used)} known exceptions, {n} violations')
    return 1 if n else 0
if __name__=='__main__':sys.exit(main())
