"""Reproduce T233 handoff questions without modifying architect-owned specs."""
import json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=ROOT/'docs/analysis-T233-gpu-resident-M0-engineering-manifest-v1.json';m=json.loads(p.read_text(encoding='utf-8-sig'))
values=list(range(192))+[2**32]*64;first=None
for k in [2**n for n in range(1,9)]:
 j=k//2
 while j:
  next_values=values.copy()
  for i in range(256):
   other=i^j
   if other>i:
    lo,hi=sorted([values[i],values[other]])
    next_values[i],next_values[other]=(lo,hi) if i&k==0 else (hi,lo)
  values=next_values;live=[i for i in range(192,256) if values[i]<2**32]
  if first is None and live:first={'k':k,'j':j,'paddingIndex':live[0],'liveId':values[live[0]],'liveCountInPadding':len(live)}
  j//=2
assert values==list(range(192))+[2**32]*64
r={'revision':'T236_T233_SORT_LAYOUT_QUESTION_V1','K':192,'Kpad':256,'manifestTwoIndexBytes':m['resourceTemplates'][0]['partsBytes']['two_index_arrays'],'twoPaddedIndexBytes':2048,'differenceBytes':512,'firstLiveInPadding':first,'note':'Out-of-place double-pad layout needs correction or explicit alternative. In-place/disjoint-pair or repacked workspace might fit existing total; architecture decision required, not proof GPU infeasible.'}
out=ROOT/'test/reports/t236';out.mkdir(parents=True,exist_ok=True);(out/'sort-storage-gap.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
report={'revision':'T236_ENGINEERING_HANDOFF_QUESTIONS_V1','manifestSha256':hashlib.sha256(p.read_bytes()).hexdigest(),'status':'NEEDS_ARCHITECT_WIRE_AND_SORT_LAYOUT_DECISION','event64PresentKeys':list(m['layouts']['event64']),'topLevelPresentKeys':list(m),'requiredClarifications':['Numeric magic/revision/seal/enum values for rootKind,outcome,terminal status/reason,source kind','event kind/flags/phaseSourceState encoding and per-kind used/zero masks promised by T233 section 2','completion outcome semantics for source SUCCESS/PARTIAL, normal REJECTED, dedup REPLAY, fatal ERROR including which receipt fields retain original values','K192 sorting index storage: extra 512B double-pad or approved in-place/repacked layout and corresponding pass/barrier costs'],'notBlocked':['Generated field indexes, checked byte ranges and resource template accounting','Standalone C++/GLSL uint primitives and independent raw-bit differential','High precision cold angle generation and reproducible SHA','Transport-independent owner completion/epoch/tombstone/lease control scaffold with explicit TEST policy'],'blockedImplementation':['Frozen product source/event/replay shader wire and full step stage integration','Native completion transport and publication/renderer integration using approved product wire'],'device':False,'M0Product':'NOT_IMPLEMENTED','CPUFallback':False}
(out/'handoff-questions.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
