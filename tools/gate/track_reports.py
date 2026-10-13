#!/usr/bin/env python3
"""Copy oracle reports from build/oracles/ into the tracked reference/oracle-reports/ as summaries.

Full reports can embed every mismatching case (tens of MB); the repository keeps a copy with
every list longer than KEEP cut to its first KEEP entries, plus `tracked_summary` giving the
full file's sha256 and byte size and which lists were cut, so the verdict and the provenance
survive but the bulk does not. Side files (`<slug>.cases.bin`, …) are not tracked.

Usage: track_reports.py [slug ...]   (no slugs = every build/oracles/<slug>.json)"""
import hashlib,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[2]
KEEP=10
ROWS=('control_words','controls','suffix_profiles','results','runs','by_entry')  # per-control-word summaries: never cut (verdict.py sums them)
def cut(node,path,cuts):
    if isinstance(node,list):
        if len(node)>KEEP and path.split('.')[-1] not in ROWS:cuts[path]=len(node);node=node[:KEEP]
        return [cut(v,f'{path}[{i}]',cuts) for i,v in enumerate(node)]
    if isinstance(node,dict):return {k:cut(v,f'{path}.{k}' if path else k,cuts) for k,v in node.items()}
    return node
def main(argv):
    src=ROOT/'build/oracles';dst=ROOT/'reference/oracle-reports';dst.mkdir(exist_ok=True)
    files=[src/f'{s}.json' for s in argv[1:]] or sorted(p for p in src.glob('*.json') if p.name.count('.')==1)
    n=0
    for f in files:
        if not f.exists():print(f'missing: {f}');continue
        raw=f.read_bytes();d=json.loads(raw);cuts={}
        out=cut(d,'',cuts)
        out['tracked_summary']={'full_sha256':hashlib.sha256(raw).hexdigest(),'full_bytes':len(raw),'lists_cut_to':KEEP,'cut_lists':cuts}
        (dst/f.name).write_text(json.dumps(out,indent=1)+'\n');n+=1
    print(f'{n} reports tracked in {dst.relative_to(ROOT)}')
    return 0
if __name__=='__main__':sys.exit(main(sys.argv))
