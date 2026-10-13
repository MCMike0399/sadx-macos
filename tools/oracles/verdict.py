#!/usr/bin/env python3
"""The one place that reads an oracle comparison report and says what it proves.

Reports were written by 90+ scripts over many sessions and do not share a schema. This module
classifies every result-bearing key by name so the three verdicts are computed the same way
for all of them:

  pass     every compared value/state/byte/boundary matches and no flag difference is reported
  partial  every compared value matches but FP status / exception flags (x87 status word, MXCSR)
           differ; the report's `limits` must say why (Unicorn 2.1.4 raises no SSE flags,
           applies the x87 NaN rule to SSE operands, FLD of an SNaN is not the hardware spec)
  fail     a compared value differs, a value-class match flag is false, or nothing recognisable

Key classes (top level and inside per-control-word rows such as `control_words`, `controls`,
`suffix_profiles`, `results`, `runs`, `by_entry`):
  * counters `*_mismatches` / `mismatch_count` / `mismatched_words`: flag-class when the name
    mentions flag, mxcsr, exception or status; value-class otherwise
  * booleans `*_match` / `*_matches` / `*_passed`: flag-class when the name mentions flag,
    emulator, strict, exception, mxcsr or status; value-class otherwise
  * `passed`: strict claim — True is a pass; False with all values matching is partial
  * a bare `mismatches` list/number is value-class unless the report's own value-class
    evidence (counters at 0, booleans true) says the values matched, in which case it is the
    per-case list of flag differences

New oracles should write `passed` (strict), `values_match` (bool) and, when flags are compared,
a `flag_mismatches` counter so no new case is needed here. Usage: verdict.py REPORT... prints
one line per report; exit 1 if any report fails. Side files (`<slug>.<x>.json`) are skipped."""
import json,pathlib,re,sys
FLAGGY=re.compile(r'flag|mxcsr|exception|status|emulator|strict')
ROWS=('control_words','controls','suffix_profiles','results','runs','by_entry')
def count(v,path='',cuts=None):
    """Length of a list, honouring the original length recorded by tools/gate/track_reports.py."""
    if isinstance(v,(list,dict)):return cuts[path] if cuts and path in cuts else len(v)
    return int(v) if isinstance(v,(int,float,bool)) else 0
def classify(d,acc,prefix='',cuts=None):
    for k,v in d.items():
        if k=='mismatches':continue
        if (k.endswith('_mismatches') or k in('mismatch_count','mismatched_words')) and isinstance(v,(int,float,list,dict)):
            acc['flag' if FLAGGY.search(k) else 'value'][k]=acc['flag' if FLAGGY.search(k) else 'value'].get(k,0)+count(v,prefix+k,cuts)
        elif isinstance(v,bool) and (k.endswith('_match') or k.endswith('_matches') or k.endswith('_passed')) and k!='passed':
            acc['flag_bool' if FLAGGY.search(k) else 'value_bool'][k]=acc['flag_bool' if FLAGGY.search(k) else 'value_bool'].get(k,True) and v
def verdict(d):
    """Return (verdict, reason) for a parsed report dict."""
    if not isinstance(d,dict):return 'fail','not a report object'
    acc={'value':{},'flag':{},'value_bool':{},'flag_bool':{}}
    cuts=(d.get('tracked_summary') or {}).get('cut_lists') or {}
    classify(d,acc,'',cuts)
    for rk in ROWS:
        rows=d.get(rk)
        if isinstance(rows,list):
            for i,r in enumerate(rows):
                if isinstance(r,dict):classify(r,acc,f'{rk}[{i}].',cuts)
    value_fail=[f'{k}={n}' for k,n in acc['value'].items() if n]+[f'{k}=false' for k,ok in acc['value_bool'].items() if not ok]
    flag_fail=[f'{k}={n}' for k,n in acc['flag'].items() if n]+[f'{k}=false' for k,ok in acc['flag_bool'].items() if not ok]
    values_known=bool(acc['value'] or acc['value_bool'])
    if 'mismatches' in d:
        n=count(d['mismatches'],'mismatches',cuts)
        if n and not(values_known and not value_fail):value_fail.append(f'mismatches={n}')
        elif n:flag_fail.append(f'mismatches={n} (per-case flag list)')
        values_known=True
    if value_fail:return 'fail',', '.join(value_fail)
    if 'passed' in d and d['passed'] and not flag_fail:return 'pass','passed'
    if flag_fail:return 'partial','values match; '+', '.join(flag_fail)+' (see limits)'
    if 'passed' in d and not d['passed']:
        return ('partial','values match; passed=false (strict claim; see limits)') if values_known else ('fail','passed=false with no compared values')
    if values_known or 'passed' in d:return 'pass',', '.join(f'{k}=0' for k in acc['value']) or ', '.join(acc['value_bool']) or 'passed'
    return 'fail','no recognised result key'
def main(argv):
    worst=0
    for path in argv[1:]:
        if pathlib.Path(path).name.count('.')>1:continue  # side file such as <slug>.mismatches.json
        try:d=json.load(open(path))
        except Exception as e:v,why='fail',f'unreadable: {e}'
        else:v,why=verdict(d)
        print(f'{v:8} {path}: {why}')
        if v=='fail':worst=1
    return worst
if __name__=='__main__':sys.exit(main(sys.argv))
