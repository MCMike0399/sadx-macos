#!/usr/bin/env python3
"""Whole-program inventory through REA's Ghidra (docs/METHOD.md §2). Run from the repository root.

  run_inventory.py procedures        one list_procedures call -> reference/inventory-raw/inv-procedures.json
  run_inventory.py callees [--limit N] [--resume]
                                     procedure_callees for every non-thunk, non-external procedure
                                     (raw results in reference/inventory-raw/, ignored by git; hours for all)
  run_inventory.py build             compact reference/inventory/functions.json from the raw results

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
    proc=subprocess.Popen(['node','reference/ghidra-live.mjs','reference/ghidra-inventory-config.json'],cwd=ROOT,
                          stdin=subprocess.PIPE,stdout=open(log,'a'),stderr=subprocess.STDOUT,text=True)
    def wait(text):
        while text not in pathlib.Path(log).read_text():
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
if __name__=='__main__':
    cmd=sys.argv[1] if len(sys.argv)>1 else ''
    if cmd=='procedures':procedures()
    elif cmd=='callees':callees(int(sys.argv[sys.argv.index('--limit')+1]) if '--limit' in sys.argv else None,'--resume' in sys.argv)
    elif cmd=='build':build()
    else:print(__doc__);sys.exit(2)
