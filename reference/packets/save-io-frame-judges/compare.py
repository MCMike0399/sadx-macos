"""Compare trusted local ABI streams by every record, not aggregate verdicts.

No normalization of defined native values, failures or unknown tags is permitted.
Source filenames reduce to basenames for checkout replay. For explicitly declared
void exports only, the inherited ctypes default int result is an unspecified host
register, not an ABI output. Preserve it in raw capture, compare an unknown tag.
"""
import gzip
import itertools
import pickle
from pathlib import Path
import sys

VOID_EXPORTS = frozenset({
    'sadx_native_save_file_list_probe', 'sadx_native_load_selected_save_probe',
    'sadx_native_save_list_free_probe', 'sadx_native_file_select_controls_probe',
})

def defined_record(row):
    if row[0]=='source-return': return (row[0],Path(row[1]).name,*row[2:])
    if row[0]=='native' and row[1] in VOID_EXPORTS:
        return (*row[:3],('unknown','declared-void-return-register'),row[4])
    return row

def records(path):
    with gzip.open(path,'rb') as stream:
        while True:
            try: row=pickle.load(stream)
            except EOFError: return
            yield defined_record(row)

def compare(left,right):
    sentinel=object()
    for index,(a,b) in enumerate(itertools.zip_longest(left,right,fillvalue=sentinel)):
        if a!=b: return index
    return None

if __name__=='__main__':
    if sys.argv[1:]==['--self-test']:
        baseline=[('case',0,[1,2]),('case',1,[3,4])]
        # Same number of cases and same aggregate values; case identities drift.
        assert compare(baseline,list(reversed(baseline)))==0
        assert compare(baseline,baseline) is None
        assert compare(baseline,baseline[:1])==1
        a=('native','sadx_native_save_list_free_probe',[b'input'],123,[b'output'])
        b=(*a[:3],456,a[4])
        assert defined_record(a)==defined_record(b)
        c=(*b[:4],[b'changed-output'])
        assert defined_record(a)!=defined_record(c)
        x=('native','sadx_native_save_frame',[b'input'],1,[b'output'])
        y=(*x[:3],0,x[4])
        assert defined_record(x)!=defined_record(y)
        print('identity drift/reordering/truncation/defined-return checks pass; void remains unknown')
    else:
        result=compare(records(sys.argv[1]),records(sys.argv[2]))
        print('identical' if result is None else f'first different record: {result}')
        raise SystemExit(int(result is not None))
