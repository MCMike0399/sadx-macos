from pathlib import Path
import json,subprocess,hashlib
rows=[]
for slug in ['game_winmain','game_reset','game_loop']:
    old=json.loads(subprocess.check_output(['git','show',f'8a03588:reference/oracle-reports/{slug}.json']))
    raw=Path(f'build/oracles/{slug}.json').read_bytes();current=json.loads(raw)
    metadata=['native_library_sha256']
    if current.get('source_files') != old.get('source_files'):
        for name,digest in current['source_files'].items():
            historical_source=subprocess.check_output(['git','show',f'8a03588:{name}'])
            assert hashlib.sha256(historical_source).hexdigest()==digest,(slug,name)
        metadata.append('source_files')
    assert all(key in current and key in old for key in metadata)
    for key in metadata:current[key]=old[key]
    restored=json.dumps(current,indent=1)+'\n'
    digest=hashlib.sha256(restored.encode()).hexdigest()
    assert digest==old['tracked_summary']['full_sha256'],(slug,digest,old['tracked_summary']['full_sha256'])
    rows.append(dict(slug=slug,full_raw_sha256=hashlib.sha256(raw).hexdigest(),historical_full_sha256=digest,metadata_restored=metadata,cases=current['cases'],passed=True))
Path('reference/production-integration-20261016/startup-initial-161-historical-oracles.json').write_text(json.dumps(dict(passed=True,reports=rows,limits='Full raw JSON reconstructed with only native library identity and already-stale provenance hashes restored (current sources independently equal HEAD8a03588); no test values/flags removed or changed.'),indent=1)+'\n')
print(rows)
