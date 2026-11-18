"""T607 finite engineering generator/checks. No fluid evolution, build or device run.

Only schemas, byte ranges, fixed submission tables and mathematical constants.
T606/T609 must independently derive physical expectations.
"""
from pathlib import Path
from fractions import Fraction as F
import hashlib,json,math,re
R=Path(__file__).resolve().parents[1];D=R/'docs'
def read(n):return json.loads((D/n).read_text(encoding='utf-8-sig'))
def emit(n,x):(D/n).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(x):return json.dumps(x,ensure_ascii=False,sort_keys=True,separators=(',',':')).encode()
def receipt(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=sha(b))
def code(s):return int.from_bytes(s.encode('ascii'),'little')
def words(n,groups):
 out=['reserved0_'+str(i) for i in range(n)];used=set()
 for start,names in groups:
  for i,name in enumerate(names):
   at=start+i;assert 0<=at<n and at not in used;out[at]=name;used.add(at)
 return out
def u64(n):return [n+'.lo',n+'.hi']
def hash8(n):return [n+'.'+str(i) for i in range(8)]
def rng(name,start,count):return dict(name=name,wordStart=start,words=count,byteStart=start*4,bytes=count*4)

design=read('design-G13-pure-fluid-manifest-v1.json')
config=words(128,[(0,['magic','schema','profile','lengthWords','Nx','Ny','boundary','seedMode',
 'LxF32','LyF32','hF32','T0F32']),(12,u64('L0Binary64')+u64('rho0Binary64')+u64('muSIBinary64')+['nuF32','unitPolicy']),
 (20,['motionMode','OmF32','durationF32','theta0F32','gxWorldF32','gyWorldF32','forceAmpF32','forceKind',
 'tracerCount','positiveWorkCapF32','precisionPolicy','gatesRevision','wallBottomF32','wallTopF32','pressureReferenceF32']),
 (36,hash8('algorithmSHA')+hash8('layoutSHA')+hash8('gatesSHA')+hash8('seedSHA')+hash8('fixtureSHA')+hash8('configSHA'))])
metrics=['KwindowStart','K','WEuler','Wnonpotential','Wwall','Wpositive','Dadv','Dnu','DBE','Dprojection',
 'Rdualdiv','RH','RP','RrhsQuantization','Rfloat','windowAbsWork','windowDissipation','Escalefrozen',
 'momentumClosureX','momentumClosureY','CFLincoming','CFLoutgoing','divergenceRMS','divergenceMax']
state=words(256,[(0,['magic','schema','profile','lengthWords','Nx','Ny','boundary','status']),
 (8,sum((u64(x) for x in ['run','context','surface','geometry']),[])),
 (16,u64('commit')+u64('acceptedQuantum')+['selectedJ','rootKind']+u64('sourceSeq')),
 (24,u64('motionOriginQuantum')+['theta0F32','OmF32','durationF32','forceAmpF32','motionMode','motionFlags']),
 (32,sum((hash8(x) for x in ['configSHA','algorithmSHA','layoutSHA','gatesSHA','seedSHA','inputSHA']),[])),
 (80,sum(([n+'.valueF32',n+'.errorBoundF32'] for n in metrics),[])),
 (128,['viscBlock','viscIterations','pressureCycle','pressureSweeps','rHRMSF32','rHMaxF32']+
   u64('rPMaxQ32')+[f'rPSumSquares128.{i}' for i in range(4)]+u64('piMeanSumQ32')+
   ['lambdaLowerBoundF32','etaBF32','pressureCertificateF32','velocityCertificateF32',
    'projectionIdentityMaxF32','thetaWrappedF32','OmegaF32','alphaF32','phiMeanF32','pressureReferenceF32',
    'mathBackend','firstFailedOp','firstFailure','activeMicro','activeBlocks','activeCycles','noopDispatches','qualityFlags']),
 (160,[f'wall{wall}.{x}' for wall in range(4) for x in
 ['pressureImpulseXF32','pressureImpulseYF32','viscousImpulseXF32','viscousImpulseYF32','workF32','meanTotalPressureF32','flags','reserved0']]),
 (192,u64('consumedSourceSeq')+u64('sourceControlGeneration')+u64('sourceIntent')+u64('forceStartQuantum')+u64('forceEndQuantum')+
   ['gxWorldF32','gyWorldF32','forceKind','forceAmpF32','motionMode','dedupFlags']+u64('sourceSampleNativeNs')+
   u64('acceptedCommandSerial')+hash8('sourceInputSHA')+u64('motionOriginTick')+['controlStatus','reserved0'])])
inp=words(128,[(0,['magic','schema','lengthWords','kind']),
 (4,sum((u64(x) for x in ['run','context','surface','geometry','configGeneration','pageSession','permission',
 'controlGeneration','commandSeq','sampleNativeNs','baseCommit','acceptedQuantum','motionOriginQuantum']),[])),
 (30,['diagnosticMode','forcedJ']),
 (32,sum((hash8(x) for x in ['configSHA','algorithmSHA','layoutSHA','gatesSHA','seedSHA','fixtureSHA','inputSHA']),[])),
 (88,['sourceOpcode','motionMode','theta0F32','OmF32','durationF32','gxWorldF32','gyWorldF32','forceKind','forceAmpF32']+
   u64('forceStartQuantum')+u64('forceEndQuantum')+['wallBottomF32','wallTopF32','boundaryRevision']+
   u64('sourceIntent')+u64('enqueueSerial')),(127,['seal'])])
