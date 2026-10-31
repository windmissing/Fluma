# T333 judgement 4: V1 archive adaptation rule (T327 contract §2 rule 2).
# V1 archives carry label-swapped keys: "sourceCommit" key holds physicsCommit
# and "physicsCommit" key holds sourceCommit. The adaptation rule must recover
# the contract values (header[5]=physicsCommit, header[6]=sourceCommit) from the
# T310 device archive without modifying it. Three frames, any layer.
import json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
d=json.loads((root/'test/reports/t310/cw-critical-1793341741239-1-frames.json').read_text(encoding='utf-8'))
ns=json.loads(d['nativeSamples'])
assert ns['revision']=='CW_C_DIAG_V1',ns['revision']
passed=0
frames=sorted({s['frame'] for s in ns['samples']})
assert len(frames)==3,frames
for frame in frames:
    sample=next(s for s in ns['samples'] if s['frame']==frame and s['layer']==2)
    # header contract ground truth (same for all sampled frames in the run)
    h5=float(d['headers']['5']);h6=float(d['headers']['6'])
    # Adaptation rule: V1 "sourceCommit" key actually holds physicsCommit (h[5]);
    # V1 "physicsCommit" key actually holds sourceCommit (h[6]).
    adapted_physics=sample['sourceCommit']
    adapted_source=sample['physicsCommit']
    assert adapted_physics==h5,(frame,adapted_physics,h5)
    assert adapted_source==h6,(frame,adapted_source,h6)
    # Cross-check against the critical TERMINAL ledger side: l[16]=r.sourceCommit
    l16=float(d['ledgers']['16'])
    assert adapted_source==l16,(frame,adapted_source,l16)
    # And the token band [4],[5] carries h[5],h[6] in contract order.
    tok=sample['token'].split('|')
    assert float(tok[4])==h5 and float(tok[5])==h6,(frame,tok[4],tok[5])
    passed+=1
print(json.dumps({'revision':'T333_V1_ARCHIVE_ADAPTATION_V1','passed':passed,'failed':0,'frames':frames,'device':'NOT_RUN'}))
