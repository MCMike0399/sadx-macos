"""Execute a finite set of unchanged Gamma, matrix, animation and Sonic judges."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from verify_packet import ROOT, OUT, run
SLUGS=['gamma_cursors','gamma_setup_resource','gamma_effects','gamma_input_buffer',
       'gamma_selection_core','model_matrices','player_vertex_weld','sonic_model',
       'player_animation','player_animation_blend','sonic_player']

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--start-at',choices=SLUGS,default=SLUGS[0]);parser.add_argument('--summarize-only',action='store_true');args=parser.parse_args()
    prior=OUT/'regression-before';prior.mkdir(exist_ok=True)
    for slug in SLUGS:
        path=Path('/Users/burbujamc/Developer/sadx-macos/build/oracles')/(slug+'.json')
        target=prior/(slug+'.json')
        if not target.exists():
            if path.exists(): target.write_bytes(path.read_bytes())
            else: target.write_bytes((ROOT/'reference/oracle-reports'/(slug+'.json')).read_bytes())
    if not args.summarize_only: run('build-regression-probes',['make','-j2','-o','build/native-controller-deps/include/SDL3/SDL.h',
        'INCLUDES=-Ibuild/gamma-fan-native/include -Isrc -Itests/fixtures',
        *['build/native-'+s.replace('_','-')+'-probe.dylib' for s in SLUGS]])
    spec=__import__('importlib.util',fromlist=['']).spec_from_file_location('oracle_verdict',ROOT/'tools/oracles/verdict.py')
    verdict_module=__import__('importlib.util',fromlist=['']).module_from_spec(spec);spec.loader.exec_module(verdict_module)
    make=(ROOT/'build/oracles.mk').read_text().splitlines();results=[]
    for slug in SLUGS:
        entry=next(i for i,l in enumerate(make) if l.startswith('oracle-'+slug.replace('_','-')+':'))
        recipe=make[entry+1].strip().replace('$(REFERENCE_PYTHON)','env PYTHONPATH=tools/lib:tools/oracles reference/python/bin/python').replace('$(GAME_EXE)','/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe')
        argv=shlex.split(recipe)
        if not args.summarize_only and SLUGS.index(slug)>=SLUGS.index(args.start_at):
            rc=run('regression-'+slug,argv,(0,2))
        else: rc=int((OUT/('regression-'+slug+'.exit')).read_text())
        before=json.loads((prior/(slug+'.json')).read_text());after=json.loads((ROOT/'build/oracles'/(slug+'.json')).read_text())
        # Binary identities differ after native static relinking. Every remaining
        # source observation, count and individual partial identity is compared.
        ignored={'native_library_sha256','source_path','native_library_path','elapsed_seconds','duration_seconds','seconds','native_probe_sha256','native_sources'}
        raw_baseline=(Path('/Users/burbujamc/Developer/sadx-macos/build/oracles')/(slug+'.json')).exists()
        keys=(set(before)|set(after)) if raw_baseline else set(before)&set(after)
        drift=[k for k in keys if k not in ignored and before.get(k)!=after.get(k)]
        results.append({'slug':slug,'exit_status':rc,'drift':drift,
            'before_sha256':hashlib.sha256((prior/(slug+'.json')).read_bytes()).hexdigest(),
            'after_sha256':hashlib.sha256((ROOT/'build/oracles'/(slug+'.json')).read_bytes()).hexdigest(),
            'verdict':verdict_module.verdict(after),
            'semantic_counters':{k:v for k,v in after.items() if isinstance(v,(int,bool))},
            'metadata_differences':{k:{'before':before.get(k),'after':after.get(k)} for k in ignored if before.get(k)!=after.get(k)},
            'unchanged_base_source_fingerprints':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in after.get('native_sources',{})},
            'baseline_kind':'raw' if raw_baseline else 'tracked-summary-counts-only',
            'individual_partial_lists':{k:{'retained':v==after.get(k),'count':len(v),'sha256':hashlib.sha256(json.dumps(v,sort_keys=True).encode()).hexdigest()} for k,v in before.items() if isinstance(v,list) and ('partial' in k or 'mismatch' in k)},
            'original_keys':sorted(before)})
    import subprocess
    for result in results:
        for path,value in result['unchanged_base_source_fingerprints'].items():
            base=subprocess.check_output(['git','show','bdb61450:'+path],cwd=ROOT)
            assert hashlib.sha256(base).hexdigest()==value,path
    (ROOT/'reference/packets/gamma-fan-native/REGRESSIONS.json').write_text(json.dumps(results,indent=2)+'\n')
    assert not any(r['drift'] for r in results),[(r['slug'],r['drift']) for r in results if r['drift']]
if __name__=='__main__':main()
