"""T614 authoritative engineering table revision; no product execution."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parents[1];D=R/'docs'
def get(n):return json.loads((D/n).read_text(encoding='utf-8-sig'))
def put(n,v):(D/n).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def receipt(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
fields=['physicalPlanned','auxPlanned','physicalSealed','auxSealed','pending','pendingPhysicalStart',
 'pendingAuxStart','firstError','auxWriteCursor','entryPhysicalStart','entryAuxStart','entryValid',
 'entryNonceLo','entryNonceHi','entryRole','entryBranchEpochLo','entryBranchEpochHi','entryRevision']
fields+=['reserved_'+str(i) for i in range(18,31)]+['entrySeal']
raw=dict(words=32,revision=2,fields=fields,usedMaskU32=0x8003ffff,entrySeal=3235775745,
 entryValid=1,immutableWords=list(range(9,18))+[31],zeroWords=list(range(18,31)),mutableWords=list(range(9)),
 identity='uNonce/uM3Role/GPU role descriptor words5,6',
 replay='reset mutable0..8 only; verify identity/start and retain immutable entry')
c=dict(schema='T614_V5_SCOPE_ENTRY_CURSOR_V1',date='2026-11-18',task='T614',
 status='ENGINEERING_READY_PRODUCT_NOT_IMPLEMENTED_NOT_VALIDATED',rawCursor=raw,
 capture=dict(clearAt='existing SCOPE_INIT enum9 owner lane',
 initializeAt='v5BeginScope in INPUT_E_REQUEST enum15 before first credit',
 entryCopyAfter='INPUT_E_REQUEST dispatch plus API barrier, not SCOPE_INIT',
 finalCopyAfter='SCOPE_FINALIZE before reuse',rawWordStart=768,perRoleWords=64,
 entryOffset=0,finalOffset=32,endWord=1088,publicWordStart=768,publicPerRoleWords=32,
 publicEndWord=928,publicZeroUntilWord=2048,copiesPerPlan=40,copyBytesPerPlan=5120,
 extraGPUBytes=0,extraDispatches=0,extraHotMapBytes=0),
 publicWire=dict(payload='GRS_M3B_LINEAGE_COLD_V5_2',headerRevision=2,pageRevision=2,
 MetaMagic=894848323,MetaRevision=2,MetaSeal=3235775745,MetaUsedMaskU32=0x8007ffff,segmentRevisionDefault=1,
 FULL_CONTROL_ROOT_0to4_Revision=2,ROOT_CONTROL_Revision=1,prefixRevision=2,
 metaStartMap={'physicalStart':9,'auxStart':10},
 metaEndMap={'physicalPlannedEnd':0,'physicalSealedEnd':2,'auxPlannedEnd':1,'auxSealedEnd':3},
 complete='entry/final/control same nonce/role/branch; pending0; both planned==sealed; firstError0; actual finalize',
 missing='META_INCOMPLETE/FORENSIC, not guessed NOT_EXECUTED',
 serialize='copy first768 control words, build5 Meta32, zero928..2047; raw retention is never public'),
 inputs=[receipt('entry/src/main/cpp/gpu_resident/m3b/'+n) for n in
 ['lineage_events.glsl','lift_writer.glsl','profiles_generated.h','quantum_v5.comp','transport.cpp']],
 checks=dict(controlEndBytes=1088*4,controlCapacityBytes=8192,perPlanCopies=4*5*2,
 perPlanCopyBytes=4*5*2*32*4,productTestsRun=False,deviceUsed=False))
assert len(fields)==32 and c['checks']['controlEndBytes']<=8192
assert c['checks']['perPlanCopies']==40 and c['checks']['perPlanCopyBytes']==5120
assert raw['usedMaskU32']==sum(1<<i for i in list(range(18))+[31])
put('architecture-T614-v5-entry-cursor-v1.json',c)
pn='architecture-T527-v5-cold-manifest-v1.json';wn='architecture-T552-v5-segment-wire-v2.json'
p=get(pn);w=get(wn)
before=[[(d['fixed']['byteOffset'],d['fixed']['byteLength']) for d in x['fullDescriptors']] for x in w['profiles']]
p['scopeCursorCaptureContract']=dict(task='T614',path='docs/architecture-T614-v5-entry-cursor-v1.json',
 rawRevision=2,metaRevision=2,privateWords=32,privateUsedMaskU32=0x8003ffff,
 FULLControlSegmentRevision=2,ROOTControlSegmentRevision=1,prefixAndHeaderUnchanged=True)
if not any(x.get('task')=='T614' for x in p['revisionHistory']):
 p['revisionHistory'].append(dict(task='T614',engineeringRevision=3,
 change='immutable scope entry, Meta2 and FULL CONTROL segmentRevision2; header2/payloadV5_2/capacity unchanged'))
put(pn,p)
w['engineeringRevision']=3;w['segmentRevisionExceptions']={'FULL:CONTROL_ROOT_0..4':2}
w['cursorCapture']['revision']=2;w['cursorCapture']['dictionary']='T614_CAPTURE_SCOPE_META_V2'
w['cursorCapture']['rawCursor32']=raw
w['cursorCapture']['cursorWords'].update(physicalEntry=9,auxEntry=10,entryValid=11,entryRevision=17,entrySeal=31)
w['cursorCapture']['capturePoints']=[c['capture']['entryCopyAfter'],c['capture']['finalCopyAfter']]
w['cursorCapture']['rawClearPoint']=c['capture']['clearAt']
w['cursorCapture']['internalReplay']=raw['replay'];w['cursorCapture']['publicSerialization']=c['publicWire']['serialize']
for x in w['profiles']:
 for d in x['fullDescriptors']:
  if d['name'].startswith('CONTROL_ROOT_'):d['fixed']['segmentRevision']=2
w['parent']=receipt('docs/'+pn)
after=[[(d['fixed']['byteOffset'],d['fixed']['byteLength']) for d in x['fullDescriptors']] for x in w['profiles']]
assert before==after and all(len(x['fullDescriptors'])==61 for x in w['profiles'])
put(wn,w)
print(json.dumps(dict(task='T614',FULLSegments=[len(x['fullDescriptors']) for x in w['profiles']],
 controlRevision=2,offsetsUnchanged=True,copies=40,bytes=5120,testsRun=False)))
