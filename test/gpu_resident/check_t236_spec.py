"""Check the accepted T241 handoff; preserve the historical T236 gap evidence."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=ROOT/'docs/analysis-T233-gpu-resident-M0-engineering-manifest-v1.json';m=json.loads(p.read_text(encoding='utf-8-sig'))
for row in m['resourceTemplates']:
 k=row['K'];assert row['partsBytes']['two_index_arrays']==8*(256 if k==192 else 512)
assert len(m['eventKinds'])==7 and m['wire']['revisionWord']==1
r={'revision':'T236_T241_HANDOFF_CHECK_V2','manifestSha256':hashlib.sha256(p.read_bytes()).hexdigest(),'K192DoublePadBytes':2048,'productWireDefined':True,'eventKinds':7,'historicalGaps':'RESOLVED_BY_T241','newQuestion':'MAX_ID terminal next-ID exhaustion representation; see original task comment','device':False}
out=ROOT/'test/reports/t236/resume';out.mkdir(parents=True,exist_ok=True);(out/'spec-check.json').write_text(json.dumps(r,indent=2),encoding='utf-8');print(json.dumps(r,indent=2))