tail=words(256,[(0,['magic','schema','profile','lengthWords','rootKind','outcome','firstErrorOp','firstError']),
 (8,sum((u64(x) for x in ['nonce','plan','run','owner','context','surface','geometry','configGeneration',
 'baseCommit','proposedCommit','startedNativeNs','deadlineNativeNs','completedNativeNs','acceptedQuantum']),[])),
 (36,['selectedJ','requiredAuditMask','actualAuditMask','actualDispatches','activeDispatches','noopDispatches',
 'publicationEligible','traceSealedMask']+u64('consumedSourceSeq')+u64('sourceGeneration')),
 (48,sum((hash8(x) for x in ['configSHA','algorithmSHA','layoutSHA','gatesSHA','seedSHA','inputSHA','fieldSHA']),[])),
 (104,['rHRMSF32','rHMaxF32','divergenceRMSF32','divergenceMaxF32','pressureCertificateF32','velocityCertificateF32',
 'projectionIdentityMaxF32','RfloatF32','energyBudgetF32','momentumClosureXF32','momentumClosureYF32','positiveWorkF32']),
 (116,u64('renderGeneration')+u64('rootTraceGeneration')+['candidateBank','renderPackBank','failureScope','failureTrial','failureMicro','mathBackend','diagnosticMode','precisionFlags']),
 (128,sum((u64(x) for x in ['pageSession','permission','inputEpoch','controlGeneration']),[])),(255,['seal'])])
tail[136:146]=['thetaWrappedF32','OmegaF32','alphaF32','gxBoxF32','gyBoxF32','motionEnded','forceOn','sourceControlValid']+u64('motionOriginQuantum')
micro=words(1024,[(0,['magic','schema','profile','lengthWords','scope','trialJ','microOrdinal','status']+
 sum((u64(x) for x in ['nonce','plan','baseCommit','acceptedTickBefore','acceptedTickAfter','sourceSeq']),[])+
 ['deltaDenominator','firstErrorOp','firstError','completedFixedPassMask']),
 (32,hash8('inputSHA')+hash8('configSHA')+hash8('algorithmSHA')+hash8('layoutSHA')+hash8('gatesSHA')),
 (72,['thetaF32','OmegaF32','alphaF32','gxF32','gyF32','forceStartFlag','positiveWorkF32','CFLincomingF32',
 'CFLoutgoingF32','uMaxF32','vMaxF32','hF32','nuF32','lambdaLowerBoundF32','etaBF32','mathBackend']),
 (128,[f'pressureCycle{c}.{x}' for c in range(16) for x in
 ['ordinal','flags','converged','fault','rSquared0','rSquared1','rSquared2','rSquared3','rMaxLo','rMaxHi',
 'sumBLo','sumBHi','gaugeMeanLo','gaugeMeanHi','activeSweeps','seal']]),
 (384,[f'viscBlock{c}.{x}' for c in range(8) for x in ['ordinal','flags','converged','fault',
 'trueRMSF32','trueMaxF32','lastIteration','lambdaMaxF32','rhoF32','alphaF32','betaF32','nuF32','energyResidualF32','reserved0','activeIterations','seal']]),
 (512,[f'stage{s}.{x}' for s in range(32) for x in ['opcode','flags','firstError','activeElements','CFLmaxF32','valueMaxF32','scalarF32','seal']]),
 (768,[f'motion.{i}' for i in range(64)]),(832,[f'energy.{i}' for i in range(64)]),
 (896,[f'momentumAndWalls.{i}' for i in range(64)]),(960,[f'compatibilityAndProof.{i}' for i in range(32)]),(1023,['seal'])])
renderCell=['uCenterF32','vCenterF32','piF32','phiF32','totalPressureF32','curlF32','divergenceF32','speedF32',
 'xF32','yF32','viscResidualProxyF32','pressureResidualF32','validFlags','cellId','boundaryFlags','reserved0']
micro[768:832]=words(64,[(0,['thetaWrappedF32','OmegaF32','alphaF32','gxF32','gyF32','phiMeanF32','pressureReferenceF32',
 'forceAmpF32','deltaF32','nuF32','hF32','wallBottomF32','wallTopF32','positiveWorkCapF32','forceOn','mathStatus']),
 (16,[f'{n}.binary128.{i}' for n in ['theta','Omega','alpha','gx','gy'] for i in range(4)]),
 (36,u64('tickBefore')+u64('tickAfter')),(40,hash8('mathSHA'))])
micro[832:896]=words(64,[(0,sum(([n+'.valueF32',n+'.errorBoundF32'] for n in metrics),[])),
 (48,['windowAcceptedQuanta','flags']+u64('windowFirstQuantum')+u64('windowLastQuantum')+
 ['externalErrorBoundF32','priorEscalefrozenF32','preKickKF32','postKickKF32','floatBudgetF32','ledgerFlags'])])
micro[896:960]=words(64,[(0,[f'wall{w}.{x}' for w in range(4) for x in
 ['pressureJxF32','pressureJyF32','viscJxF32','viscJyF32','workF32','meanPressureF32','flags','reserved0']]),
 (32,[f'{n}.{a}F32' for n in ['body','coriolis','viscous','pressure','normalConstraint','advection','phiTraction',
 'deltaP','closure','sumAbsJ'] for a in ['x','y']])])
