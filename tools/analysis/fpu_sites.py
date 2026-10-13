#!/usr/bin/env python3
"""Confirm the FPU/SSE shim sites of sonic.exe (M1 4e) and write reference/inventory/fpu-sites.json.

Candidates come from `run_inventory.py fpu` (a byte-pattern scan). Each candidate is confirmed by a linear-sweep
disassembly (capstone, in the reference venv) of its containing procedure, from the procedure's start to the
next procedure's start: a candidate is `confirmed` when the sweep decodes an instruction of that mnemonic
starting exactly at it. Sweeps can desynchronise on data inside code (jump tables); such candidates stay
unconfirmed and are listed, never silently dropped.
Usage: reference/python/bin/python tools/analysis/fpu_sites.py GAME_EXE"""
import bisect,json,pathlib,sys
from collections import Counter
import capstone
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/lib'))
from pe_image import PEImage
MNEMONIC={'FPATAN':'fpatan','FSINCOS':'fsincos','FPTAN':'fptan','F2XM1':'f2xm1','FYL2X':'fyl2x','FYL2XP1':'fyl2xp1','FSIN':'fsin','FCOS':'fcos',
          'RCPSS':'rcpss','RSQRTSS':'rsqrtss','RCPPS':'rcpps','RSQRTPS':'rsqrtps'}
def main(exe):
    out=ROOT/'reference/inventory/fpu-sites.json';d=json.loads(out.read_text())
    funcs=sorted(int(x['address'],16) for x in json.loads((ROOT/'reference/inventory/functions.json').read_text())['functions'] if not x['address'].startswith('EXTERNAL'))
    im=PEImage(exe);md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    decoded={}
    for s in d['sites']:
        if not s['procedure']:continue
        p=int(s['procedure'],16)
        if p not in decoded:
            i=bisect.bisect_right(funcs,p);end=funcs[i] if i<len(funcs) else p+0x1000
            decoded[p]={ins.address:ins.mnemonic for ins in md.disasm(im.read(p,min(end-p,0x10000)),p)}
        s['confirmed']=decoded[p].get(int(s['address'],16))==MNEMONIC[s['instruction']]
    d['note']='byte-pattern candidates confirmed by a linear-sweep disassembly (capstone '+capstone.__version__+') of the containing procedure; unconfirmed = not an instruction start in that sweep'
    d['confirmed_counts']=Counter(s['instruction'] for s in d['sites'] if s['confirmed'])
    out.write_text(json.dumps(d,indent=0)+'\n')
    print(dict(d['confirmed_counts']))
if __name__=='__main__':main(sys.argv[1])
