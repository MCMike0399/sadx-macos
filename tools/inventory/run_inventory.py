#!/usr/bin/env python3
"""Whole-program inventory through REA's Ghidra (docs/METHOD.md §2). Run from the repository root.

  run_inventory.py procedures        one list_procedures call -> reference/inventory-raw/inv-procedures.json
  run_inventory.py callees [--limit N] [--resume]
                                     procedure_callees for every non-thunk, non-external procedure
                                     (raw results in reference/inventory-raw/, ignored by git; hours for all)
  run_inventory.py build             compact reference/inventory/functions.json from the raw results
  run_inventory.py hot               reference/inventory/hot-paths.json: every procedure reachable from the
                                     frame loop 40cad0 through direct calls, with its static call depth
  run_inventory.py fpu GAME_EXE      reference/inventory/fpu-sites.json: byte-pattern candidates for the
                                     FPU/SSE instructions the guest must shim (step 4e), with the containing
                                     procedure; `confirmed` stays false until an instruction-level check

The Ghidra session is the owned runner reference/ghidra-live.mjs with reference/ghidra-inventory-config.json
(the game-loop snapshot and seeds). The raw directory is local; functions.json is the tracked product."""
import json,pathlib,subprocess,sys,time
ROOT=pathlib.Path(__file__).resolve().parents[2]
RAW=ROOT/'reference/inventory-raw';OUT=ROOT/'reference/inventory'
def payload(path):
    d=json.loads(path.read_text())
    if d.get('isError'):return None
    e=d.get('structuredContent') or json.loads(next(x for x in d['content'] if x['type']=='text')['text'])
    return e
def run(requests,log):
    """Feed a batch to one owned Ghidra session; returns when the runner has closed."""
    RAW.mkdir(parents=True,exist_ok=True)
    req=RAW/'requests.json';req.write_text(json.dumps(requests))
    start=pathlib.Path(log).stat().st_size if pathlib.Path(log).exists() else 0
    proc=subprocess.Popen(['node','reference/ghidra-live.mjs','reference/ghidra-inventory-config.json'],cwd=ROOT,
                          stdin=subprocess.PIPE,stdout=open(log,'a'),stderr=subprocess.STDOUT,text=True)
    def wait(text):
        while text not in pathlib.Path(log).read_text()[start:]:  # only this session's output
            if proc.poll() is not None:raise RuntimeError(f'runner exited; see {log}')
            time.sleep(3)
    wait('READY')
    proc.stdin.write(json.dumps({'tool':'batch','requests_file':str(req)})+'\n');proc.stdin.flush()
    wait('"retained":"%s"'%requests[-1]['label'])
    proc.stdin.write('{"tool":"close"}\n');proc.stdin.flush();proc.stdin.close();proc.wait()
def procedures():
    run([{'tool':'list_procedures','args':{},'label':'inv-procedures'}],RAW/'procedures.log')
def listed():
    e=payload(RAW/'inv-procedures.json')
    r=e['normalized_result']  # [{address, value: name, procedure: {external, thunk, thunk_target}}]
    return [dict(address=x['address'],name=x.get('value'),is_external=bool((x.get('procedure') or {}).get('external')),
                 is_thunk=bool((x.get('procedure') or {}).get('thunk')),thunk_target=(x.get('procedure') or {}).get('thunk_target'))
            for x in r]
def callees(limit=None,resume=False):
    procs=[p for p in listed() if not p.get('is_thunk') and not p.get('is_external')]
    todo=[]
    for p in procs:
        a=p['address'];label='inv-callees-'+a.lower().replace('0x','')
        if resume and (RAW/(label+'.json')).exists():continue
        todo.append({'tool':'procedure_callees','args':{'procedure':a},'label':label})
    if limit:todo=todo[:limit]
    print(f'{len(todo)} procedure_callees requests',flush=True)
    if todo:run(todo,RAW/'callees.log')