micro[960:992]=words(32,[(0,u64('sumBBeforeQ32')+u64('sumBAfterQ32')+['maxCorrectionLSB','correctionCount']+
 u64('piMeanSumQ32')+[f'rPSquared128.{i}' for i in range(4)]+u64('rPMaxQ32')+
 ['etaBF32','lambdaLowerBoundF32','pressureCertificateF32','velocityCertificateF32','projectionIdentityF32',
 'proofFlags','tightPressureToleranceF32','hInverse','J','roundPolicy','diagnosticReason','actualDivRMSF32','actualDivMaxF32'])])
rootTrace=words(1024,[(0,['magic','schema','profile','lengthWords','scope','status','rootKind','flags']+
 u64('nonce')+u64('plan')+u64('baseCommit')+['selectedJ','firstFailure']+hash8('inputSHA')+hash8('configSHA')),
 (64,[f'trial{j}.{x}' for j in [1,2,4] for x in words(64,[(0,['J','status','firstErrorOp','firstError',
 'attemptedMicro','sealedMicro','selected','requiredMicro','activeDispatches','noopDispatches','positiveWorkF32','floatRemainderF32']+
 u64('baseCommit')+u64('proposedCommit')),(16,hash8('inputSHA')+hash8('fieldSHA')),(63,['seal'])])]),
 (256,['summary.'+x for x in tail]),(512,['input.'+x for x in inp]),(640,['config.'+x for x in config]),
 (768,[f'mathQualification.{i}' for i in range(128)]),(896,[f'quotaAndClock.{i}' for i in range(64)]),(1023,['seal'])])
schemas=dict(Config128=config,StateHeader256=state,Input128=inp,Completion256=tail,MicroTrace1024=micro,
 RootTrace1024=rootTrace,RenderCell16=renderCell,Tracer8=design['layout']['tracerWords'],
 common=dict(U64='little-endian low/high U32, bridge decimal string',F32='raw IEEE binary32 bits',
 S64Q32='signed two-complement low/high, difference before conversion',hash='SHA256 raw bytes; each4 digest bytes LE word',
 unknown='reject unknown revision/flags; all reserved0 and absent records zero',seal=0xc0de1301))
schemas['RenderHeader64']=words(64,[(0,['magic','schema','profile','lengthWords','Nx','Ny','boundary','flags']),
 (8,sum((u64(n) for n in ['run','context','surface','geometry','commit','nonce','packGeneration','configGeneration']),[])),
 (24,hash8('configSHA')+hash8('layoutSHA')),(40,['uColorMinF32','uColorMaxF32','piColorMinF32','piColorMaxF32',
 'curlColorMinF32','curlColorMaxF32','thetaWrappedF32','phiMeanF32','pressureReferenceF32','displayMode']),
 (63,['seal'])])
schemas['ColdDescriptor32']=words(32,[(0,['kind','schema','segmentIndex','flags','bankRole','strideWords']+
 u64('byteOffset')+u64('byteLength')+['validRows','plannedRows','sealedRows']+u64('commit')+u64('nonce')+
 hash8('rawSHA')+['unitKind','fieldLayoutRevision']),(31,['seal'])])
schemas['ColdHeader256']=words(256,[(0,['magic','schema','profile','headerWords','mode','flags','segmentCount','pageCount']),
 (8,sum((u64(n) for n in ['traceId','requestId','createdNativeNs','run','context','surface','geometry','commit','nonce','rawBytes']),[])),
 (32,sum((hash8(n) for n in ['algorithmSHA','layoutSHA','gatesSHA','mathSHA','configSHA','seedSHA','inputSHA','fixtureSHA']),[])),
 (96,['Nx','Ny','boundary','rootKind','outcome','firstError','rootTailBytes','diagnosticMode']),
 (248,hash8('canonicalPrefixSHA_selfZeroed'))])
schemas['NativePermit128']=words(128,[(0,['schema','rootKind','flags','candidateBank','replayBank','renderPackBank','sourceHead','requiredAuditMask']),
 (8,sum((u64(n) for n in ['nonce','plan','run','owner','context','surface','geometry','configGeneration','pageSession',
 'permission','inputEpoch','controlGeneration','baseCommit','proposedCommit','startedNativeNs','deadlineNativeNs']),[])),
 (40,sum((hash8(n) for n in ['configSHA','algorithmSHA','layoutSHA','gatesSHA','inputSHA','acceptedBaseIdentitySHA']),[])),
 (88,u64('acceptedQuantum')+u64('sourceSeq')+['diagnosticMode','forcedJ','viewRebind','precisionFlags']),
 (127,['seal'])])
schemas['ViewPolicy64']=words(64,[(0,['magic','schema','lengthWords','mode','speedMinF32','speedMaxF32','piMinF32','piMaxF32',
 'curlMinF32','curlMaxF32','totalPressureMinF32','totalPressureMaxF32','tracerRadiusF32','arrowScaleF32','lineWidthF32','flags']),
 (16,hash8('configSHA')+hash8('viewSHA_selfZeroed')+hash8('layoutSHA')),(63,['seal'])])
schemas['MathQualification128']=words(128,[(0,['schema','backend','qualificationFlags','sampleCount']+
 hash8('mathSHA')+hash8('programSHA')+hash8('deviceBuildSHA')),
 (32,[f'sample{i}.{x}' for i in range(16) for x in ['functionId','argumentBinary64Lo','argumentBinary64Hi',
 'resultF32','referenceF32','errorBoundF32']])])
