"""T580 architect: formula/source audit only; no tester/product/device execution.
Generate only new T580 assets. Compare primitive-derived values to frozen tables;
never use an arena or resident reference value to derive event rows.
"""
from pathlib import Path
from fractions import Fraction
import ast
import hashlib
import json

ROOT=Path(__file__).resolve().parents[1]
D=ROOT/'docs'
N,L,O,JMAX=128,64,4096,32


def receipt(path):
    b=(ROOT/path).read_bytes()
    return dict(path=path,bytes=len(b),sha256=hashlib.sha256(b).hexdigest())


def emit(name,obj):
    (D/name).write_text(json.dumps(obj,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def read(name):
    return json.loads((D/name).read_text(encoding='utf-8-sig'))


parent=read('architecture-T554-g9-dm-manifest-v2.json')
g12=read('architecture-T567-g12-engineering-v1.json')
tester_path='test/t578/resource_recompute.py'
source=(ROOT/tester_path).read_text(encoding='utf-8-sig')
tree=ast.parse(source)
literals={}
for stmt in tree.body:
    if isinstance(stmt,ast.Assign) and len(stmt.targets)==1 and isinstance(stmt.targets[0],ast.Name):
        try:literals[stmt.targets[0].id]=ast.literal_eval(stmt.value)
        except (ValueError,TypeError):pass
assert literals['MANIFEST'][(512,64)]['arena']==44584960
assert literals['MANIFEST'][(512,64)]['pathRows']==163840
assert literals['MANIFEST'][(512,64)]['eventRows'] is None
fn=next(x for x in tree.body if isinstance(x,ast.FunctionDef) and x.name=='recompute')
assert "ev = MANIFEST[(K, Kw)]['eventRows']" in ast.get_source_segment(source,fn)

event_domains={
 'parentMicro':[
   dict(name='natural_mobilization',rows='N',coefficientN=1),
   dict(name='sand_force',rows='N',coefficientN=1),
   dict(name='friction',rows='N',coefficientN=1),
   dict(name='deposit',rows='N',coefficientN=1),
   dict(name='interior_faces_plus_step_seal',rows='(N-1)+1=N',coefficientN=1),
   dict(name='DM_private_predictor',rows='N',coefficientN=1),
   dict(name='P_flight',rows='K',coefficientK=1),
   dict(name='P_capture',rows='K',coefficientK=1)],
 'parentRoot':[
   dict(name='touch_column_compound_envelope',rows='4N'),
   dict(name='packet_source_root_envelope',rows='K'),
   dict(name='root_control_global_envelope',rows='16')],
 'g12Micro':[
   dict(name='P_KICK_A',rows='K'),dict(name='P_KICK_B',rows='K'),
   *[dict(name=n,rows='N') for n in ['WET_PROBE','FS_PRESSURE','FS_DRAG_A','FS_DRAG_B',
       'WATER_FACE','WATER_ORIGIN','WET_CAPACITY_ENERGY','HYDRO_SUMMARY']],
   *[dict(name=n,rows='Kw') for n in ['J_FLIGHT','J_CONTACT','J_ARRIVE','J_RETIRE']],
   dict(name='shared_stage_audit_wall_global_envelope',rows='32')],
 'g12Source':dict(rows=64,charge='once per arena envelope, not per micro/pass/column',
     emissionLimit='one WATER_SOURCE +one affected-column capacity/energy +one affected-column hydro summary +at most16 SOURCE global records <=19; 45 remaining credit unused',
     noPerColumnHydroRows=True),
 'HYDRO_SUMMARY':'one row per column per micro carries all7 HYDRO epochs; no7N extra records',
 'WATER_FACE':'at most N-1 interior rows, walls consume shared32 global credit',
 'global32':'collective phase/audit/wall credit, not32 mandatory rows plus independent wall rows',
 'parentPolicy':'conservative parent allocation retained even when wet writer replaces an old action; a physical record is emitted once, not through both old/new aliases',
 'empty':'credit is not a manufactured event; sealed cursor advances only on actual typed emission',
 'pending':'A drag->pressure and B pressure->drag each charge one composite P row per slot; never charge again at seal',
 'sourceRootUnion':'SOURCE and QUANTUM are distinct root kinds; parent+64 is a conservative additive capacity, not a claim both execute in one root'}
profiles=[]
for K in [192,512]:
    parent_state_items={
      'header_and_command_capsules':2048,'column32':N*32*4,
      'bed_and_flow_cohort4':2*N*L*4*4,'packet64':K*64*4,'sandSource16':O*16*4}
    state_parent=sum(parent_state_items.values())
    r1_work_items={'faces129x32':(N+1)*32*4,'twoColumnStaging32':2*N*32*4,
      'Hamilton32PerOrigin':N*L*32*4,'twoOriginAuditU64':2*O*8,
      'twoParticleIndexesU32':2*K*4,'input128':128*4,'sixSummary64':6*64*4,
      'rootStageControls':4096,'completion256':256*4}
    t524_extra_items={'researchConfigQuota':1536,'rootControlExtension':512,
      'columnWideLedgers':9216,'particleContactStaging':32768,
      'originAuditExtension':65536,'stageReplayDiagnostics':5120}
    r1_work=sum(r1_work_items.values())+sum(t524_extra_items.values())
    assert sum(t524_extra_items.values())==114688
    dm_add=N*48*4
    dm_work=r1_work+dm_add
    parent_micro=6*N+2*K
    parent_root=4*N+K+16
    parent_E=JMAX*parent_micro+parent_root
    parent_P=JMAX*8*K
    pp=next(r for r in parent['resources'] if r['K']==K)
    assert (parent_E,parent_P,state_parent,dm_work)==(pp['eventRows'],pp['pathRows'],pp['stateBytes'],pp['workBytes'])
    for Kw in [64,128]:
        extra_micro=2*K+8*N+4*Kw+32
        extra_source=64
        E=parent_E+JMAX*extra_micro+extra_source
        assert E==JMAX*(14*N+4*K+4*Kw+32)+4*N+K+80
        assert E==58960+129*K+128*Kw
        P=parent_P+JMAX*16*Kw
        A=E*256+P*64
        assert A==15093760+49408*K+65536*Kw
        state_add_items={'waterHeader64':64*4,'waterColumn16':N*16*4,
           'waterOrigin4':N*L*4*4,'waterSource16':O*16*4,'waterPacket32':Kw*32*4}
        state=state_parent+sum(state_add_items.values())
        g12_work_fixed_items={'hydroColumn128':N*128*4,'waterFace64':(N+1)*64*4,
           'fourFinalShare4Planes':4*N*L*4*4,'wetRootControl':8192,
           'wetDrive32':N*32*4,'wideColumn32':N*32*4,'sixWaterSummary64':6*64*4,
           'prefixCompare':8192,'runtimeConfigExtension64':64*4,'reservedZero':2048}
        assert sum(g12_work_fixed_items.values())==675840
        work=dm_work+sum(g12_work_fixed_items.values())+K*32*4+Kw*64*4
        assert state==944384+256*K+128*Kw
        assert work==1985664+136*K+256*Kw
        resident=5*state+work+4*A
        root=2*state+2*A+65536+4096
        core=resident+65536+4096
        trials=core+6*A
        core_peak=resident+2*core+22528+16384
        full_peak=resident+6*A+2*trials+22528+16384
        assert core_peak==3*resident+178176
        assert full_peak==3*resident+18*A+178176
        row=dict(K=K,Kw=Kw,parentStateItems=parent_state_items,parentStateBytes=state_parent,
          parentR1DesignWorkItems=r1_work_items,T524EngineeringExtraItems=t524_extra_items,
          parentR1EngineeringWorkBytes=r1_work,DMWorkAddBytes=dm_add,parentDMWorkBytes=dm_work,
          stateAddItems=state_add_items,g12FixedWorkItems=g12_work_fixed_items,
          eventBreakdown=dict(parentMicroRows=parent_micro,parentRootRows=parent_root,
              parentArenaRows=parent_E,g12MicroRows=extra_micro,g12MicroTotal=32*extra_micro,
              g12SourceExtraRows=extra_source,g12ArenaRows=E),
          pathBreakdown=dict(parentSandRows=parent_P,extraWaterRows=32*16*Kw,totalRows=P),
          stateBytes=state,workBytes=work,eventRows=E,pathRows=P,
          eventBytes=E*256,pathByteOffset=E*256,pathBytes=P*64,paddingBetweenEventAndPath=0,
          arenaBytes=A,residentPayloadBytes=resident,ROOTSlotBytes=root,FULL_CORESlotBytes=core,
          FULL_TRIALSSlotBytes=trials,FULL_TRIALSGPUAdditionalBytes=6*A,
          twoFULL_CORESlotsBytes=2*core,twoFULL_TRIALSSlotsBytes=2*trials,
          knownCoreResidentTwoColdInputPageBytes=core_peak,
          knownFullTrialsResidentTwoColdInputPageBytes=full_peak,
          coexistence=dict(normalGPUStateBanks=5,normalGPUArenas=4,normalGPUWorkBuffers=1,
             optionalDiscoveryGPUArenas=6,nativeSlots=2,nativeTrialArenasPerSlot=6,
             fullPeakArenaEquivalent=4+6+2*(4+6),
             decomposition=dict(resident=resident,discoveryGPU=6*A,
                 nativeCold0=trials,nativeCold1=trials,inputSnapshot=22528,page=16384)))
        assert row['coexistence']['fullPeakArenaEquivalent']==30
        assert row['pathByteOffset']%256==0 and row['pathBytes']%64==0 and A%4096==0
        reference=next(r for r in g12['resources'] if (r['K'],r['Kw'])==(K,Kw))
        keys=['stateBytes','workBytes','eventRows','pathRows','pathByteOffset','arenaBytes',
          'residentPayloadBytes','ROOTSlotBytes','FULL_CORESlotBytes','FULL_TRIALSSlotBytes',
          'FULL_TRIALSGPUAdditionalBytes','twoFULL_CORESlotsBytes','twoFULL_TRIALSSlotsBytes',
          'knownCoreResidentTwoColdInputPageBytes','knownFullTrialsResidentTwoColdInputPageBytes']
        assert all(row[k]==reference[k] for k in keys),(K,Kw)
        profiles.append(row)

reported=Fraction('133590.625')
proper_inverse=Fraction(44584960-163840*64,256)
assert proper_inverse==133200
implied_arena=reported*256+163840*64
assert implied_arena==44684960 and implied_arena-44584960==100000

contract={'schema':'T580_G12_RESOURCE_FORMULAS_V1','date':'2026-11-18',
 'status':'ENGINEERING_FORMULA_CLARIFICATION_NO_BUDGET_INCREASE','units':'integer event/path rows; all byte quantities exact raw payload bytes',
 'constants':{'N':N,'K':[192,512],'Kw':[64,128],'JMax':32,'eventStrideBytes':256,'pathStrideBytes':64},
 'formulas':{'parentR1EventRows':'32*(5N+2K)+(4N+K+16)',
   'parentDMEventRows':'32*(6N+2K)+(4N+K+16)',
   'g12EventIncrement':'32*(2K+8N+4Kw+32)+64',
   'g12EventRows':'32*(14N+4K+4Kw+32)+4N+K+80',
   'g12EventRowsN128':'58960+129K+128Kw',
   'pathRows':'32*8K+32*16Kw','pathRowsSimplified':'256K+512Kw',
   'arenaBytes':'256*eventRows+64*pathRows',
   'arenaBytesN128':'15093760+49408K+65536Kw',
   'stateBytesN128':'944384+256K+128Kw',
   'workBytesN128':'1985664+136K+256Kw',
   'ROOT':'2S+2A+65536+4096','FULL_CORE':'R+65536+4096',
   'FULL_TRIALS':'FULL_CORE+6A','resident':'5S+W+4A',
   'coreKnownPeak':'3R+178176','fullTrialsKnownPeak':'3R+18A+178176'},
 'eventDomains':event_domains,'profiles':profiles,
 'alignment':{'rawInternalPaddingBytes':0,'eventBegin':0,'pathBegin':'256E',
   'binding':'whole arena buffer at binding3, internal offsets are shader array addresses',
   'rangeBinding':'if an implementation uses glBindBufferRange, query actual GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT and report added allocation/ranges separately',
   'notMeasured':['driver/internal allocator','GL range offset alignment','FBO','host fixed state','JIT','JS/runtime','files'],
   'rule':'no invented padding added to row counts and no fractional event rows; revised physical ranges must remain compatible with declared byte dictionary'},
 'producerAccounting':{'SOURCE64':'once at root/arena envelope, never64*16 source passes or64*32 microsteps',
   'discovery':'six scopes each get at most one saved arena; no multiplication of E by190 scope-micro count',
   'pending':'planned/sealed cursor plus outstanding composite window <=E; seal does not reserve a second copy',
   'onOverflow':'typed failure with requested/remaining/phase evidence, no truncation or silent budget growth'},
 'T576':{'physicalSupportChanged':False,'newBudget':False,'ABIOrOffsetsChanged':False,
   'dispatchChanged':False,'realPrerequisite':'T573 complete DM backend',
   'instruction':'consume expanded formulas and same four capacities; do not copy failed inverse arithmetic or bypass evidence credit'},
 'references':[receipt(p) for p in ['docs/algorithm-T515-gpu-three-state-dynamics-v1.md',
   'docs/algorithm-G9-dynamic-mobilization-v2.md','docs/algorithm-G12-water-sfp-coupling-v1.md',
   'docs/architecture-T554-g9-dm-manifest-v2.json']],
 'deviceExecuted':False,'productPASS':False}
emit('architecture-T580-resource-formulas-v1.json',contract)
emit('analysis-T580-resource-attribution-v1.json',{
 'schema':'T580_SOURCE_AND_RESOURCE_ALGEBRA_AUDIT_V1','date':'2026-11-18',
 'result':'ATTRIBUTION_COMPLETE_STATIC_FORMULAS_MATCH_NOT_PRODUCT_PASS',
 'inputs':[receipt(p) for p in ['docs/test-report-T578-g12-resources-v1.md',tester_path,
      'docs/architecture-T567-g12-engineering-v1.json','docs/architecture-T567-water-sfp-engineering-v1.md',
      'docs/analysis-T567-T570-engineering-checks-v1.py',
      'docs/analysis-T552-T554-engineering-checks-v1.py','docs/design-G9-dynamic-mobilization-checks-v2.py']],
 'testerSourceFragment':{'line':fn.lineno,'source':ast.get_source_segment(source,fn)},
 'findings':['all4 eventRows/arena values were already present in T567 formal engineering manifest; design reference table has2 rows',
   'tester code reads eventRows from its own2-known/2-None MANIFEST dictionary; arena/resident/etc are not independent from eventRows references',
   'None descendants skipped by checks comprehension; cannot report all4 downstream formulas independently verified',
   'formal parent and G12 formulas exist but engineering flat trace of row domains and WORK baseline needed explicit packaging',
   '6 extra GPU discovery arenas and6 per native slot already in explicit engineering fields; inverse alone is not independent proof'],
 'inverseAudit':{'givenArena':44584960,'givenPathRows':163840,'pathStride':64,'eventStride':256,
    'properEventRows':int(proper_inverse),'reportedEventRows':str(reported),
    'impliedArenaForReportedValue':int(implied_arena),'impliedExtraBytes':100000,
    'cause':'arithmetic or undisclosed operand mismatch; no raw inversion trace supports a more specific cause',
    'alignmentExplanationSupported':False},
 'all4PrimitiveCrossChecks':True,'profiles':[{k:p[k] for k in ['K','Kw','eventRows','arenaBytes','stateBytes','workBytes',
    'residentPayloadBytes','knownCoreResidentTwoColdInputPageBytes','knownFullTrialsResidentTwoColdInputPageBytes']} for p in profiles],
 'historicalT578Status':'PARTIAL preserved; tester must independently resume against expanded input contract',
 'testerCodeChanged':False,'testerExecuted':False,'productTestsExecuted':False,'deviceExecuted':False})
print(json.dumps({'T580':'primitive resource formulas/source audit complete',
  'eventRows':[p['eventRows'] for p in profiles],'arenaBytes':[p['arenaBytes'] for p in profiles],
  'reportedFractionReproduced':False,'numericBudgetChanged':False},ensure_ascii=False))
