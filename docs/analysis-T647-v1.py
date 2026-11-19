"""T647 finite engineering table/source review; no physics stepper or product execution."""
from pathlib import Path
import json,hashlib,sys
R=Path(__file__).resolve().parents[1]
def get(p):return json.loads((R/p).read_text(encoding='utf-8-sig'))
def fp(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def emit(p,v):(R/p).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def magic(s):return int.from_bytes(s.encode(),'little')
def build():
 r1=get('docs/architecture-T524-g9-spf-manifest-v1.json');dm=get('docs/architecture-T554-g9-dm-manifest-v2.json')
 v=get('docs/architecture-T634-composite-closure-v1.json');rx=get('docs/design-T637-manifest-v1.json')
 d=get('docs/architecture-T554-g9-dm-dispatch-v2.json');vr=get('docs/architecture-T563-g11-r1-dispatch-v1.json');vd=get('docs/architecture-T563-g11-dm-dispatch-v1.json')
 assert len(d['source'])==12 and len(d['quantum'])==2404
 def count_table(m):
  # T563 tables explicitly separate source and quantum arrays.
  return len(m['source']),len(m['quantum'])
 assert count_table(vr)==(19,2543) and count_table(vd)==(19,2923)
 families=[('R1_DEFAULT',r1['profileCode'],r1['profileCode'],12,2024,65535,65535,r1['identities']['completion']['magic'],1,r1['seal']),
  ('DM_DEFAULT',dm['profileCode'],dm['profileCode'],12,2404,65535,262143,dm['identity']['completionMagic'],2,dm['identity']['seal']),
  ('G11_R1',r1['profileCode'],magic('G11R'),19,2543,0xcffff,0xcffff,magic('G11C'),2,0xc0de6471),
  ('G11_DM',dm['profileCode'],magic('G11D'),19,2923,0xcffff,0xfffff,magic('G11C'),2,0xc0de6471),
  ('G9_RX',rx['profileCode'],rx['profileCode'],12,2404,65535,262143,dm['identity']['completionMagic'],2,dm['identity']['seal'])]
 rows=[]
 for name,parent,tail,sc,q,sm,qm,cm,rev,seal in families:
  for kind,n,mask in [(1,sc,sm),(2,q,qm)]:rows.append(dict(family=name,rootKind=kind,physicalProfileCode=parent,tailProfileCode=tail,actualPasses=n,requiredAudits=mask,completionMagic=cm,tailRevision=rev,seal=seal,words=256,tailReadBytes=1024,quantumDelta=int(kind==2),visualTickDelta=256*int(kind==2)))
 assert all((r['requiredAudits']&65535)==65535 for r in rows)
 assert next(r for r in rows if r['family']=='G11_DM' and r['rootKind']==2)['requiredAudits']==0xfffff
 ext={156:'expectedContractRevision1',157:'parentKind_1R1_2DM',158:'physicalProfileCode',159:'compositeProfileCode',
  160:'staticContractSHA',168:'visualConfigSHA',176:'visualSeedRecipeSHA',184:'physicalPipelineSpecSHA',192:'compositeLayoutSHA',200:'compositePipelineSHA',
  208:'baseVisualGeneration',210:'proposedVisualGeneration',212:'baseAcceptedVisualTicks',214:'proposedAcceptedVisualTicks',216:'physicalSeedSHA',224:'actualVisualSeedSHA',232:'frozenExpectedSHA'}
 fields=[]
 for at,name in ext.items():
  words=8 if name.endswith('SHA') else 2 if 208<=at<216 else 1
  fields.append(dict(start=at,words=words,name=name))
 assert sorted(i for f in fields for i in range(f['start'],f['start']+f['words']))==list(range(156,240))
 auditNames=get('docs/design-T515-three-state-manifest-v1.json')['audits']
 expected=[]
 head=['magicGREX','revision1','familyCode','rootKind','tailMagic','tailRevision','tailProfileCode','physicalProfileCode','seal','tailWords','actualPasses','requiredAudits','stateBytesLo','stateBytesHi','workBytesLo','workBytesHi','eventCapacity','pathCapacity']
 for i,n in enumerate(head):expected.append(dict(start=i,words=1,name=n))
 for i,n in enumerate(['nonce','plan','frame','control','geometry','deadlineNs','rootBeginNs','baseCommit','baseQuanta','arenaGeneration']):expected.append(dict(start=18+2*i,words=2,name=n))
 for i,n in enumerate(['baseBank','candidateBank','arena','ordinal','K','V']):expected.append(dict(start=38+i,words=1,name=n))
 expected.append(dict(start=44,words=18,name='scope9U64_run_owner_context_inputEpoch_pageSession_permission_window_config_surface'))
 for i,n in enumerate(['baseVisualGeneration','baseVisualTicks','proposedVisualGeneration','proposedVisualTicks']):expected.append(dict(start=62+2*i,words=2,name=n))
 expected.append(dict(start=70,words=2,name='reservedZero'))
 for i,n in enumerate(['parameterSHA','physicalSeedSHA','physicalLayoutSHA','actualShaderSHA','physicalPipelineSpecSHA','compositeLayoutSHA','activePipelineSHA','visualConfigSHA','visualSeedRecipeSHA','actualVisualSeedSHA','staticContractSHA','inputSHA','inputPolicySHA']):expected.append(dict(start=72+8*i,words=8,name=n))
 for i,n in [(176,'clockGeneration'),(178,'lifecycleGeneration')]:expected.append(dict(start=i,words=2,name=n))
 for i,n in [(180,'inputDomain'),(181,'physicalParameterRevision'),(182,'implementationRevision1'),(183,'parentKind')]:expected.append(dict(start=i,words=1,name=n))
 expected.append(dict(start=184,words=40,name='reservedZero'))
 assert sorted(i for f in expected for i in range(f['start'],f['start']+f['words']))==list(range(224))
 table=dict(schema='T647_TYPED_ACCEPTANCE_CONTRACT_V1',date='2026-11-19',deviceUsed=False,productTestsRun=False,productImplemented=False,
  rows=rows,rootKind='SOURCE1/QUANTUM2 explicit; never inferred from pass count',factory='native trusted pipeline identity + generated table + active immutable permit; no caller arbitrary counts/masks',
  CommonFields='T524 completion words0..155 exact semantic predicates plus new family identity; no normalization of count/mask/revision/profile',
  defaultReservedZero=[156,256],compositeExtension=fields,compositeReservedZero=[240,256],
  FrozenExpected224=expected,expectedMagic=magic('GREX'),expectedRevision=1,expectedWords=224,expectedBytes=896,
  encoding='U32 LE, U64 low/high, SHA256 digest bytes packed4 LE; ranges half-open',familyCodes=dict(R1_DEFAULT=1,DM_DEFAULT=2,G11_R1=3,G11_DM=4,G9_RX=5),
  expectedHash='SHA256 of exact896 LE bytes, GREX magic; no self hash or raw tail; stream serialization, no extra full copy',
  expectedAbsent='non-composite visual fields/V zero; non-RX receipt clock/lifecycle/domain fields zero; expected fields derived only from trusted native scope/config, never raw tail',
  sourceDuplicateExpectation='FrozenExpected proposedVisualGeneration is candidate base+1; duplicate tail must instead equal base; frozenExpectedSHA stays immutable',
  G11ExpectedUpload=dict(controlByteStart=61312,descriptorWords=224,digestWord=15552,bytes=928,perRootUploadBytes=928,additionalBufferSubDataCalls=1,remainingReservedZero=[62240,65536],existingControlBytes=65536,noAddedResidentBytes=True),
  nativeEvidence=['actual own dispatch/barrier counters','real fenceNs with rootBegin<=fenceNs<=deadline','first original GL/EGL errors','active context/owner identity','mapped raw1024B exact bytes'],
  auditBits=[dict(bit=i,name=n) for i,n in enumerate(auditNames)]+[
   dict(bit=16,name='DM_maximal_q_source_prefix_actual_patch'),dict(bit=17,name='DM_private_predictor_corrector_single_force_time'),dict(bit=18,name='G11_PLAN_MAPPING'),dict(bit=19,name='G11_VIS_REPLAY_GLYPH')],
  DMControlWords=[dict(word=i,name=n) for i,n in enumerate(['expectedScopeMask','seenScopeMask','certified16ScopeMask','certified17ScopeMask','currentScopeId','requiredStageMask','visitedStageMask','auditProtocolRevision1'],120)],
  scopeBits=dict(discovery=[0,6],authority=6,rejectPrefix=[7,13],acceptedReplay=13),
  scopeRequirement='selected k: discovery bits0..k, rejected-prefix bits7..(7+k-1), authority6, replay13; blocked all discovery0..5 and rejected-prefix7..12; no authority/replay',
  DMWork='exact T554 appended128x48words; no uColumnStage+32 overlap',
  rootConfigDM=dict(fullConfig64=[64,128],DMParameters=[112,120]),rootConfigRX=dict(fullConfig64=[64,128],DMParameters=[76,84]),
  time=dict(physicalQuantumSeconds='1/120',restTickSeconds='1/3840',restTicksPerExecutedMicro='32/J',visualTicksPerAcceptedQuantum=256,sourceTicks=0,displayTargetFps=30,
   sourceDuplicate='no commit increment, no quanta or visual-generation increment',nonDuplicateSource='commit+1, quanta unchanged; combined visualGeneration+1',acceptedQuantum='commit+1, quanta+1; combined visualGeneration+1; visualTicks=checked256*quanta',blockedFaultLate='no published commit/quanta/ticks/visual generation'),
  proofs=['private constructor ParentValidatedAccepted','private constructor ParentValidatedBlocked','private constructor ValidatedCompositeAccepted','private constructor ValidatedCompositeBlocked'],
  noCpuPassFabrication=True,noAdditionalPhysicsDispatch=True,normalRootFences=1,maxComputeSSBO=4,deadlineNs=250000000,gapNs=33333333)
 inputs=['docs/developer-T639-v1.md','test/reports/t639/blocker-evidence.json','docs/algorithm-G9-dynamic-mobilization-v2.md','docs/architecture-T554-g9-dm-manifest-v2.json','docs/architecture-T554-g9-dm-dispatch-v2.json',
  'docs/architecture-T563-g11-manifest-v1.json','docs/architecture-T563-g11-r1-dispatch-v1.json','docs/architecture-T563-g11-dm-dispatch-v1.json',
  'entry/src/main/cpp/gpu_resident/g9dm/transport.cpp','entry/src/main/cpp/gpu_resident/g9dm/completion_gate.cpp','entry/src/main/cpp/gpu_resident/g9dm/pipeline.comp','entry/src/main/cpp/gpu_resident/g9dm/research_config.h',
  'entry/src/main/cpp/gpu_resident/g9spf/completion_gate.cpp','entry/src/main/cpp/gpu_resident/g11vis/visual_transport.cpp','entry/src/main/cpp/gpu_resident/g11vis/visual_transport.h',
  'docs/architecture-T634-composite-closure-v1.json','docs/architecture-T641-runtime-v1.json']
 saved=get('test/reports/t639/blocker-evidence.json')['sources'];agreement=[]
 for p,sourceReceipt in saved.items():agreement.append(dict(path=p,sameAsT639=fp(p)['sha256']==sourceReceipt['sha256']))
 assert all(x['sameAsT639'] for x in agreement)
 assert v['acceptanceEngineeringRevision']['tailSeal']==0xc0de6471
 assert v['acceptanceEngineeringRevision']['newVisualControlRegistration']['perRootUploadBytes']==928
 assert 61312+224*4+8*4==62240 and 62240<=65536
 assert get('docs/architecture-T641-runtime-v1.json')['acceptanceEngineeringRevision']['quantumAudits']==0x3ffff
 for selected in range(6):
  discovery=(1<<(selected+1))-1;rejected=(1<<selected)-1
  expectedScopes=discovery|(rejected<<7)|(1<<6)|(1<<13)
  assert expectedScopes.bit_count()==2*selected+3
 sources=[]
 needles=['?12:2024','?12:2404','uRequiredPasses==2024u','g9Set(17u,65535u)','uColumnStage+c*64u+52u','uColumnStage+c*64u+56u','uint dmWord','C[uConfig+','awaiting formal parent','P("uVisTicks"']
 for p in inputs[8:]:
  lines=(R/p).read_text(encoding='utf-8-sig').splitlines();matches=[dict(line=i+1,text=s) for i,s in enumerate(lines) if any(n in s for n in needles)]
  if matches:sources.append(dict(path=p,matches=matches))
 evidence=dict(schema='T647_FINITE_SOURCE_REVIEW_V1',date='2026-11-19',deviceUsed=False,productBuildsRun=False,productTestsRun=False,
  checks=['10 family/root exact contract rows','T55412/2404 and G11 19/2543/2923 actual formal tables','20 audit families and exact masks','composite tail156..239 no overlap,240..255zero','DM/RX distinct RootConfig parameter offsets','T639 source SHA matches current snapshot'],
  inputs=[fp(p) for p in inputs],blockerSourceAgreement=agreement,sourceMatches=sources,
  limits=['no validated GPU audit producer deployed','no runtime proof/physics/replay/performance/device PASS','DM_DEFAULT/RX cannot accept by relaxing gate alone'])
 return table,evidence
def main():
 t,e=build()
 for p,v in [('docs/architecture-T647-acceptance-v1.json',t),('docs/analysis-T647-evidence-v1.json',e)]:
  if '--record' in sys.argv:emit(p,v)
  else:assert get(p)==v,p
 print('PASS: T647 finite engineering contract/source review only; no product execution')
if __name__=='__main__':main()