schemas['QuotaAndClock64']=words(64,[(0,sum((u64(n) for n in ['session','sequence','uiNs','nativeReceiveNs','pulses',
 'maxUiGapNs','maxNativeGapNs','generated','accepted','overload','revoked','carry','clearedCarry','planSerial',
 'pending','activeRemaining','sourceBoundary','sourceIntent','sourceControlGeneration','meaningfulInputSeq',
 'retiredPlan','retiredNonce','nativePollNs','rootSubmitNs','rootDeadlineNs','rootCompletionPollNs',
 'sceneNewQuantumFrames','sceneSourceFrames','sceneRepeatFrames','sceneStaleFrames','clockFault','flags']),[]))])
schemas['TaggedReceiptHeader16']=words(16,[(0,['tag','schema','encoding','wordLength','usedBytes','flags']+
 hash8('payloadSHA')),(15,['seal'])])
schemas['GpuGlobal256']=words(256,[(0,['rootStatus','firstErrorOp','firstError','scope','trialJ','micro',
 'selectedJActual','selectedJReplay','rootKind','activeMask','activePasses','noopPasses','pressureFrozen','viscosityFrozen','scopeFailed','traceMicroIndex']),
 (16,sum((u64(n) for n in ['sourceSeq','nonce','plan','baseCommit','proposedCommit','baseQuantum','proposedQuantum','controlGeneration']),[])),
 (32,sum(([n+'.valueF32',n+'.errorBoundF32'] for n in metrics),[])),
 (80,sum((u64(n) for n in ['tickBefore','tickAfter','sourceIntent','sourceBoundary','parameterGeneration','rootTraceGeneration',
 'renderGeneration','scopeNonce','failedTrialNonce','windowFirstQuantum','windowLastQuantum','windowAcceptedQuanta',
 'sumBPreQ32','sumBPostQ32','piMeanSumQ32','pressureMaxRawQ32']),[])),
 (112,['drive.'+str(i) for i in range(32)]),(144,['proof.'+str(i) for i in range(32)]),
 (176,['momentumAndWalls.'+str(i) for i in range(32)]),(208,['conversionAndProbe.'+str(i) for i in range(32)]),
 (240,['scheduleIndex','requiredPasses','requiredAuditMask','actualAuditMask','traceSealedMask','rootComplete',
 'candidateBank','replayBank','renderPackBank','viewRebind','diagnosticMode','forcedJ','precisionPolicy','mathBackend','flags','seal'])])
rootTrace[768:896]=['mathQualification.'+x for x in schemas['MathQualification128']]
rootTrace[896:960]=['quotaAndClock.'+x for x in schemas['QuotaAndClock64']]
profiles=[]
for gp in design['support']['gridProfiles']:
 nx,ny=gp['Nx'],gp['Ny'];nc=nx*ny;nu=(nx+1)*ny;nv=nu+nx*(ny+1);levels=[];a,b=nx,ny;sumc=0
 while True:
  levels.append(dict(Nx=a,Ny=b,cells=a*b,packedCellStart=sumc));sumc+=a*b
  if min(a,b)==4:break
  a//=2;b//=2
 r=(max(nc,nv)+63)//64
 stateRanges=[rng('header',0,256),rng('u',256,nu),rng('v',256+nu,nx*(ny+1)),rng('piQ32',256+nv,2*nc),rng('tracers',256+nv+2*nc,8*4096)]
 work=[];at=0
 for name in design['layout']['faceArrays']:work.append(rng(name,at,nv));at+=nv
 for name in design['layout']['MGArrays']:work.append(rng(name,at,2*sumc));at+=2*sumc
 for name in design['layout']['cellArrays']:work.append(rng(name,at,nc));at+=nc
 work.append(rng('partial16',at,16*r));at+=16*r
 work.append(rng('coefficientsAndPersistentControl',at,4096));at+=4096
 sb=4*(256+nv+2*nc+32768);wb=4*at;pack=4*(64+16*nc);resident=3*sb+wb+74752+2*pack
 item=dict(Nx=nx,Ny=ny,diagnosticOnly=gp.get('diagnosticOnly',False),h=gp['Lx']/nx,Nc=nc,Nu=nu,Nv=nv,MG=levels,
  MGcells=sumc,reductionGroups=r,state=stateRanges,work=work,stateBytes=sb,workBytes=wb,renderPackBytes=pack,
  CTRL=[rng('controlAndConfig',0,1024),rng('originalInputCapsule',1024,1024),rng('traceActual',2048,8192),rng('traceReplay',10240,8192),rng('completion',18432,256)],
  traceScope=[rng('rootAnd3TrialRecords',0,1024)]+[rng(f'micro{i}',1024+1024*i,1024) for i in range(7)],
  residentBytes=resident,ROOTColdBytes=65536+2*sb+74752,FULLColdBytes=65536+resident,
  pressureSweepsPerV=8*len(levels)+28)
 for mode,segments in [('ROOT',[('PREFIX',65536),('BASE',sb),('CANDIDATE',sb),('CTRL',74752)]),
  ('FULL',[('PREFIX',65536),('BASE',sb),('CANDIDATE',sb),('REPLAY',sb),('WORK',wb),('CTRL',74752),('PACK0',pack),('PACK1',pack)])]:
  pos=0;directory=[]
  for index,(name,size) in enumerate(segments):
   directory.append(dict(index=index,kind=index+1,name=name,byteOffset=pos,byteLength=size,strideWords=1))
   pos+=size
  assert pos==item[mode+'ColdBytes'];item[mode+'ColdDirectory']=directory
 for key in ['state','work','CTRL','traceScope']:
  ranges=item[key];assert ranges[0]['wordStart']==0
  assert all(x['wordStart']+x['words']==y['wordStart'] for x,y in zip(ranges,ranges[1:]))
 assert item['CTRL'][-1]['byteStart']+item['CTRL'][-1]['bytes']==74752
 old=next((p for p in design['referenceResources'] if p['Nx']==nx and p['Ny']==ny),None)
 if old:
  assert (sb,wb,2*pack,resident,item['ROOTColdBytes'],item['FULLColdBytes'])==tuple(old[k] for k in ['stateBytes','workBytes','renderPairBytes','residentBytes','ROOTColdBytes','FULLColdBytes'])
 profiles.append(item)
