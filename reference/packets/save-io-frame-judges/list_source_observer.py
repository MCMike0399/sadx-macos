"""Separate known source fixture node identities from unknown sentinel payload.

This supplements, never changes, the unchanged list comparator. Raw sentinel
bytes are allocator-fixture poison, not a recovered source default. Link identity
is known independently. No original CRT allocation-failure or reentry guarantee.
"""
import hashlib
import json
from pathlib import Path
import sys
sys.path[:0]=['tools/lib','tools/oracles']
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_save_file_list import Source,cases

exe,folder=sys.argv[1:];folder=Path(folder);folder.mkdir(parents=True,exist_ok=True)
image=PEImage(exe);assert image.sha256==SOURCE_SHA256
source=Source(image);rows=[]
# Use complete inherited default cohort, preserving every stop and selection.
for index,v in enumerate(cases(505050,256)):
    expected=source.run(v)
    nodes=[]
    for pointer in source.nodes():
        raw=bytes(source.m.mem_read(pointer,16))
        nodes.append({'fixture_identity':pointer,'raw_fixture_bytes':raw.hex(),
                      'name_time_known':bool(nodes),'next_identity':int.from_bytes(raw[12:16],'little')})
    rows.append({'case':index,'input':v,'source_nodes':nodes,
                 'source_output_sha256':hashlib.sha256(b''.join(x.to_bytes(4,'little') for x in expected)).hexdigest()})
raw=folder/'list-identities.json';raw.write_text(json.dumps(rows,separators=(',',':'))+'\n')
report={'cases':len(rows),'raw_sha256':hashlib.sha256(raw.read_bytes()).hexdigest(),'raw_bytes':raw.stat().st_size,'limits':__doc__}
(folder/'list-identity-receipt.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
