"""Architect source/ABI/unit/capacity audit only. No product, GL or CPU physics tests.
Writes only new T567/T570 engineering artifacts; never runs old generators.
"""
from pathlib import Path
from collections import Counter
from fractions import Fraction
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
D = ROOT/'docs'


def read(p):
    return json.loads((ROOT/p).read_text(encoding='utf-8-sig'))


def emit(name,obj):
    (D/name).write_text(json.dumps(obj,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def receipt(p):
    b=(ROOT/p).read_bytes()
    return dict(path=p,bytes=len(b),sha256=hashlib.sha256(b).hexdigest())


def fields(names, count=None):
    if count:
        assert len(names)<=count
        names=names+['reserved'+str(i) for i in range(len(names),count)]
    return [dict(word=i,name=n) for i,n in enumerate(names)]


def seg(items,start=0):
    out=[]
    for name,n,stride in items:
        size=n*stride
        out.append(dict(name=name,byteStart=start,byteEnd=start+size,bytes=size,count=n,strideBytes=stride))
        start+=size
    return out


parent=read('docs/architecture-T554-g9-dm-manifest-v2.json')
pd=read('docs/architecture-T554-g9-dm-dispatch-v2.json')
design=read('docs/design-G12-water-sfp-manifest-v1.json')
fixtures=read('docs/design-G12-water-sfp-fixtures-v1.json')
assert [x['id'] for x in fixtures['cases']]==[f'C{i:02}' for i in range(1,21)]
names=design['micro_passes']; assert len(names)==32
assert sum(n.startswith('HYDRO_') for n in names)==7
source_ops=['SOURCE_INIT_BASE','SOURCE_VALIDATE_INPUT','SOURCE_DEDUP','SOURCE_DECODE',
            'SOURCE_WET_DOMAIN_GATE','SOURCE_CREDIT','SOURCE_PRIVATE_BIRTH','SOURCE_HYDRO',
            'SOURCE_ORIGIN_CERTIFICATE','SOURCE_LEDGER','SOURCE_WATER_ENERGY_AUDIT',
            'SOURCE_AUDIT_REPLAY','SOURCE_COMPARE','SOURCE_PACK_SCENE',
            'SOURCE_FINAL_WET_AUDIT','SOURCE_SEAL']
source=[dict(index=i,op=n,bindingCount=4) for i,n in enumerate(source_ops)]
wet=[]
for row in pd['quantum']:
    if row['micro']>=0:
        if row['op']!='DM_REQUEST':
            continue
        for op in names:
            wet.append(dict(row,op=op,parentIndex=None))
    else:
        wet.append(dict(row,parentIndex=row['index']))
for i,row in enumerate(wet):row['index']=i
assert len(wet)==6204 and len(source)==16
assert len(pd['quantum'])==2404
assert Counter(x['role'] for x in wet)==Counter({
    'DISCOVERY':63*32+6*8,'REJECTED_PREFIX_CHECK':63*32+6*8,
    'AUTHORITATIVE':32*32+8,'REPLAY':32*32+8,'ROOT':12})
assert all(x['bindingCount']==4 for x in wet+source)
emit('architecture-T567-g12-dispatch-v1.json',{
    'schema':'T567_G12_FULL_DISPATCH_V1','source':source,'wetQuantum':wet,'dryQuantum':pd['quantum'],
    'opcodes':{n:i for i,n in enumerate(dict.fromkeys(source_ops+[x['op'] for x in wet]+[x['op'] for x in pd['quantum']]))},
    'geometry':{'localSize':[64,1,1],'workgroups':[1,1,1]},
    'rootKind':'explicit SOURCE/QUANTUM plus global BASE water inventory mode; never infer from pass count',
    'passCount':{'SOURCE':16,'WET_QUANTUM':6204,'DRY_QUANTUM':2404,'WET_G1F4':24832,'DRY_G1F4':9632}})

water_header=['magic','revision','Kw','nextJId','waterSourceCount','flags','initialWaterLo','initialWaterHi',
              'admittedWaterLo','admittedWaterHi','sumHLo','sumHHi','sumJLo','sumJHi','lastWaterSourceSeqLo','lastWaterSourceSeqHi']
for key in ['waterSourceImpulseX','waterSourceImpulseY','waterGravityImpulseY','wetSupportImpulseX',
            'wetSupportImpulseY','waterSourceEnergy','waterSourceWork','wetTouchWork',
            'initialWaterEnergy','waterDragDissipation','wetConversionDissipation']:
    water_header += [key+str(i) for i in range(4)]
water_header += ['phaseFlags','waterIdExhausted','reserved62','seal']
assert len(water_header)==64
water_column=['H4','pWLo','pWHi','J4','eta4','R4','W4','C_Q16','waterOriginCount','PCount','JCount',
              'wetFlags','hydroEpoch','hydroPhase','geometryStatus','reserved15']
water_packet=['mass4','xQ20','y4Q22','vxQ20','vyQ20','column','id','generation','originLo','originHi',
              'sourceId','flags','birthSeqLo','birthSeqHi','ageTicksLo','ageTicksHi','contactKind',
              'contactLevel4','pathBegin','pathCount','birthCommitLo','birthCommitHi']
water_packet += ['reserved'+str(i) for i in range(len(water_packet),31)]+['seal']
hydro=['epoch','phase','B','Psum','H4','J4','eta4','R4','W4','SWet4','FWet4','C_Q16','PCount','JCount','lowerBoundIterations','status']
for key in ['waterPotentialQ62','waterPotentialScaledLower','waterPotentialScaledUpper','PiWQ62','PiFQ60','NFQ40']:
    hydro += [key+str(i) for i in range(4)]
hydro += ['aPXQ20','gammaFQ16','gammaSQ16','gammaRefQ16','gnRefQ20','psiLo','psiHi',
          'probeDepth','jProbeLo','jProbeHi','aWaterLo','aWaterHi','fStaticEligible','waveSpeedQ20',
          'sourceGeometryEpoch','wetLengthEpoch','nextEta4','nextR4','nextW4']
faces=['leftColumn','rightColumn','qW4','jpWLo','jpWHi','qF','jpFLo','jpFHi','CfaceQ16','waveSpeedQ20',
       'etaLeft4','etaRight4','FLeft','FRight','flags','status']
for key in ['fmWQ42','fpWQ62','ALQ62','ARQ62','PiWLQ62','PiWRQ62','capacityOperand','reservedWide']:
    faces += [key+str(i) for i in range(4)]
drive=['eta4','SWet4','FWet4','probeDepth','gammaRefQ16','gnRefQ20','psiLo','psiHi','jProbeLo','jProbeHi',
       'aWaterLo','aWaterHi','direction','qPhysical','qOrigin','qActual','pendingTouchJyLo','pendingTouchJyHi',
       'touchConsumed','touchReason','modeEntry','pEntryLo','pEntryHi','wetEpoch']

config_names=['parentConfig64_'+str(i) for i in range(64)]
config_names += ['magic','revision','profile','Kw','kappaQ16','phiNumerator','phiDenominator',
                 'waterDensityNumerator','waterDensityDenominator','bulkDensityNumerator','bulkDensityDenominator',
                 'waterMassScaleBits','wetAngleIndex','waterSourcesMax','waterOriginsPerColumn','JPathRowsPerMicro']
for key in ['designSHA','parentParameterSHA','seedRecipeSHA','fixturesSHA']:
    config_names += [key+str(i) for i in range(8)]
config_names += ['reserved'+str(i) for i in range(len(config_names),128)]
assert len(config_names)==128
parent_input=read('docs/architecture-T524-g9-spf-manifest-v1.json')['designTables']['input']
input_names=list(parent_input['wordNames']); input_names[78]='waterSourceMode';input_names[79]='waterUnitRevision'
water_mask=sorted(set(parent_input['bodyMasks']['SOURCE']+list(range(60,64))+[78,79]))
source_mask=sorted(set(parent_input['bodyMasks']['SOURCE']+list(range(60,64))))

common=['magic','revision','kind','trial','nonceLo','nonceHi','ordinal','stage','micro','fromColumn',
        'toColumn','slot','id','generation','flags']
event_bodies={
 'WATER_SOURCE':['sourceMode','q4','originLo','originHi','sourceId','xQ20','y4Q22','vxQ20','vyQ20',
                 'oldH4','newH4','oldJ4','newJ4','oldEta4','newEta4','workCap0','workCap1','workCap2','workCap3'],
 'WET_PROBE':['probeDepth','SWet4','FWet4','eta4','gammaRefQ16','gnRefQ20','jProbeLo','jProbeHi',
              'aWaterLo','aWaterHi','qPhysical','qOrigin','qActual','reason'],
 'FS_PRESSURE':['oldPFLo','oldPFHi','newPFLo','newPFHi','oldPWLo','oldPWHi','newPWLo','newPWHi',
                'jFRequestedLo','jFRequestedHi','jFActualLo','jFActualHi','jSActualLo','jSActualHi',
                'supportXLo','supportXHi','eta4','FWet4','SWet4','aPXQ20','D'],
 'FS_DRAG_A':['oldPFLo','oldPFHi','newPFLo','newPFHi','oldPWLo','oldPWHi','newPWLo','newPWHi',
              'jFRequestedLo','jFRequestedHi','jFActualLo','jFActualHi','jSActualLo','jSActualHi',
              'supportXLo','supportXHi','gammaFQ16','gammaSQ16','kappaQ16','D'],
 'FS_DRAG_B':['oldPFLo','oldPFHi','newPFLo','newPFHi','oldPWLo','oldPWHi','newPWLo','newPWHi',
              'jFRequestedLo','jFRequestedHi','jFActualLo','jFActualHi','jSActualLo','jSActualHi',
              'supportXLo','supportXHi','gammaFQ16','gammaSQ16','kappaQ16','D'],
 'P_KICK_A':['mass','gammaQ16','wet4','eta4','kappaEff','D','oldVx','oldVy','newVx','newVy',
             'oldPWLo','oldPWHi','newPWLo','newPWHi','pressureReqXLo','pressureReqXHi',
             'pressureActXLo','pressureActXHi','pressureReqYLo','pressureReqYHi','pressureActYLo','pressureActYHi',
             'dragReqXLo','dragReqXHi','dragActXLo','dragActXHi','dragReqYLo','dragReqYHi',
             'dragActYLo','dragActYHi','supportXLo','supportXHi','supportYLo','supportYHi'],
 'P_KICK_B':['mass','gammaQ16','wet4','eta4','kappaEff','D','oldVx','oldVy','newVx','newVy',
             'oldPWLo','oldPWHi','newPWLo','newPWHi','pressureReqXLo','pressureReqXHi',
             'pressureActXLo','pressureActXHi','pressureReqYLo','pressureReqYHi','pressureActYLo','pressureActYHi',
             'dragReqXLo','dragReqXHi','dragActXLo','dragActXHi','dragReqYLo','dragReqYHi',
             'dragActYLo','dragActYHi','supportXLo','supportXHi','supportYLo','supportYHi'],
 'WATER_FACE':['q4','jpLo','jpHi','oldHL4','oldHR4','newHL4','newHR4','oldPWLLo','oldPWLHi',
               'oldPWRLo','oldPWRHi','newPWLLo','newPWLHi','newPWRLo','newPWRHi','etaL4','etaR4','CfaceQ16','waveSpeedQ20'],
 'WATER_ORIGIN':['qLeft4','qRight4','oldCount','newCount','oldMass4','newMass4','sharePlanOffset','sharePlanCount','status'],
 'WET_CAPACITY_ENERGY':['oldBulk','newBulk','oldH4','newH4','oldJ4','newJ4','waterEnergyDeltaLower0',
                        'waterEnergyDeltaLower1','waterEnergyDeltaLower2','waterEnergyDeltaLower3',
                        'waterEnergyDeltaUpper0','waterEnergyDeltaUpper1','waterEnergyDeltaUpper2','waterEnergyDeltaUpper3','reason'],
 'HYDRO_SUMMARY':['entryEpoch','flightEpoch','flowEpoch','depositEpoch','captureEpoch','arriveEpoch','endEpoch',
                  'eta4','R4','W4','H4','Psum','J4','maxIterations','status'],
 'J_FLIGHT':['mass4','oldX','oldY4','newX','newY4','oldVx','oldVy','newVx','newVy','oldColumn','newColumn',
             'originLo','originHi','sourceId','pathBegin','pathCount','ticksLo','ticksHi','reason'],
 'J_CONTACT':['mass4','x','y4','contactLevel4','oldColumn','newColumn','contactKind','vyBefore','vyAfter','reason'],
 'J_ARRIVE':['mass4','originLo','originHi','sourceId','oldH4','newH4','oldJ4','newJ4','oldPWLo','oldPWHi',
             'newPWLo','newPWHi','oldEta4','newEta4','supportYLo','supportYHi','oldGeneration','retiredGeneration'],
 'J_RETIRE':['mass4','originLo','originHi','sourceId','oldId','oldGeneration','newId','newGeneration','reason'],
 'WET_GLOBAL':['auditMask','reason','phase','operand','waterMassLo','waterMassHi','sourceCommitLo','sourceCommitHi']}
# A phase record may span two kernels, so retain BOTH sub-kernel endpoints;
# an overall old/new water momentum pair would overlap other P records.
for name in ['FS_DRAG_A','FS_DRAG_B']:
    event_bodies[name]=['oldPFLo','oldPFHi','newPFLo','newPFHi',
        'FBeforePWLo','FBeforePWHi','FAfterPWLo','FAfterPWHi',
        'SBeforePWLo','SBeforePWHi','SAfterPWLo','SAfterPWHi',
        'jFRequestedLo','jFRequestedHi','jFActualLo','jFActualHi',
        'jSRequestedLo','jSRequestedHi','jSActualLo','jSActualHi',
        'supportXLo','supportXHi','gammaFQ16','gammaSQ16','kappaQ16','D']
for name in ['P_KICK_A','P_KICK_B']:
    event_bodies[name]=['mass','gammaQ16','wet4','eta4','kappaEff','D',
        'oldVx','oldVy','intermediateVx','intermediateVy','newVx','newVy',
        'dragBeforePWLo','dragBeforePWHi','dragAfterPWLo','dragAfterPWHi',
        'pressureBeforePWLo','pressureBeforePWHi','pressureAfterPWLo','pressureAfterPWHi',
        'pressureReqXLo','pressureReqXHi','pressureActXLo','pressureActXHi',
        'pressureReqYLo','pressureReqYHi','pressureActYLo','pressureActYHi',
        'dragReqXLo','dragReqXHi','dragActXLo','dragActXHi',
        'dragReqYLo','dragReqYHi','dragActYLo','dragActYHi',
        'supportXLo','supportXHi','supportYLo','supportYHi']
event_bodies['WET_GLOBAL'] += ['waterGravityY'+str(i) for i in range(4)]
event_bodies['WET_GLOBAL'] += ['normalSupportY'+str(i) for i in range(4)]
events=[]
for code,(name,body) in enumerate(event_bodies.items(),32):
    assert len(body)<=48
    words=common+body+['reserved'+str(i) for i in range(15+len(body),63)]+['seal']
    used=list(range(15+len(body)))+[63]
    masks=[sum(1<<(i%32) for i in used if i//32==h) for h in [0,1]]
    assert len(words)==64
    events.append(dict(name=name,kind=code,words=fields(words),usedMask=masks,unusedZero=True,
                       roleMask=3,sealWord=63,seal=0xC0DE1201))

resources=[]
for p in parent['resources']:
    for Kw in [64,128]:
        K=p['K']
        state=seg([('waterHeader64',1,256),('waterColumn16',128,64),('waterOrigin4',128*64,16),
                   ('waterSource16',4096,64),('waterPacket32',Kw,128)],p['stateBytes'])
        work=seg([('hydroColumn128',128,512),('waterFace64',129,256),
                  ('waterLeftShare4',128*64,16),('waterRightShare4',128*64,16),
                  ('sandLeftShare4',128*64,16),('sandRightShare4',128*64,16),
                  ('wetRootControl',1,8192),('wetDrive32',128,128),('waterWideColumn32',128,128),
                  ('sixTrialWaterSummary64',6,256),('prefixCompare',1,8192),
                  ('runtimeConfigExtension64',1,256),('reservedZero',1,2048),
                  ('packetKick32',K,128),('waterFlight64',Kw,256)],p['workBytes'])
        state_bytes=state[-1]['byteEnd'];work_bytes=work[-1]['byteEnd']
        assert state_bytes-p['stateBytes']==401664+128*Kw
        assert work_bytes-p['workBytes']==675840+128*K+256*Kw
        event_rows=p['eventRows']+32*(2*K+8*128+4*Kw+32)+64
        path_rows=p['pathRows']+32*16*Kw
        arena=event_rows*256+path_rows*64
        resident=5*state_bytes+4*arena+work_bytes
        metadata=65536+4096
        cold_root=2*state_bytes+2*arena+metadata
        cold_core=resident+metadata
        cold_trials=cold_core+6*arena
        resources.append(dict(K=K,Kw=Kw,stateSegments=state,workSegments=work,stateBytes=state_bytes,
            workBytes=work_bytes,eventRows=event_rows,pathRows=path_rows,pathByteOffset=event_rows*256,
            arenaBytes=arena,residentPayloadBytes=resident,largestGLObjectBytes=max(state_bytes,work_bytes,arena),
            ROOTSlotBytes=cold_root,FULL_CORESlotBytes=cold_core,FULL_TRIALSSlotBytes=cold_trials,
            FULL_TRIALSGPUAdditionalBytes=6*arena,twoFULL_CORESlotsBytes=2*cold_core,
            twoFULL_TRIALSSlotsBytes=2*cold_trials,
            knownCoreResidentTwoColdInputPageBytes=resident+2*cold_core+22528+16384,
            knownFullTrialsResidentTwoColdInputPageBytes=resident+6*arena+2*cold_trials+22528+16384,
            coldDescriptorsMax=32,coldPrefixBytes=65536,
            fullCorePageUpperBound=1+(cold_core+4095)//4096+32,
            fullTrialsPageUpperBound=1+(cold_trials+4095)//4096+32,
            coldTwoSlotsSharedActualCredit=True,excluded=['alignment','host objects','driver/JIT','FBO>=8*w*h','JS/runtime','files']))
for ref in design['resources']['reference_profiles']:
    row=next(x for x in resources if (x['K'],x['Kw'])==(ref['K'],ref['Kw']))
    assert all(row[k]==v for k,v in ref.items())

writer={
 'HYDRO_ENTRY':('derived geometry only','current sealed H/B/P; wet-domain and overlap guards'),
 'WATER_PROBE':('WORK wetDrive only','read entry water/geometry; no actual impulse/time'),
 'DM_REQUEST':('WORK qPhysical/qOrigin','entry-frozen gnRef/aWater; same monotone Eval'),
 'DM_APPLY':('S/F/origins','actual top cut only; B/H invariant; invalidate F/S wet lengths'),
 'DRAG_HALF_A':('pF/P/pW/support; pending kick rows','F then unsigned P ID then S; begin pending event window'),
 'P_PRESSURE_A':('P/pW/support; seal kick rows','same entry geometry; actual lattice reactions'),
 'SAND_FLIGHT':('P/path/column reservation','parent rational flight using kicked velocities'),
 'J_FLIGHT':('J/path/reservation/contact flags','point water packet at Q22 y4, no glyph volume; at most16 rows/slot'),
 'HYDRO_AFTER_FLIGHT':('derived geometry only','current sealed flight; reject overlap/capacity'),
 'F_PREDICTOR':('private F predictor WORK','wet PiF; no authority inventory/time'),
 'F_MIDPOINT':('private midpoint WORK','same entry and frozen force request'),
 'F_CORRECTOR':('WORK final F q/jp','one corrector; actual not predictor'),
 'WATER_FACE':('WORK final water q/jp','eta head, Cface Q16 divide before Q42 subtraction'),
 'COUPLED_FORCE_RESERVE':('WORK actual F/S pressure j and paired targets','preflight complete water+F momenta/capacity; no authority writes'),
 'WATER_ORIGIN_RESERVE':('water left/right final-share planes','reuse parent Hamilton scratch; preserve original accepted shares'),
 'F_ORIGIN_RESERVE':('sand left/right final-share planes','preserve parent left-first tie; previous water planes remain stable'),
 'COUPLED_FLOW_APPLY':('F/H/pF/pW/both origin maps/support/events','stage all new maps in reusable scratch before any C write; barrier then pair apply'),
 'HYDRO_AFTER_FLOW':('derived geometry only','current applied H/F/P'),
 'P_PRESSURE_B':('P/pW/support; pending kick rows','new flow geometry; begin pending event window'),
 'DRAG_HALF_B':('pF/P/pW/support; seal kick rows','reverse A order: S, descending P ID, F'),
 'F_FRICTION_DEPOSIT':('F/S/pF/origins/support','wet NF, rest/p0/stability; local coupled energy'),
 'HYDRO_AFTER_DEPOSIT':('derived geometry only','B preserved; fresh S/F wet lengths'),
 'SAND_CAPTURE_PLAN':('WORK ordered P capture proposals','column/y/id; no retire before full preflight'),
 'SAND_CAPTURE_APPLY':('P/F/origins/support/events','same-position interval; local water+sand energy gate'),
 'HYDRO_AFTER_CAPTURE':('derived geometry only','capture sealed geometry'),
 'J_ARRIVAL_PLAN':('WORK ordered contact proposals','column/y4/id low-to-high, dynamic eta reclassification'),
 'J_ARRIVAL_APPLY':('J/H/pW/water origins/ID/support/events','per-packet current eta/capacity/local energy; next micro for new H flow'),
 'HYDRO_AFTER_ARRIVE':('derived geometry only','current J/H arrival; no second transport'),
 'LAUNCH':('F/P/origins/ID/workCap/events','consume root pending upward touch once in first active micro; no repeated impulse'),
 'HYDRO_END':('derived geometry only','final physical geometry/current H'),
 'WET_LEDGER_ENERGY_AUDIT':('WORK audit flags','exact inventories/origins/reactions and closed-micro Q62 energy gate'),
 'STAGE_SUMMARY_SEAL':('WORK completion/summary','all gates passed, pending empty, canonical typed evidence sealed')}
assert set(writer)==set(names)

manifest={
 'schema':'T567_G12_WATER_SFP_ENGINEERING_V1','date':'2026-11-17',
 'status':'ENGINEERING_READY_RESEARCH_NOT_IMPLEMENTED_NOT_VALIDATED','notUsingDevice':True,
 'profile':'G12_POROUS_SPF_Q20_V1','profileCode':int.from_bytes(b'G12P','little'),
 'magic':{k:int.from_bytes(v,'little') for k,v in {'state':b'G12S','input':b'G12I','event':b'G12E',
          'path':b'G12T','config':b'G12C','completion':b'G12F','cold':b'G12K','waterHeader':b'G12W'}.items()},
 'revision':1,'seal':0xC0DE1201,'defaultOFF':True,'CPUPhysicsAdded':False,
 'inputs':[receipt('docs/'+n) for n in ['algorithm-G12-water-sfp-coupling-v1.md',
          'design-G12-water-sfp-manifest-v1.json','design-G12-water-sfp-fixtures-v1.json',
          'architecture-T554-g9-dm-manifest-v2.json','architecture-T554-g9-dm-dispatch-v2.json']],
 'support':design['support'],'parameters':design['parameters'],
 'config':{'materialConfig128':fields(config_names),'parent64':'exact frozen parent material config bytes',
           'materialSHA':'all128 LE u32 words, not the legacy parent-only SHA',
           'parentSHA':'exact parent64 separately bound at88..95',
           'GPUConfig192':'runtime envelope64 + materialConfig128; parent allocation128 grows64 in declared runtimeConfigExtension64',
           'waterConstants':{'phi':[1,2],'waterDensity':[1,1],'bulkDensity':[1,1],
                             'massScaleBits':22,'wetAngleIndex':0,'waterSourceMax':4096,'mapMax':64,'JPathRowsMax':16},
           'seedSHA':'actual initialized raw bytes after derived caches/audits, never recipe hash'},
 'coldSeed':{'recipeSHA':'SHA256 exact SeedDescriptor bytes: initial totals, ordered S/F/H origin inventory, P/J coordinates/velocities/IDs/generations, recipe revision; excludes derived cache',
             'dispatch':['SEED_UPLOAD_VALIDATE','SEED_HYDRO_ENERGY_AUDIT','SEED_FINAL_SEAL'],
             'copies':'after sealed initialization, four full-state copies to other banks, separate actual copy ledger',
             'hash':'stream finalized GPU seed through existing cold caller-span after fence; bind actualSeedSHA in runtime identity outside hashed seed bytes',
             'memory':'same allocated state/work/arena, <=4096B caller-span in existing16384B page credit; no hidden full seed CPU buffer',
             'publish':'no permit/CURRENT until geometry/audit/hash identity binding complete'},
 'state':{'waterHeader64':fields(water_header),'waterColumn16':fields(water_column),
          'waterOrigin4':fields(['originLo','originHi','sourceId','mass4']),
          'waterSource16':fields(['originLo','originHi','sourceId','sourceKind','admittedMassLo','admittedMassHi',
                         'sourceSeqLo','sourceSeqHi','inputGenerationLo','inputGenerationHi','digest0','digest1','digest2','digest3','flags','reserved15']),
          'waterPacket32':fields(water_packet),'freeJ':'zero all except preserved generation; checked increment on reuse',
          'retiredId':'never reused; MAX_ID allocatable once with exhausted flag, no wrap',
          'wetAdditionalLedgers':'parent sand Q40 impulse/Q60 E2 plus declared wet corrections Q42/Q62; never silently reinterpret parent fields',
          'cumulativeWidth':'S128 checked; instantaneous bounds do not excuse cumulative overflow'},
 'work':{'hydroColumn128':fields(hydro,128),'waterFace64':fields(faces,64),
         'wetDrive32':fields(drive,32),'share4':fields(['originLo','originHi','sourceId','actualShare']),
         'packetKick32':'per-packet staged actual/requested S64 impulses/velocities/status, no extra source of authority',
         'waterFlight64':'per-J original/proposed state, path/contact/rational cursor and sort index, max64 words',
         'wideColumn32':'two capacities, aggregate impulses and per-column source/energy guards',
         'pendingControl':'in declared 8192B root control, bounds/cursors/phase/key, no new global binding',
         'scratchReuse':'after both reserves, parent Hamilton scratch stages128*(newFmap256+newHmap256)words before C mutation'},
 'input128':{'fields':fields(input_names),'commandKind':{'NONE':0,'SOURCE_SAND':1,'IMPULSE':2,'SOURCE_WATER':3},
             'waterSourceMode':{'DIRECT_H':0,'FLIGHT_J':1},'waterUnitRevision':1,
             'bodyUsedWords':{'NONE':[],'SOURCE_SAND':source_mask,'IMPULSE':parent_input['bodyMasks']['IMPULSE'],
                              'SOURCE_WATER':water_mask},
             'waterMass':'raw binary64 physical volume bits -> floor(4Q*mass), checked max4CQ and capacity',
             'sourceWork':'words60..63 now valid SOURCE positive-work cap in Q62 scaled interval, dry sand uses parent zero fields',
             'touchWork':'IMPULSE words60..63 retain parent Q60+24 cap; wet comparison promotes by4 into Q62+24, never reinterprets raw bits',
             'dedup':'entire40..127 original capsule + parent identity additions; separate SOURCE/IMPULSE U64 sequences; water and sand share SOURCE sequence',
             'unsupported':'old M3 BULK_WET_H/source preview/repair cold rejected; no legacy water writer'},
 'events':{'parentKinds':'new namespace, parent payload retained; wet DM reserved operands carry eta4/gamma/gnRef/probe',
           'newKinds':events,'phaseSlotsPerMicro':{'P_KICK_A': 'K','P_KICK_B':'K',
             'columnTypes':['WET_PROBE','FS_PRESSURE','FS_DRAG_A','FS_DRAG_B','WATER_FACE','WATER_ORIGIN','WET_CAPACITY_ENERGY','HYDRO_SUMMARY'],
             'JTypes':['J_FLIGHT','J_CONTACT','J_ARRIVE','J_RETIRE'],'global':32},
           'bound':'parent +32*(2K+8N+4Kw+32)+64; WATER_FACE <=N interior, wall reactions in column/global slots',
           'pending':'A drag->pressure and B pressure->drag share one reserved event window; seal entire window only after last pass',
           'phaseReplay':'compound kick block replays sub-kernel endpoints in real order; never apply each composite old/new as a whole overlapping water patch',
           'knownFlags':{'roleBits':[0,1],'roleValues':[0,1,2],'actualPatchBit':2,'mask':7},
           'unused':'all unused physical words zero; compare known flags and original key before role normalization',
           'prefix':'on fault preserve last fully sealed event/path cursors and first typed error; never advance through pending block'},
 'path16':fields(['magic','revision','namespace','slot','id','generation','fromColumn','toColumn',
                  'oldXQ20','oldY4Q22','newXQ20','newY4Q22','tauBegin','tauEnd','kind','seal']),
 'pathNamespace':{'SAND':0,'WATER_J':1},'sandPath':'parent schema/path budget unchanged, namespace chooses correct row dictionary',
 'JGeometry':{'collision':'point as T237, glyph radius/halfheight is not displaced water or contact surface',
              'position':'xQ20 and y4Q22 permits exact odd eta4 contact; separate new reader',
              'energy':'J4*(vx^2+vy^2)+gn*J4*y4/2; rational /2 retained in 24-fraction interval',
              'flight':'parent rational gravity trajectory, wet gx0, vy<=0, no old one-way wet terminal-speed law',
              'contact':'center reaches max(4B,eta4), low-to-high actual y4; J only reserves volume before arrival'},
 'numeric':{'hydroIterationsMax':31,'hydroCallsPerMicro':7,
            'C8':'2*z4 - min(z4,4B) - sum P slab overlap lengths; no union of overlapping packets',
            'hydroExact':'inclusive [0,4CQ] lower_bound then C8(eta4)==2H4; H4=0 eta4=0',
            'Cface':'(CLQ16+CRQ16)/2 exact for32768/65536; convert Q16 product before subtraction',
            'fmW':'RNE(((pWL+pWR)*65536-aQ20*CfaceQ16*(etaR4-etaL4))/(2*65536)); single wide rounding',
            'pressure':design['integer_pressure'],'drag':design['drag'],
            'diffNumeratorMaxBits':114,'division':'signed U128/S128 magnitude and TZ/RNE; fixed maximum limb loops from backend declared',
            'waveSpeed':'U32 checked, may exceed signed32; do not cast to signed velocity',
            'aWater':'wide nonnegative magnitude; reject overflow of parent driver operands rather than truncate',
            'energy':design['energy'],'noEpsilon':True},
 'writers':[dict(passIndex=i+1,op=n,writes=writer[n][0],requires=writer[n][1]) for i,n in enumerate(names)],
 'root':{'banks':5,'arenas':4,'bindings':4,'deadlineNs':250000000,'gapNs':33333333,'quotaHz':120,'displayFps':30,
          'wetMode':'global sum H4+J4 in BASE, not per-column feature and not water flag alone',
          'wetSourceFirstGate':'before H/J commit check horizontal angle and all current P slab disjointness',
          'dryReduction':'same DM operation order/rounding/input physical fields; ignore only declared profile/config identity metadata',
          'dryDerived':'H/J/pW zero; wet geometry cache epoch0 is N/A, no overlap gate on dry P and no stale cache treated as valid',
          'rootTouch':'wet upward Jy staged once, LAUNCH first active micro consumes it once; dry retains parent root touch/launch',
          'requiredMasks':{'DRY_SOURCE':0xffff,'DRY_QUANTUM':0x3ffff,'WET_SOURCE':0x7cffff,'WET_QUANTUM':0x7fffff},
          'newAuditBits':{'VOLUME_HYDRO':18,'WATER_ORIGIN':19,'ACTUAL_REACTION':20,'COUPLED_ENERGY':21,'WET_DOMAIN_SOURCE':22},
          'publish':'all water/sand/derived/IDs/dedup/time commit atomically from same accepted root; no partial water publication',
          'replay':'sixJ discovery, rejected prefix, authoritative, accepted replay each from full same BASE; no host water solver'},
 'dispatch':{'SOURCE':16,'WET_QUANTUM':6204,'DRY_QUANTUM':2404,'WET_G1F4':24832,'DRY_G1F4':9632},
 'resourceFormulaContract':{'schema':'T580_G12_RESOURCE_FORMULAS_V1','path':'docs/architecture-T580-resource-formulas-v1.json',
     'purpose':'explicit parent/G12 row domains, DM WORK baseline, raw alignment and all normal/trial/native copies; numeric capacities unchanged'},
 'resources':resources,
 'complexity':{'perMicroGeometry':'7*(31*N*(K+1)+K*(K-1)/2), no division by64 for serial cost',
               'originReserve':'O(N*64^2) independently sand/water','JOrder':'O(Kw^2) bounded selection',
               'pairDrag':'O(K+N), two ID orders plus wide arithmetic','timeConclusion':'UNMEASURED_NOT_REALTIME_CLAIM'},
 'cold':{'schema':'G12_POROUS_COLD_V1','page':'G12_POROUS_SEGMENT_PAGE_V1',
          'modes':['ROOT','FULL_CORE','FULL_TRIALS'],'slots':2,'readerMax':1,'builderMax':1,
          'ttlNs':'60000000000','prefixBytes':65536,'inputBytes':4096,
          'pageWords':1024,'pageUtf8Bytes':16384,'descriptorsMax':32,
          'FULL_CORE':'T559 design FULL payload, five state/four current role arenas/WORK; not six full discovery originals',
          'FULL_TRIALS':'FULL_CORE +six discovery arenas; reserve6 GPU arenas and add6 arenas each native cold slot',
          'copies':'at most six discovery whole-arena copies separate from dispatch; unexecuted trials SKIPPED with validCount0, never old arena residue',
          'credit':'shared actual capacity with old routes; no third slot until pinned old allocations really freed',
          'reader':'raw state/origin/geometry/ledgers plus independent C8/potential/actual-event checking; reject old eta readers'},
 'scene':{'route':'XComponent+shader only, explicit FAULT or labelled STALE complete old compatible frame',
           'caps':'T560 COMPUTE/SCENE/PRESENT layers; no vertex SSBO; fragment CURRENT state',
           'water':'derive optical free water from same eta and actual solid intervals, J no water level',
           'P':'same CURRENT integer instance attributes, physical slab for immersion; glyph radius not volume',
           'J':'new waterPacket32 integer instance attributes using mass4/x/y4, distinct VAO/fragment optics',
           'G11':'dry G11 is NOT a wet implementation; wet G11 composite/schema/dispatch/old-new map hooks separately versioned before enabling',
           'frame':'one lease/token for S/F/P/H/J/derived/foreground, storage+vertex consumer barriers'},
 'milestones':{'M0':'profile/config/seed/H/J/origins/hydro/energy/cold +dry reduction, not bidirectional function',
               'M1':'all32 wet passes/source16/sand-water actual reaction/root replay/complete new shader; consumes M0 and actual G9-DM backend',
               'M2':'independent C01-C20 device and effect evidence; not developer close condition'},
 'unmeasured':['actual GPU source/link/resource capability','standard wet-case advance rate/energy refusals',
               '250ms/runtime','allocation fit including cold','water optics/material thresholds'],
 'handoff':'developer implementation/compile/commit only, no tests/device; model/energy counterexample returns designer, no phi0 or deleted reaction'}
emit('architecture-T567-g12-engineering-v1.json',manifest)

# Supplied static volume algebra, not a time integrator or independent physics oracle.
geo_cases=0
for intervals in [[],[(0,8)],[(4,8)],[(0,4),(8,14)],[(0,4),(4,8)]]:
    def c8(z): return 2*z-sum(max(0,min(z,b)-a) for a,b in intervals if z>a)
    for h in range(c8(32)//2+1):
        lo,hi=0,32;iterations=0
        while lo<hi:
            mid=(lo+hi)//2
            if c8(mid)>=2*h:hi=mid
            else:lo=mid+1
            iterations+=1
        assert c8(lo)==2*h and iterations<=31
        overlap=sum(max(0,min(lo,b)-a) for a,b in intervals if lo>a)
        assert overlap%2==0 and 0<=overlap//2<=h
        geo_cases+=1
for event in events:
    used=[x['word'] for x in event['words'] if not x['name'].startswith('reserved')]
    assert [sum(1<<(i%32) for i in used if i//32==h) for h in [0,1]]==event['usedMask']
# Dimensional scale reductions (each Q shift represented by an integer exponent).
assert 20+16+22-16==42             # wave/Cface/head -> pW scale
q=8; g=3; eta=5
assert Fraction((g*q)*(4*q*eta)**2,8)==Fraction(g*eta**2,2)*(4*q**3)
assert ((1<<22)*(2*(1<<30)*(1<<60))) < 1<<114
maxr=resources[-1]
emit('analysis-T567-engineering-checks-v1.json',{
 'schema':'T567_FINITE_ENGINEERING_AUDIT_V1','result':'FINITE_LAYOUT_ALGEBRA_PASS_NOT_PRODUCT_PASS',
 'configWords':len(config_names),'GPUConfigWords':192,'waterHeaderWords':len(water_header),
 'staticVolumeCases':geo_cases,'newEventKinds':len(events),'resourceCombinations':len(resources),
 'dispatch':manifest['dispatch'],'maxResource':maxr,
 'checks':['all four profiles exactly match design formulas','reference192/64 and512/128 exact',
           'contiguous nonoverlapping state/work segments','full32 writer table and6204 pass table',
           'used masks from exact64word dictionaries','Cface Q16 conversion explicit','114bit drag numerator',
           'C8 exact static lattice','FULL_CORE versus six-trial FULL cost explicit'],
 'deviceExecuted':False,'CPUPhysicsAdded':False,'productPASS':False})

# T570: preserve exact raw causal scope. Do not infer API/callback causes from UI text.
base='test/reports/t491/t408vA/'
summaries=[]
for n in ['prep-33.json','prep-35.json','prep-36.json','prep-37.json','prep-38.json','prep-40.json','prep-41.json','prep-74.json']:
    obj=read(base+n);r=obj['readiness'];ev=[json.loads(e) if isinstance(e,str) else e for e in r['events']]
    snaps=[json.loads(e['value']) for e in ev if e['kind']=='SNAPSHOT']
    summaries.append(dict(file=receipt(base+n),state=r['state'],reason=r['readinessReason'],
        prepareId=r['prepareId'],page=r['pageGeneration'],observationPage=(obj['observation'] or {}).get('pageGeneration'),
        serial=r['observationSerial'],permitIsNull=r['permit'] is None,keyboardAreaSeen=r['keyboardAreaSeen'],
        sessionEverEdited=r['sessionEverEdited'],kbCfg=r['snapshot']['keyboardConfiguration'],
        APIs=[dict(api=a['api'],code=a['code']) for a in r['api']],eventKinds=dict(Counter(e['kind'] for e in ev)),
        changes=[e['value'] for e in ev if e['kind']=='CHANGE'],
        owners=[{k:s[k] for k in ['run','context','controller','session','surface','geometry','windowId','windowGeneration','nativeReady']} for s in snaps]))
p40=next(s for s in summaries if s['file']['path'].endswith('prep-40.json'))
p74=next(s for s in summaries if s['file']['path'].endswith('prep-74.json'))
assert p40['owners'][0]['run']!=p40['owners'][1]['run']
assert p40['owners'][0]['nativeReady']
assert p74['kbCfg']=='UNKNOWN' and p74['keyboardAreaSeen'] and p74['sessionEverEdited']
assert all(s['permitIsNull'] for s in summaries)
assert 'INPUT' not in p40['eventKinds']
source_root=ROOT/'.codex/local-test/t540-final'
generated_manifest=read('.codex/local-test/t540-final/t540-manifest.json')
normalized_files={k.replace('\\','/'):v for k,v in generated_manifest['files'].items()}
generated_matches=[]
for name in ['pages/P051DerivedV2Demo.ets','render/PrepareReadiness.ets','render/T413Capture.ets']:
    actual=hashlib.sha256((source_root/'entry/src/main/ets'/name).read_bytes()).hexdigest()
    assert actual==normalized_files[name],name
    generated_matches.append(dict(path=name,sha256=actual,generatedManifestMatches=True))
readiness=(source_root/'entry/src/main/ets/render/PrepareReadiness.ets').read_text(encoding='utf-8')
capture=(source_root/'entry/src/main/ets/render/T413Capture.ets').read_text(encoding='utf-8')
assert "if(this.state==='READY')this.invalidate(reason);else this.invalidateHold(reason);" in readiness
assert "if(s.state!=='RESERVED'&&s.state!=='HOLD_OPEN'&&s.state!=='SEALED')return;" in capture
emit('analysis-T570-raw-causality-v1.json',{
 'schema':'T570_RAW_CAUSALITY_AUDIT_V1','result':'SOURCE_AND_RAW_SCOPE_AUDIT_COMPLETE_NOT_DEVICE_PASS',
 'summaries':summaries,'sourceReceipts':[receipt(p) for p in [
   'test/t426/PrepareReadiness.ets','test/t455/PrepareFrameObservation.ets','test/t455/pump.inc',
   'test/t540/overlay.py','test/t540/page_raster.inc','test/t540/RasterGeometry.ets',
   '.codex/local-test/t540-final/entry/src/main/ets/render/PrepareReadiness.ets',
   '.codex/local-test/t540-final/entry/src/main/ets/pages/P051DerivedV2Demo.ets',
   '.codex/local-test/t540-final/entry/src/main/ets/render/T413Capture.ets']],
 'layoutReceipts':[receipt(base+n) for n in ['lI.json','lJ.json','lK.json','lM.json','lP.json','lQ.json','lR.json']],
 'reportReceipt':receipt('docs/test-report-T408-ime-domain-v10.md'),
 'generatedManifestReceipt':receipt('.codex/local-test/t540-final/t540-manifest.json'),
 'generatedSourceMatches':generated_matches,
 'sourceVsInstalledETS':'local generation manifest matched; this task does not independently unpack/reverse installed Ark bytecode',
 'findings':['prep40 native run actually changes while prior nativeReady; hard stale gate is valid',
             'prep41 includes HIDE/page2 with observer page1; not a ready permit invalidated solely by blur',
             'prep74 UNKNOWN fixed-keyboard declaration with observed keyboard history and stable later serial',
             'later confirmed-keyboard STALE lacks one-to-one full cold witness in present directory'],
 'notProven':['specific callback causing post-confirmation attempt stale','all prepare paths permanently fail'],
 'noDevice':True})
states=['PREPARING','READY'];kind=['SOFT','HARD'];matrix=[]
for state in states:
    for change in kind:
        after='PREPARING' if state=='PREPARING' and change=='SOFT' else 'INVALIDATED'
        matrix.append(dict(before=state,change=change,after=after,permitSurvives=False,deadlineReset=False))
assert 2*16384+64*256+8*256+2048==53248
emit('architecture-T570-prepare-contract-v1.json',{
 'schema':'T570_PREPARE_TRANSACTION_V4','status':'ENGINEERING_READY_CAUSAL_LIMITS_EXPLICIT',
 'deadlineMs':3000,'deadlineOrigin':'user begin app monotonic; no restart/renew','transitionMatrix':matrix,
 'subphases':['DISMISSING','SETTLING','VERIFY_PAIR'],
 'hardOwner':['page','windowId','windowGeneration','controller','session','surface','context','run'],
 'softBeforeReady':['input focus/edit','avoid/keyboardHeight','layout/config values'],
 'stableFrame':'original complete predicate, same owner and semantic snapshot, two increasing tickets each independently willDraw sampled',
 'READY':'serial/layout/input/owner changes revoke; synchronous fresh check ticket then existing reserve; consume only on RESERVED in same stack, no second stopEditing',
 'witness':{'payloadBytesMax':53248,'jsonBytesMax':65536,'records':64,'recordBytes':256,
            'snapshotCount':2,'snapshotBytes':16384,'APIRecords':8,'APIRecordBytes':256,'headerBytes':2048,
            'credit':'existing2MiB writer/4MiB total; overflow complete INCOMPLETE; first cause never overwritten'},
 'evidenceGap':'post-fixed-keyboard-confirmation failed attempt exact first invalidator/owner before-after',
 'deviceExecuted':False,'productPASS':False})
print(json.dumps({'T567':'finite ABI/resource audit complete','T570':'raw/source causal audit complete',
                  'wetDispatch':len(wet),'resources':len(resources),'geometryCases':geo_cases},ensure_ascii=False))