layout=dict(schema='T607_G13_LAYOUT_V1',revision=1,profile='G13_MAC_FILLED_V1',
 magic={n:code('G13'+c) for n,c in [('profile','F'),('config','C'),('state','S'),('input','I'),('trace','T'),('tail','R'),('render','V'),('cold','K'),('view','W')]},
 schemas=schemas,profiles=profiles,computeBinding={0:'BASE read-only',1:'CANDIDATE or REPLAY',2:'WORK',3:'CTRL_INPUT_TRACES_COMPLETION'},
 bindingOverrides={'ROOT_SCOPE_COMPARE':{0:'CANDIDATE',1:'REPLAY',2:'WORK',3:'CTRL'},
 'PACK_VIEW_AND_AUDIT':{0:'selected CANDIDATE readonly',1:'candidate RenderPack',2:'WORK',3:'CTRL'}},
 enums=dict(boundary={'CLOSED_NOSLIP':1,'TEST_PERIODIC_XY':2,'TEST_PERIODIC_X_CHANNEL':3},
 seed={'REST':0,'ANALYTIC_FIXTURE':1,'AUDITED_COLD_FILE':2,'PRIVATE_PROJECT_SCRATCH':3},
 view={'VELOCITY':0,'TRACERS':1,'VORTICITY':2,'DYNAMIC_PRESSURE':3,'TOTAL_PRESSURE':4},
 motion={'REST':0,'GRAVITY_CHANGE_ONLY':1,'RAMP_SPIN':2,'COROTATING_TEST':3},
 source={'SET_GRAVITY':1,'START_RAMP':2,'SET_FORCE_INTERVAL':3},
 outcome={'PENDING':0,'ACCEPTED':1,'NUMERIC_BLOCKED':2,'FAULT':3,'CANCELLED':4,'STALE':5,'DEADLINE':6,'DIAGNOSTIC_COMPLETE':7,'ALREADY_CONSUMED':8},
 phaseFlags={'NOT_EXECUTED':0,'ACTIVE':1,'SUBMITTED_NOOP_SELECTED':2,'SUBMITTED_NOOP_CONVERGED':4,'FAILED_SEALED_PREFIX':8,'NOT_APPLICABLE':16,'SEALED_BIT':32},
 auditBits={n:i for i,n in enumerate(['BASE_IDENTITY','FIELD_RANGE','NORMAL_WALL','RHS_COMPATIBILITY','VISC_TRUE_RESIDUAL',
 'PRESSURE_CERTIFICATE','PRESSURE_GAUGE','PROJECTION_IDENTITY','ACTUAL_DIVERGENCE','POSITIVE_WORK_CAP','ENERGY',
 'MOMENTUM','TRACER','FULL_REPLAY_COMPARE','PACK_FIELD_MATCH','SOURCE_CONTROL','ZERO_REST_CONDITIONAL',
 'CONFIG_INPUT_HASH','SOURCE_ZERO_TIME','FIXED_FILLED_GEOMETRY'])},
 errors={n:i for i,n in enumerate(['NONE','BAD_HEADER','BAD_SCOPE','BAD_HASH','UNSUPPORTED_MODEL','CONFIG_RANGE',
 'NAPI_DECODE','CLOCK_GAP','CLOCK_REVERSED','DEADLINE','CONTEXT_LOST','CANCELLED','CFL_DOMAIN_BLOCKED',
      'VISCOSITY_NO_CONVERGENCE','PRESSURE_NO_CONVERGENCE','PRESSURE_RESOLUTION_LIMIT','NONFINITE','NUMERIC_OVERFLOW',
 'REPLAY_MISMATCH','BAD_RHS_COMPATIBILITY','PRESSURE_GAUGE','PROJECTION_IDENTITY','DIVERGENCE_GATE','WORK_CAP_BLOCKED',
 'ENERGY_GATE','MOMENTUM_GATE','VISUAL_DOMAIN_ERROR','MATH_QUALIFICATION_REQUIRED','META_INCOMPLETE','CAPABILITY_UNSUPPORTED',
 'RESOURCE_LIMIT','STALE_INPUT','IO_ERROR','TRACE_EXPIRED','COLD_WOULD_BLOCK','FIELD_RANGE','CONTROL_SEQUENCE_EXHAUSTED','MISSING_REQUIRED_AUDIT'])}),
 ctrlInternal=[rng('config128',0,128),rng('nativePermit128',128,128),rng('quotaSnapshot256',256,256),
 rng('gpuGlobal256',512,256),rng('trialFlags128',768,128),rng('schedule64',896,64),rng('reserved0',960,64)],
 inputCapsule=[rng('receipt64',0,64),rng('originalInput128',64,128),rng('canonicalInput128',192,128),
 rng('fixtureDescriptor128',320,128),rng('seedDescriptor128',448,128),rng('boundaryDescriptor128',576,128),
 rng('motionReceipt128',704,128),rng('config128',832,128),rng('archiveMeta64',960,64)],
 coldPrefix=[rng('header256',0,256),rng('descriptors32x32',256,1024),rng('config128',1280,128),
 rng('inputCapsule1024',1408,1024),rng('capability512',2432,512),rng('mathAndParameterReceipt512',2944,512),
 rng('compileReceipt1024',3456,1024),rng('sceneReceipt256',4480,256),rng('quotaSnapshot256',4736,256),
 rng('buildReceipt512',4992,512),rng('diagnostic256',5504,256),rng('rootTicket256',5760,256),rng('reserved0',6016,10368)],
 faceIndex=dict(u='j*(Nx+1)+i',v='Nu+j*Nx+i',pi='j*Nx+i',
 canonicalClosed='u 1<=i<Nx; v 1<=j<Ny',canonicalPeriodicXY='u i<Nx; v j<Ny',canonicalChannel='u i<Nx; v 1<=j<Ny',
 alias='right u/top v copied only by canonical owner; all operator reads canonical wrap; excluded from sums'))
