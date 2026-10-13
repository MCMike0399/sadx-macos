"""Mutation-test an oracle: apply each (name, old, new) edit from MUTANTS.json to SRC, rebuild PROBE, run the
make TARGET and report how many mismatches each mutant produced. Usage: mutate.py SRC TARGET PROBE MUTANTS.json
Run from the repository root."""
import os,pathlib,subprocess,time,json,sys
f,target,probe=sys.argv[1],sys.argv[2],sys.argv[3];M=json.loads(open(sys.argv[4]).read())
res=[]
src=pathlib.Path(f);orig=src.read_text()
for name,a,b in M:
    assert a in orig,name
    try:
        src.write_text(orig.replace(a,b,1));time.sleep(1.1);os.utime(src,None)
        subprocess.run(['make','-s',probe],check=True,capture_output=True)
        proc=subprocess.run(['make','-s',target],capture_output=True,text=True)
        lines=[l for l in proc.stdout.splitlines() if l.startswith('{"cases"')]
        n=json.loads(lines[0])['mismatches'] if lines else 'oracle aborted'
        res.append((name,n));print(name,n,flush=True)
    finally:
        src.write_text(orig);time.sleep(1.1);os.utime(src,None);subprocess.run(['make','-s',probe],check=True,capture_output=True)
# The last mutant left its report in build/; rerun the oracle on the restored source so the report is real.
subprocess.run(['make','-s',target],check=False)
print('caught',sum(1 for _,n in res if n),'of',len(res))
