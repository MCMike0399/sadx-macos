"""Mutation-test an oracle: apply each (name, old, new) edit from MUTANTS.json to SRC, rebuild PROBE, run the
make TARGET and report whether each mutant was caught. Usage: mutate.py SRC TARGET PROBE MUTANTS.json
Run from the repository root.

A mutant is caught when the oracle's report (build/oracles/<slug>.json) gets a worse verdict than the
unmutated baseline (tools/oracles/verdict.py: pass > partial > fail), its value-class reason changes, or
the oracle aborts without a report. Older oracles that print a `{"cases": ..., "mismatches": n}` line are
also caught when that number differs from the baseline's. Equivalent mutants (same verdict and reason) are listed as survivors."""
import os,pathlib,subprocess,time,json,sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'oracles'))
from verdict import verdict
f,target,probe=sys.argv[1],sys.argv[2],sys.argv[3];M=json.loads(open(sys.argv[4]).read())
report=pathlib.Path('build/oracles')/(target.removeprefix('oracle-').replace('-','_')+'.json')
RANK={'pass':0,'partial':1,'fail':2}
def run():
    if report.exists():report.unlink()
    proc=subprocess.run(['make','-s',target],capture_output=True,text=True)
    lines=[l for l in proc.stdout.splitlines() if l.startswith('{"cases"')]
    legacy=json.loads(lines[0]).get('mismatches') if lines else None
    if not report.exists():return ('aborted','no report'),legacy
    return verdict(json.loads(report.read_text())),legacy
src=pathlib.Path(f);orig=src.read_text()
subprocess.run(['make','-s',probe],check=True,capture_output=True)
(base,base_why),base_legacy=run();print('baseline',base,base_why[:100],flush=True)
res=[]
for name,a,b in M:
    assert a in orig,name
    try:
        src.write_text(orig.replace(a,b,1));time.sleep(1.1);os.utime(src,None)
        built=subprocess.run(['make','-s',probe],capture_output=True)
        if built.returncode:res.append((name,None,'does not compile'));print(name,'INVALID (does not compile)',flush=True);continue
        (v,why),legacy=run()
        caught=v=='aborted' or RANK[v]>RANK[base] or (v=='fail' and why!=base_why) or legacy!=base_legacy
        res.append((name,caught,f'{v}: {why[:100]}'));print(name,'caught' if caught else 'SURVIVED',v,why[:100],flush=True)
    finally:
        src.write_text(orig);time.sleep(1.1);os.utime(src,None);subprocess.run(['make','-s',probe],check=True,capture_output=True)
# The last mutant left its report in build/; rerun the oracle on the restored source so the report is real.
(v,why),_=run();print('restored',v,why[:100])
print('caught',sum(1 for _,c,_ in res if c),'of',sum(1 for _,c,_ in res if c is not None),'valid mutants;',sum(1 for _,c,_ in res if c is None),'did not compile')