for name in ['ctrlInternal','inputCapsule','coldPrefix']:
 rs=layout[name];assert all(a['wordStart']+a['words']==b['wordStart'] for a,b in zip(rs,rs[1:]))
 assert rs[-1]['wordStart']+rs[-1]['words']==(16384 if name=='coldPrefix' else 1024)
layout['layoutSHA']=sha(canonical(layout));emit('architecture-T607-g13-layout-v1.json',layout)

base=design['microFixedPasses'];assert len(base)==32
microOps=[]
for n in base:
 microOps.append(n)
 if n=='VISC_SETUP':microOps.extend('VISC_BLOCK_'+str(i) for i in range(8))
 if n=='PRESSURE_INIT':microOps.extend('PRESSURE_V_'+str(i) for i in range(16))
assert len(microOps)==56
outer=['ROOT_INIT','ROOT_INPUT_GATE','ROOT_BASE_AUDIT','ROOT_WINDOW_SNAPSHOT','ROOT_ACTUAL_BEGIN','ROOT_REPLAY_BEGIN',
 'ROOT_SCOPE_COMPARE','ROOT_FINAL_FIELD_AUDIT','ROOT_TIME_PREPARE','PACK_VIEW','PACK_AUDIT','ROOT_COMPLETION_SEAL']
trial=['TRIAL_GATE','TRIAL_RESET_FROM_BASE','TRIAL_INIT','TRIAL_ENTRY_AUDIT']
sourceOps=['SOURCE_INIT','SOURCE_INPUT_GATE','SOURCE_BASE_AUDIT','SOURCE_DEDUP_GATE','SOURCE_ACTUAL_COPY',
 'SOURCE_ACTUAL_INSTALL','SOURCE_ACTUAL_AUDIT','SOURCE_REPLAY_COPY','SOURCE_REPLAY_INSTALL','SOURCE_REPLAY_AUDIT',
 'SOURCE_COMPARE','SOURCE_CONTROL_PREPARE','SOURCE_TIME_ZERO_AUDIT','PACK_VIEW','PACK_AUDIT','SOURCE_COMPLETION_SEAL']
names=list(dict.fromkeys(outer+trial+base+['VISC_BLOCK','PRESSURE_V']+sourceOps));opcodes={n:i for i,n in enumerate(names)}
plan=[]
def add(n,scope=0,j=0,m=0,k=0):plan.append(dict(index=len(plan),op=n,opcode=opcodes[n],scope=scope,J=j,micro=m,iterationBlock=k))
for n in outer[:5]:add(n)
for scope in [1,2]:
 if scope==2:add('ROOT_REPLAY_BEGIN')
 for j in [1,2,4]:
  for n in trial:add(n,scope,j)
  for m in range(j):
   for n in microOps:
    if n.startswith('VISC_BLOCK_'):add('VISC_BLOCK',scope,j,m,int(n.rsplit('_',1)[1]))
    elif n.startswith('PRESSURE_V_'):add('PRESSURE_V',scope,j,m,int(n.rsplit('_',1)[1]))
    else:add(n,scope,j,m)
for n in outer[6:]:add(n)
assert len(plan)==820
dispatch=dict(schema='T607_G13_DISPATCH_V1',opcodes=opcodes,quantum=plan,
 source=[dict(index=i,op=n,opcode=opcodes[n]) for i,n in enumerate(sourceOps)],
 microFixed=base,microExpanded=microOps,rootOuter=outer,trialInit=trial,
 persistent=dict(VISC_BLOCK=dict(groups=1,lanes=64,iterations=16,blockCount=8),
 PRESSURE_V=dict(groups=1,lanes=64,cycles=16,internalSweeps='8*MGlevels+28')),
 barrier='each producer dispatch API SHADER_STORAGE; copy BUFFER_UPDATE; draw VERTEX_ATTRIB; persistent memoryBarrierBuffer+barrier at every sweep',
 rootTailBytes=1024,rootFences=1,G1F4Dispatches=16+4*820,G1F4TailBytes=5120,
 skip='fixed submission table; all lanes uniform no-op after selection/freeze; never skip native table index')
emit('architecture-T607-g13-dispatch-v1.json',dispatch)

# Exact rational build-time constants; not designer's physical checker/oracle.
def atan_interval(q,n=256):
 x=F(1,q);s=F(0)
 for k in range(n):s+=(-1 if k%2 else 1)*x**(2*k+1)/(2*k+1)
 t=(-1 if n%2 else 1)*x**(2*n+1)/(2*n+1)
 return min(s,s+t),max(s,s+t)
