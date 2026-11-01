"""Compare trusted local ABI streams by every record, not aggregate verdicts.

No normalization of native words, failures or unknown tags is permitted. Only
source-return filenames are reduced to basenames to permit isolated checkout replay.
"""
import gzip
import itertools
import pickle
from pathlib import Path
import sys

def records(path):
    with gzip.open(path,'rb') as stream:
        while True:
            try: row=pickle.load(stream)
            except EOFError: return
            if row[0]=='source-return': row=(row[0],Path(row[1]).name,*row[2:])
            yield row

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
        print('identity drift/reordering/truncation detected')
    else:
        result=compare(records(sys.argv[1]),records(sys.argv[2]))
        print('identical' if result is None else f'first different record: {result}')
        raise SystemExit(int(result is not None))