def build():
    procs=listed();OUT.mkdir(parents=True,exist_ok=True)
    functions=[]
    for p in procs:
        a=p['address'];f=RAW/('inv-callees-'+a.lower().replace('0x','')+'.json')
        entry=dict(p)
        if f.exists():
            e=payload(f)
            r=(e or {}).get('normalized_result')
            if isinstance(r,list):entry['callees']=sorted({c if isinstance(c,str) else c.get('address') for c in r})
        functions.append(entry)
    (OUT/'functions.json').write_text(json.dumps(dict(source='game/Sonic Adventure DX/sonic.exe (6b0937ca)',
        produced_by='tools/inventory/run_inventory.py',procedures=len(functions),
        with_callees=sum('callees' in f for f in functions),functions=functions),indent=0)+'\n')
    print(f'{len(functions)} procedures, {sum("callees" in f for f in functions)} with callees')
def hot(root='0x40cad0'):
    f={x['address']:x for x in json.loads((OUT/'functions.json').read_text())['functions']}
    depth={root:0};order=[root]
    for a in order:
        for c in f.get(a,{}).get('callees',[]) or []:
            if c not in depth and c in f:depth[c]=depth[a]+1;order.append(c)
    (OUT/'hot-paths.json').write_text(json.dumps(dict(root=root,reachable=len(order),
        note='direct calls only (Ghidra procedure_callees); pointer calls (vtables, task callbacks, the D3DX table) are not followed',
        functions=[dict(address=a,name=f[a].get('name'),depth=depth[a]) for a in order]),indent=0)+'\n')
    print(f'{len(order)} procedures reachable from {root}')
PATTERNS={'FPATAN':b'\xd9\xf3','FSINCOS':b'\xd9\xfb','FPTAN':b'\xd9\xf2','F2XM1':b'\xd9\xf0','FYL2X':b'\xd9\xf1','FYL2XP1':b'\xd9\xf9',
          'FSIN':b'\xd9\xfe','FCOS':b'\xd9\xff','RCPSS':b'\xf3\x0f\x53','RSQRTSS':b'\xf3\x0f\x52','RCPPS':b'\x0f\x53','RSQRTPS':b'\x0f\x52'}
def fpu(exe):
    sys.path.insert(0,str(ROOT/'tools/lib'));from pe_image import PEImage
    import bisect
    im=PEImage(exe);text=next(s for s in im.sections if s['name']=='.text')
    base=im.base+text['rva'];data=im.data[text['file_offset']:text['file_offset']+text['raw_size']]
    funcs=sorted(int(x['address'],16) for x in json.loads((OUT/'functions.json').read_text())['functions'] if not x['address'].startswith('EXTERNAL'))
    sites=[]
    for name,pat in PATTERNS.items():
        at=data.find(pat)
        while at>=0:
            va=base+at
            if name in('RCPPS','RSQRTPS') and at>0 and data[at-1]==0xf3:pass # the SS form, counted above
            else:
                i=bisect.bisect_right(funcs,va)-1
                sites.append(dict(address=hex(va),instruction=name,procedure=hex(funcs[i]) if i>=0 else None,confirmed=False))
            at=data.find(pat,at+1)
    sites.sort(key=lambda x:int(x['address'],16))
    from collections import Counter
    (OUT/'fpu-sites.json').write_text(json.dumps(dict(source=exe,counts=Counter(x['instruction'] for x in sites),
        note='byte-pattern scan of .text: candidates, not decoded instructions (a pattern can sit inside another instruction); step 4e confirms them in the guest',
        sites=sites),indent=0)+'\n')
    print(Counter(x['instruction'] for x in sites))
if __name__=='__main__':
    cmd=sys.argv[1] if len(sys.argv)>1 else ''
    if cmd=='procedures':procedures()
    elif cmd=='callees':callees(int(sys.argv[sys.argv.index('--limit')+1]) if '--limit' in sys.argv else None,'--resume' in sys.argv)
    elif cmd=='build':build()
    elif cmd=='hot':hot()
    elif cmd=='fpu':fpu(sys.argv[2])
    else:print(__doc__);sys.exit(2)