a,b=atan_interval(5);c,d=atan_interval(239);pilo,pihi=16*a-4*d,16*b-4*c
lnlo=2*sum((F(1,3)**(2*k+1)/(2*k+1) for k in range(256)),F(0))
lnhi=lnlo+2*F(1,3)**513/F(513)/(1-F(1,9))
def floorq(x):return x.numerator//x.denominator
recipLo=floorq(F(2**513,1)/pihi);recipHi=floorq(F(2**513,1)/pilo);assert recipLo==recipHi
def rne(x):
 q,r=divmod(x.numerator,x.denominator);return q+int(2*r>x.denominator or (2*r==x.denominator and q%2))
def binary128(x):
 sign=int(x<0);x=abs(x)
 if not x:return [0,0,0,0]
 e=x.numerator.bit_length()-x.denominator.bit_length()
 if x<F(2)**e:e-=1
 sig=rne(x/F(2)**(e-112))
 if sig==2**113:e+=1;sig//=2
 assert -16382<=e<=16383
 bits=(sign<<127)|((e+16383)<<112)|(sig-2**112)
 return [(bits>>(32*i))&0xffffffff for i in range(4)]
assert binary128(pilo)==binary128(pihi)
assert binary128(lnlo)==binary128(lnhi)
def dyadic_upper(x):
 e=x.numerator.bit_length()-x.denominator.bit_length()
 if x>F(2)**e:e+=1
 assert e < -80
 return '2^'+str(e)
mathSpec=dict(schema='T607_G13_GPU_MATH_V1',backend='SOFT_BINARY128_CONSTANT_RANGE_REDUCTION',
 basic='U32 limbs; significand113; exponent15; checked U256 products; RNE guard/round/sticky; no native double extension',
 limbOrder='low-first',piBinary128=binary128(pilo),ln2Binary128=binary128(lnlo),twoOverPiScaled512=[(recipLo>>(32*i))&0xffffffff for i in range(16)],
 coefficientRationals=dict(sin=[str(F((-1)**k,math.factorial(2*k+1))) for k in range(13)],
 cos=[str(F((-1)**k,math.factorial(2*k))) for k in range(13)],exp=[str(F(1,math.factorial(k))) for k in range(21)]),
 coefficientBits=dict(sin=[binary128(F((-1)**k,math.factorial(2*k+1))) for k in range(13)],
 cos=[binary128(F((-1)**k,math.factorial(2*k))) for k in range(13)],exp=[binary128(F(1,math.factorial(k))) for k in range(21)]),
 reduce='integer reciprocal512 quadrant reduction; theta0 raw F32 reduced before addition; accepted U64 ticks exact /480 before drive evaluation',
 evaluate='soft113 Horner; sin degree25/cos degree24 on [-pi/4,pi/4]; exp degree20 after ln2 reduction',
 smallPhaseTheta='Om*T/(4*pi)*(x-sin(x)); for |x|<1/4 evaluate x^3*(1/3!-x^2/5!+...-x^22/25!) without cancellation',
 runtime='scalar drive lane0; finite spatial sin table GPU cold in existing4096word coeff area; no extra dispatch in root',
 requiredSinCosAbsError='2^-21',requiredExpRelError='2^-20 where output representable; explicit numeric fault otherwise',
 constantsProof='Machin alternating rational intervals256 terms each, identical binary128 pi and512 reciprocal floor; positive atanh1/3 series bound for ln2',
 truncationBounds={'sin25':dyadic_upper((pihi/4)**27/math.factorial(27)),
 'cos24':dyadic_upper((pihi/4)**26/math.factorial(26)),
 'exp20':dyadic_upper(2*(lnhi/2)**21/math.factorial(21))},
 testsRun=False)
