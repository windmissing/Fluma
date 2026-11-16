"""Architect read-only evidence/contract audit; no device, GL, full tests, or business implementation."""
from pathlib import Path
from collections import Counter
from fractions import Fraction
import hashlib
import importlib.util
import json
import math
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT/'docs'


def read(path):
    return json.loads((ROOT/path).read_text(encoding='utf-8-sig'))


def fp(path):
    p = Path(path)
    if not p.is_absolute():
        p = ROOT/p
    b = p.read_bytes()
    try:
        name = p.relative_to(ROOT).as_posix()
    except ValueError:
        name = p.as_posix()
    return dict(path=name, bytes=len(b), sha256=hashlib.sha256(b).hexdigest())


def output(name, value, paths):
    value.update(date='2026-11-16', architectScope='TRIAGE_AND_ENGINEERING_HANDOFF_COMPLETE',
                 productRepair='NOT_IMPLEMENTED_BY_THIS_TASK', deviceUsed=False, GLExecuted=False,
                 CPUSimulatorAdded=False, inputs=[fp(p)for p in paths])
    (DOCS/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def package(task):
    receipt=f'test/reports/t257/{task}.json'
    r=read(receipt)
    p=Path(r['signed']['path'])
    info=fp(p)
    assert info['sha256']==r['signed']['sha256']
    libs={}
    with zipfile.ZipFile(p) as z:
        for n in z.namelist():
            if n.endswith('.so'):
                b=z.read(n);libs[n]=dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
    return dict(receipt=fp(receipt),signed=info,libs=libs)


def t530():
    folder=ROOT/'test/reports/t491/t510'
    exe=[]
    for name in ('execution-t510.json','execution2-t510.json','execution3-t510.json'):
        d=read('test/reports/t491/t510/'+name)
        actions=[json.loads(bytes.fromhex(x['raw']).decode('utf-8'))for x in d['actions']]
        counts=Counter(x['event']for x in actions)
        assert len(actions)==32 and int(d['actionDropped'])==0 and int(d['packetDropped'])==0 and d['packets']==[]
        assert counts['POINTER_DOWN_READY_COORDINATES']==counts['UPLOAD_32U32']==0
        exe.append(dict(file=fp(folder/name),stageCounts={x['first']['stage']:int(x['count'])for x in d['stages']},
                        decodedActions=actions,actionCounts=dict(counts),packetCount=0,
                        groupScopes=[g['scope']for g in d['groups']],scopeDropped=d['scopeDropped']))
    d=read('test/reports/t491/t510/execution-t510.json')
    results=[]
    for g in d['groups']:
        start=int(g['root']['started']);pending={};flush=[]
        for p in g['events']:
            if p['stage']=='FLUSH_ENTER':
                pending[p['root']]=p
            elif p['stage']=='FLUSH_EXIT' and p['root'] in pending:
                a=pending.pop(p['root'])
                flush.append(dict(root=p['root'],fromPass=int(a['a']),toPass=int(a['b']),
                                  enterNs=a['ns'],exitNs=p['ns'],wallNs=int(p['ns'])-int(a['ns'])))
        points={}
        for p in g['events']+g['tail']:
            if p['stage'] in ('ROOT_ACCEPT','EGL_SWAP_ENTER','EGL_SWAP_RETURN','POST_DRAW_PERMIT_EXIT','FIRST_REVOKE'):
                points.setdefault(p['stage'],[]).append(p)
        results.append(dict(scope=g['scope'],root={k:v for k,v in g['root'].items()if not k.startswith('identity')},
                            firstCause=g['firstCause'],faultRoot={k:v for k,v in g['faultRoot'].items()if not k.startswith('identity')},
                            retainedEvents=len(g['events']),dropped=int(g['dropped']),
                            tailDropped=int(g['tailDropped']),flushes=flush,
                            flushTotalWallNs=sum(p['wallNs']for p in flush),points=points))
    assert results[0]['flushTotalWallNs']==219136405
    assert max(x['wallNs']for x in results[0]['flushes'])==86528281
    assert len(results[0]['flushes'])==13
    a=results[1];start=int(a['root']['started'])
    first={k:v[0]for k,v in a['points'].items()}
    assert first['ROOT_ACCEPT']['root']=='1' and first['EGL_SWAP_RETURN']['code']==0
    assert first['POST_DRAW_PERMIT_EXIT']['a']=='1' and first['POST_DRAW_PERMIT_EXIT']['b']=='1'
    acceptNs=int(first['ROOT_ACCEPT']['ns'])-start
    swapNs=int(first['EGL_SWAP_RETURN']['ns'])-int(first['EGL_SWAP_ENTER']['ns'])
    swapSinceRoot=int(first['EGL_SWAP_RETURN']['ns'])-start
    assert (acceptNs,swapNs,swapSinceRoot)==(160538594,10353282,199538907)
    latest=read('test/reports/t491/t510/execution2-t510.json')
    gap=next(x['first']for x in latest['stages']if x['first']['stage']=='CONTROL_REVOKED')
    assert int(gap['c'])==33413750 and int(gap['c'])-33333333==80417
    bridge='.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/app_bridge.cpp'
    controller='.codex/local-test/t492-m1-delivery/entry/src/main/ets/physics/GpuM1DemoController.ets'
    page='.codex/local-test/t492-m1-delivery/entry/src/main/ets/pages/GpuResidentM1.ets'
    c=(ROOT/controller).read_text(encoding='utf-8-sig')
    allowed=next(l.strip()for l in c.splitlines()if 'private allowed('in l)
    assert 'CURRENT'not in allowed and "permissionState==='READY'"in allowed
    down=c[c.index(' down(id:'):c.index(' move(id:')]
    assert down.index("'POINTER_DOWN_READY_COORDINATES'")<down.index('if(!ok)')
    groups={'COPY':[0,20,22,28],'PREFLIGHT':[1,24],'WATER':[2,3,4],
            'INDEX_SORT':[6,7,8,9,16,17],'TRANSPORT':[5,10,11,15,18],
            'BED':[12,13,14],'AUDIT_REPLAY':[19,21,23],'PUBLICATION':[25,26,27]}
    allstages=[i for x in groups.values()for i in x]
    assert sorted(allstages)==list(range(29)) and len(set(allstages))==29
    invalid=(folder/'execution4-t510.json').read_text(encoding='utf-8-sig')
    assert 'No such file' in invalid
    paths=list(folder.glob('*'))+[ROOT/bridge,ROOT/controller,ROOT/page,
        ROOT/'.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/bridge_state.h',
        ROOT/'.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/m1/quantum_candidate.cpp',
        ROOT/'.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/m1/full_transport.cpp',
        ROOT/'.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/m1/scene.cpp',
        ROOT/'docs/test-report-T510-control-source-chain-device-v1.md',
        ROOT/'docs/architecture-T505-device-root-present-evidence-v1.md',
        ROOT/'docs/architecture-T490-m1-first-root-budget-v1.md',
        ROOT/'docs/test-report-T410-v2-dual-source-device-v1.md',
        ROOT/'docs/test-report-T437-prefix-benefit-device-v1.md',
        ROOT/'docs/test-report-T461-m2-v3-device-v1.md',
        ROOT/'docs/architecture-T530-source-swap-triage-v1.md',Path(__file__)]
    r2=read('test/reports/t491/t510/boot-t510-r2.json')
    r2recovery={k:r2['recovery'][k]for k in ('phase','reason','firstReason','epoch','session','runLo','incomplete','dropped')}
    output('analysis-T530-evidence-v1.json',dict(schema='T530_ARCHITECT_TRIAGE_V1',
        package=package('T492-m1-delivery'),executionSummaries=exe,rootScopes=results,
        sourceAllowedActual=allowed,downRejectStillEmitsDiagnostic=True,
        invalidExecution4=invalid,run2BootPid=r2['pid'],run2Recovery=r2recovery,
        run2ExecutionEvidence='NOT_AVAILABLE_IN_LOCAL_ARCHIVE',
        acceptedRootWallNs=acceptNs,firstSwapCallWallNs=swapNs,firstSwapSinceRootNs=swapSinceRoot,
        lastGapNs=int(gap['c']),gapOverLimitNs=80417,
        pixelCause='UNRESOLVED_NO_PAIRED_FBO_DEFAULT_SYSTEM_ROI',
        controllerCURRENTGate='ABSENT_REPORT_INFERENCE_REJECTED',specializationGroups=groups,
        activeProgramRoleBound=12,rootDispatchK192Kw128=35+36+28+28),[p for p in paths if p.is_file()])
    return dict(task='T530',acceptedRootNs=acceptNs,firstSwapNs=swapNs,flushTotalNs=results[0]['flushTotalWallNs'])


def t531():
    folder=ROOT/'test/reports/t491'
    pkg=package('T502-m1-delivery')
    man=read('test/reports/t491/t519/timeline-manifest.json')
    declared=man['identity']['packageSha256']
    assert declared!=pkg['signed']['sha256'] and declared[:16]==pkg['signed']['sha256'][:16]
    trace=read('test/reports/t491/t526/trace-manifest.json')
    assert trace['words']==7680 and trace['header']['stateWords']==7424 and trace['header']['eventWords']==0
    verified=[]
    for i in range(3):
        p=folder/'t526'/f'trace-{i}.json';actual=fp(p);receipt=trace['files'][i]
        assert actual['bytes']==receipt['bytes'] and actual['sha256']==receipt['sha256']
        page=json.loads(p.read_text(encoding='utf-8-sig'))
        assert page['scope']==trace['scope'] and actual['bytes']<=16384
        verified.append(dict(page=i,actual=actual,receipt=receipt))
    assert sum(x['bytes']for x in trace['files'])==24786
    assert trace['header']['rootKind']==2 and trace['header']['rootToken'].split('|')[1]=='521'
    samples={}
    for p in sorted((folder/'t519').glob('mem-*.txt')):
        text=p.read_text(encoding='utf-8-sig');rows={}
        for l in text.splitlines():
            m=re.match(r'\s*(GL|Total)\s+(\d+)\s+',l)
            if m:rows[m.group(1)+'PssKb']=int(m.group(2))
        samples[p.name]=rows
    assert samples['mem-cold-start.txt']['TotalPssKb']==1590410
    assert samples['mem-after-reset.txt']['TotalPssKb']==1538879
    assert samples['mem-after-export.txt']['TotalPssKb']==368027
    vm=(folder/'t519/vm-after-reset.txt').read_text(encoding='utf-8-sig')
    assert re.search(r'VmRSS:\s*370136\s*kB',vm)
    known=55410688+3501224+16384+65536
    assert known==58993832
    cases=['RESOURCE_PHASE_SAMPLE','TRACE_MAX_SOURCE_Q','TWO_SLOT_PIN_REPLACE','TTL_WALL',
           'TTL_EDGE_INJECTED','DIAGNOSTIC_CREDIT','DIAGNOSTIC_BUSY_FAIL','CACHE_FAULT']
    paths=list((folder/'t519').glob('*'))+list((folder/'t526').glob('*'))+[
        ROOT/'docs/test-report-T519-resource-baseline-device-v1.md',ROOT/'docs/test-report-T526-cold-protocol-device-v1.md',
        ROOT/'docs/developer-T501-resource-ledger.md',ROOT/'docs/developer-T502-cache-temp-budget.md',
        ROOT/'docs/developer-T507-bounded-cold.md',ROOT/'docs/architecture-T503-bounded-cold-protocol-v1.md',
        ROOT/'test/t501/generate.cjs',ROOT/'test/t502/generate.cjs',ROOT/'test/t507/generate.cjs',
        ROOT/'test/t507/service.inc',ROOT/'test/t507/napi.inc',ROOT/'test/t507/pages.cjs',
        ROOT/'.codex/local-test/t502-m1-final/entry/src/main/ets/pages/GpuResidentM1.ets',
        ROOT/'.codex/local-test/t507-m2-1116/entry/src/main/cpp/gpu_resident/cold_trace.h',
        ROOT/'docs/architecture-T531-resource-lab-and-export-v1.md',Path(__file__)]
    output('analysis-T531-evidence-v1.json',dict(schema='T531_ARCHITECT_TRIAGE_V1',
        resourcePackage=pkg,declaredPackage=declared,fullSHAIdentityMatch=False,
        coldPackage=package('T507-m2-complete'),memoryPssSamples=samples,vmAfterResetOriginal=vm,
        traceManifestOriginal=trace,locallyVerifiedTracePages=verified,
        traceMissingLocalPages=list(range(3,9)),traceManifestOnlyTotalUtf8=24786,
        localTraceCompleteness='3_OF_9_ORIGINAL_PAGES',
        traceReleaseDoesNotProveCapacityZero=True,systemRssDoesNotProveDriverFree=True,
        resourceApiWiredToPage=False,pressureCases=cases,
        labEvidenceLabels=['FIXTURE_SYNTHETIC','PRODUCTION_READ_ONLY'],labPhysicsValidated=False,
        limits=dict(traceCapacity=55410688,diagnosticCopyAndWriter=3501224,pageWriter=16384,
                    labControl=65536,knownLabHostPayload=known,cachePinSeparateMax=134217728,
                    resourceRawUtf8Max=1048576,resourceSampleTotalUtf8Max=2097152,
                    workers=2,queueCommands=8,TTL=60000000000),
        measurementsStillMissing=['same-scope native logical/capacity/temp figures','actual pressure on device',
                                  'nonempty SOURCE/scene long sequence','driver resource reclaim']),[p for p in paths if p.is_file()])
    return dict(task='T531',identityMismatch=True,localTracePages='3/9',labKnownHostBytes=known)


def t532():
    p='test/reports/t491/t408a8/prepare-a8.json';d=read(p);r=d['readiness']['snapshot']['raster']
    parent=r['allocation']['parent'];want=r['desiredWindowRect'];actual=r['actual'];local=r['localPx']
    left=math.ceil(parent['x']);right=math.floor(parent['x']+parent['width'])
    top=math.ceil(parent['y']);bottom=math.floor(parent['y']+parent['height'])
    assert want==dict(x=left,y=top,width=right-left,height=bottom-top)
    assert (left,top,right-left,bottom-top)==(52,990,1176,1716)
    assert all(r[k]==actual for k in ('inner','touch','screen','innerScreen','touchScreen'))
    assert parent['x']+local['x']==52 and actual['x']-local['x']==51
    assert actual['x']-52==-0.20001220703125
    assert actual['x']>=parent['x'] and actual['x']+actual['width']<=parent['x']+parent['width']
    assert d['readiness']['readinessReason']=='PREPARE_TIMEOUT:RASTER_PLACEMENT_MISMATCH'
    finite=[]
    for windowX in (0,17,-23):
        for hostX in (0,7,31):
            q=52-hostX
            assert hostX+q==52 and windowX+hostX+q==windowX+52
            finite.append(dict(windowOrigin=windowX,hostX=hostX,requestedLocal=q,windowActual=52,
                               screenActual=windowX+52))
    sdk='C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/ets/api/@ohos.arkui.componentUtils.d.ts'
    paths=[ROOT/p,ROOT/'test/reports/t491/t408a8/t408a8_l1.json',ROOT/'test/reports/t491/t408a8/t408a8_s1.jpeg',
           ROOT/'test/reports/t516/handoff.json',ROOT/'test/reports/t516/generation.json',
           ROOT/'test/reports/t525/t525-inward-position-oracle.json',ROOT/'docs/test-report-T525-inward-position-v1.md',ROOT/'test/t516/generate.cjs',
           ROOT/'test/t516/RasterGeometry.ets',ROOT/'.codex/local-test/t516-delivery/entry/src/main/ets/pages/P051DerivedV2Demo.ets',
           ROOT/'docs/architecture-T509-inward-raster-position-v2.md',ROOT/'docs/developer-T516-inward-raster.md',
           ROOT/'docs/test-report-T408-calibrated-display-v1.md',ROOT/'docs/architecture-T532-window-host-placement-v1.md',
           Path(sdk),Path(__file__)]
    output('analysis-T532-evidence-v1.json',dict(schema='T532_ARCHITECT_TRIAGE_V1',
        package=package('T516-release'),originalRaster=r,readinessReason=d['readiness']['readinessReason'],
        placementDeltaX=actual['x']-52,impliedCompositionOrigin=actual['x']-local['x'],
        windowScreenOriginZero=True,originalOutsideAllocationFixed=True,checkerCorrectlyRejects=True,
        cause='FLOATING_PARENT_COMPOSITION_ASSUMPTION_UNPROVEN_PLATFORM_CUT_NOT_IDENTIFIED',
        replacement='T532_WINDOW_HOST_RASTER_V3',actualTolerance=0,feedbackOffsetCalibration=False,
        finiteHostArithmeticCases=finite,platformPlacementMeasuredByThisTask=False,
        nextRawFields=['localOffset','windowOffset','screenOffset','size','full transform matrix','parent node ids',
                       'integer host raw rect','requested local on host','layout/draw generation','native tuple']),paths)
    return dict(task='T532',deltaX=actual['x']-52,hostArithmeticCases=len(finite))


def t533():
    p=ROOT/'test/t499/m3_source_oracle.py'
    spec=importlib.util.spec_from_file_location('t533_existing_oracle_readonly',p)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    zero=[0]*128;water=[5,5]+[0]*126;cap=[5]+[module.CQ]*127;request=[0,4]+[0]*126
    status,admitted,wt,detail=module.solve_lambda(request,cap,water,zero,water,65536)
    assert status=='OK' and detail['eHi']==1 and sum(wt)==10 and wt[:6]==[5,2,1,1,1,0]
    assert admitted==request and detail['d']==4
    def F(e):
        return sum(min(max(water[i]+e,admitted[i]),cap[i])-admitted[i]for i in range(128))
    assert (F(0),F(1))==(6,133)
    violated=[i for i in range(128)if water[i]+detail['eHi']>cap[i]]
    assert violated==[0]
    variants=[]
    for q in (1,2,3,4):
        r=[0,q]+[0]*126;s,a,w,h=module.solve_lambda(r,cap,water,zero,water,65536)
        assert s=='OK' and h['eHi']==1
        variants.append(dict(request=q,existingOracle=s,eHi=1,formalResult='ETA_TRUNCATION_RETRY'))
    capEqual=[6]+[module.CQ]*127
    se,ae,we,de=module.solve_lambda(request,capEqual,water,zero,water,65536)
    assert se=='OK' and de['eHi']==1 and not any(water[i]+de['eHi']>capEqual[i]for i in range(128))
    half=module.solve_lambda(request,cap,water,zero,water,32768)
    assert half[0]=='OK' and half[3]['eHi']==1 and water[0]+1>cap[0]
    fullMatchCap=[5,module.CQ]+[0]*126
    matched=module.solve_lambda(request,fullMatchCap,water,zero,water,65536)
    assert matched[0]=='OK' and matched[3]['eHi']==4 and matched[3]['d']==0
    assert water[0]+4>fullMatchCap[0]
    assert module.solve_lambda(zero,cap,water,zero,water,65536)[0]=='WANT0'
    emptyWater=module.solve_lambda(request,[module.CQ]*128,zero,zero,zero,65536)
    assert emptyWater[0]=='OK' and emptyWater[3]['eHi']==0
    paths=[p,ROOT/'test/t522/solver_cases.py',ROOT/'entry/src/main/cpp/gpu_resident/m3a/occupied_solvers.glsl',
           ROOT/'docs/algorithm-T497-gpu-M3-preview-source-v1.md',ROOT/'docs/developer-T522-v4-implementation.md',
           ROOT/'docs/test-report-T523-m3-v4-product-diff-v1.md',ROOT/'docs/architecture-T533-conservative-eta-oracle-triage-v1.md',Path(__file__)]
    output('analysis-T533-evidence-v1.json',dict(schema='T533_ARCHITECT_ORACLE_CONTRACT_TRIAGE_V1',
        originalInputs=dict(N=128,T0=zero,W0=water,eta0=water,cap=cap,requested=request,lambdaQ16=65536),
        existingOracle=dict(status=status,admitted=admitted,Wtrial=wt,detail=detail),
        formalCheck=dict(F0=6,F1=133,waterTotal=10,violatingColumns=violated,
                         formalResult='ETA_TRUNCATION_RETRY',gpuObservedStatusFromExistingReport=4),
        requestVariants=variants,equalityBoundary=dict(cap0=6,eHi=1,formalResult='OK'),
        additionalFiniteChecks=dict(halfLambdaConservativeReject=True,exactFhiVConservativeReject=True,
                                    wantZeroShortCircuit=True,eHiZeroAllowed=True),
        responsibility='TEST_ORACLE_IMPLEMENTATION_OMITS_FORMAL_PREDICATE',
        handoffRole='tester via manager; no product change / no oracle editing by developer',
        completeTestSuitesRun=False,existingOracleSingleAttemptCalls=10),paths)
    return dict(task='T533',existingOracle=status,formalResult='ETA_TRUNCATION_RETRY',F0=6,F1=133)


if __name__=='__main__':
    for task in (t530,t531,t533,t532):
        print(json.dumps(task(),ensure_ascii=False))