mathSpec['mathSHA']=sha(canonical(mathSpec));emit('architecture-T607-g13-math-v1.json',mathSpec)
engineering=dict(schema='T607_G13_ENGINEERING_V1',date='2026-11-18',role='architect',task='T607',
 status='ENGINEERING_READY_NOT_IMPLEMENTED_NOT_VALIDATED',deviceUsed=False,productTestsRun=False,
 inputs=[receipt('docs/'+n) for n in ['algorithm-G13-pure-fluid-gpu-v1.md','design-G13-pure-fluid-manifest-v1.json',
 'design-G13-pure-fluid-fixtures-v1.json','architecture-T613-g13-reuse-boundaries-v1.md','architecture-T613-g13-reuse-manifest-v1.json']],
 tables=['architecture-T607-g13-layout-v1.json','architecture-T607-g13-dispatch-v1.json','architecture-T607-g13-math-v1.json'],
 hashes=dict(layoutSHA=layout['layoutSHA'],gatesSHA=sha(canonical(design['gates'])),mathSHA=mathSpec['mathSHA'],
 algorithmSHA=sha(canonical(dict(designMD=receipt('docs/algorithm-G13-pure-fluid-gpu-v1.md')['sha256'],
 designManifest=receipt('docs/design-G13-pure-fluid-manifest-v1.json')['sha256'],mathSHA=mathSpec['mathSHA'])))),
 gates=design['gates'],support=design['support'],goalOrder=[13,14,12],goal10Paused=True,
 unresolved='F05 physical pressure/splitting and all GPU qualification/performance unmeasured; linear residual is not a physical oracle',
 newFiles=['entry/src/main/cpp/gpu_resident/g13flow/'+n for n in ['contract_generated.h','layout_generated.glsl',
 'kernel_assets.cpp','pipeline.comp','mac_access.glsl','advection.glsl','viscosity.glsl','pressure_mg.glsl','wide_q32.glsl',
 'math_soft128.glsl','drive.glsl','ledger.glsl','trace.glsl','render_pack.glsl','controller.h','controller.cpp','transport.h',
 'transport.cpp','cold.h','cold.cpp','scene.h','scene.cpp','bridge.cpp','kernels.cmake','CMakeLists.txt']]+
 ['entry/src/main/ets/pages/GpuResidentG13.ets','entry/src/main/ets/physics/GpuG13Cold.ets',
  'entry/src/main/cpp/types/libgrs_g13/index.d.ts','tools/g13/generate_assets.py','tools/g13/prepare_build.cjs'],
 APIs=dict(configure='g13Configure(config128:Uint32Array,seedReceiptJSON:string):string',
 offer='g13Offer128(input:Uint32Array):string',pulse='g13PagePulse(session:string,sequence:string,uiNs:string):string',
 stop='g13Stop(scopeJSON:string):string',status='g13Status():string',coldBegin='g13ColdBegin(token:string,mode:string):string',
 coldPage='g13ColdPage(id:string,segment:number,wordOffset:number):string',coldEnd='g13ColdEnd(id:string):string',
 diagnostic='g13DiagnosticBegin(config128,fixtureReceiptJSON):string; test-only seeds/faults are separate identity',
 session='g13BeginSession(scopeJSON:string):string',active='g13SetActive(active:boolean,scopeJSON:string):string',
 view='g13ViewMode(mode:string,scales64:Uint32Array):string; immutable viewGeneration, no physical advance'),
 cold=dict(route=13,routeRawBudgetBytes=11510272,slots=2,builder=1,reader=1,TTLns=60000000000,
 prefixBytes=65536,ROOTsegments=['PREFIX','BASE','CANDIDATE','CTRL'],
 FULLsegments=['PREFIX','BASE','CANDIDATE','REPLAY','WORK','CTRL','PACK0','PACK1'],
 pageWords=1024,pageUtf8Bytes=16384,pageMax='1+ceil(rawBytes/4096)+32',archiveUtf8Max='(pageMax+1)*16384',
 prefixHashPolicy=dict(canonical='SHA256 prefix65536 with header words248..255 zero',
  prefixDescriptorHash='all8words0; header canonicalPrefixSHA owns prefix hash',
  otherDescriptors='SHA256 complete actual segment bytes',
  externalLastManifest=['rawPrefixSHA','payloadRawSHA','pagesFileSHA'],finalFileSHA='external receipt only'),
 freeze='owner idle; pin retained banks/work/control/pack; copy to broker-owned host slot; release GPU pins only after copy completion',
 normalHotReadBytes=1024,sourceOrQuantum='1 tail fence, no field hot readback'),
 checks=dict(profiles=len(profiles),quantumDispatches=len(plan),sourceDispatches=len(sourceOps),G1F4=3296,
 layoutNoOverlap=True,resourceMatchesT605=True,constantIntervalsAgree=True,physicalTestsRun=False),
 scientificSources=[dict(title='Khronos GLSL ES3.10 specification',url='https://registry.khronos.org/OpenGL/specs/es/3.1/GLSL_ES_Specification_3.10.pdf',
 scope='API/precision/barrier constraints, not project solver or error/performance qualification')])
emit('architecture-T607-g13-engineering-v1.json',engineering)
for n,a in schemas.items():
 m=re.search(r'(\d+)$',n)
 if m:assert len(a)==int(m.group(1)),n
for s in [1,2]:
 for j in [1,2,4]:assert len([x for x in plan if x['scope']==s and x['J']==j])==4+56*j
assert engineering['gates']==design['gates'] and engineering['support']==design['support']
emit('analysis-T607-engineering-evidence-v1.json',dict(schema='T607_FINITE_ENGINEERING_EVIDENCE_V1',date='2026-11-18',
 role='architect',deviceUsed=False,buildsRun=False,productTestsRun=False,CPUPhysicsAdded=False,productPASS=False,
 checks=['five nonoverlapping layout profiles and four resource rows match T605','all declared word counts',
 '820 indexed quantum passes, each scope/trial4+56J, source16/G1F4 3296',
 'Machin/ln2 rational intervals agree on constants and polynomial truncation bounds',
 'original gates/support unchanged','local document links exist'],
 limits=['not a pressure/viscosity solve','not a CPU fluid stepper or independent physical oracle',
 'software GPU math, actual FP32 behavior and F05 pressure/splitting unverified','performance and device qualifications unmeasured'],
 artifacts=[receipt('docs/'+n) for n in engineering['tables']+['architecture-T607-g13-engineering-v1.json',
 'architecture-T607-g13-engineering-v1.md','retro-T607-g13-engineering-v1.md','analysis-T607-engineering-layout-v1.py']]))
for n in ['architecture-T607-g13-engineering-v1.md','retro-T607-g13-engineering-v1.md']:
 for link in re.findall(r'\]\(([^)]+)\)',(D/n).read_text(encoding='utf-8')):
  if not link.startswith('http'):assert (D/link).exists(),link
print(json.dumps(dict(profiles=len(profiles),quantum=len(plan),source=len(sourceOps),G1F4=3296,
 maxResident=max(x['residentBytes'] for x in profiles),mathConstants='exact rational bounds checked',productTestsRun=False)))
