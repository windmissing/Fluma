"""Finite engineering/source/artifact attribution, never runs tests or products.
--record writes only this batch's formal docs; default compares saved receipts.
"""
from pathlib import Path
import json,hashlib,re,sys,zipfile,subprocess
R=Path(__file__).resolve().parents[1]
def text(p):return (R/p).read_text(encoding='utf-8-sig')
def get(p):return json.loads(text(p))
def sha(b):return hashlib.sha256(b).hexdigest()
def fp(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=sha(b))
def emit(p,v):(R/p).write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def cuts(p,needles):return dict(path=p,lines=[dict(line=i+1,text=s[:1800],lineCharacters=len(s)) for i,s in enumerate(text(p).splitlines()) if any(n in s for n in needles)])
def base(n):return dict(task='T'+str(n),date='2026-11-19',role='architect',deviceUsed=False,productBuildsRun=False,productTestsRun=False)
def collect655():
 log='.codex/local-test/main-build-t629b.txt';clean=re.sub(r'\x1b\[[0-9;]*m','',text(log));commands=[s for s in clean.splitlines() if 'cmd.exe /C' in s]
 assert len(commands)==2 and all(s.count('--rtlib=compiler-rt')==62 for s in commands)
 build='entry/.cxx/default/default/debug/arm64-v8a/';cache=text(build+'CMakeCache.txt');ninja=text(build+'build.ninja');rules=text(build+'CMakeFiles/rules.ninja')
 flag=next(s for s in cache.splitlines() if s.startswith('CMAKE_SHARED_LINKER_FLAGS:STRING='));assert flag.count('--rtlib=compiler-rt')==63
 rsp=build+'CMakeFiles/grs_cold_slots.rsp';assert text(rsp).strip()=='gpu_resident/CMakeFiles/grs_cold_slots.dir/cold_slot_broker.cpp.o  -lm'
 target=next(s for s in ninja.splitlines() if s.startswith('build ') and 'libgrs_cold_slots.so: CXX_SHARED' in s)
 tool='C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/build/cmake/ohos.toolchain.cmake'
 assert '${OHOS_COMMON_LINKER_FLAGS} ${_DEVICEINFO_LIB} ${CMAKE_SHARED_LINKER_FLAGS}' in Path(tool).read_text(encoding='utf-8')
 provider='entry/src/main/cpp/gpu_resident/napi_provider_profile.cmake';assert 'LINKER_FLAGS' not in text(provider)
 manifests='.codex/local-test/t541-t624-a1/manifest.json';m=get(manifests)
 argv={n:text('.codex/local-test/t541-t624-a1/'+n) for n in ['compile-cmd.txt','g9dm-compile-cmd.txt','kernel_assets-cmd.txt','g9spf-ka-cmd.txt']}
 assert 't564-stage-tree' in argv['compile-cmd.txt'] and 'HelloWorld\\entry\\src' in argv['g9dm-compile-cmd.txt']
 contract=dict(schema='T655_WINDOWS_LINK_STATE_CONTRACT_V1',**base(655),classification='BUILD_CACHE_FLAG_ACCUMULATION; tester isolated clean build first, durable build-policy engineering only if necessary',
  currentTarget='grs_cold_slots',alreadyUsesObjectResponseFile=True,knownObjectCount=1,
  rules=['use fresh attempt output/build cache, preserve original polluted cache/logs','native subprocess argv array, shell=False; never POSIX shlex Windows argv','absolute response token @D:/.../file.rsp as one argv item, cwd is its build tree','all target flags retained, -fno-fast-math/-ffp-contract=off never relaxed',
   'CMAKE_NINJA_FORCE_RESPONSE_FILE alone only moves in/libs in current rule, not repeated LINK_FLAGS','do not patch global installed SDK or delete global caches','if clean target still exceeds wrapper bound, use target-scoped full driver rsp with all effective flags/objects/libs and explicit build dependencies'],
  rspEncoding='UTF8 no BOM, LF, driver response quoting; forward-slash canonical paths, embedded double-quotes/backslashes encoded for the consuming driver; native argv serialization is separate',
  receipts=['full native argv JSON+cwd+launcher identity','generated rules/target stanza/response raw SHA','known user vs SDK flags baseline and repetition count','compiler/linker SHA, immutable source/generated inputs/object SHA','new unique ELF path/hash/BuildId and package entry relation'],
  compilerHistoricalConclusion='a1 reported fast compilation is attempt-local; no proof excluding every compiler issue or explaining old CPU-zero waits',
  noAutomaticBlock='independently built correct G9rx package is not blocked by this main-tree cache failure')
 inputs=['test/reports/t618/t590-blocked.json','test/reports/t618/t541-staged.json',log,build+'CMakeCache.txt',build+'build.ninja',build+'CMakeFiles/rules.ninja',rsp,provider,'entry/src/main/cpp/gpu_resident/cold_slot_broker.cmake',manifests]
 e=dict(schema='T655_STATIC_BUILD_EVIDENCE_V1',**base(655),commands=[dict(characters=len(s),repeatedSdkGroups=s.count('--rtlib=compiler-rt'),ansiStrippedCommand=s) for s in commands],cacheFlagCharacters=len(flag),cacheRepeatedGroups=63,
  target=target,rsp=fp(rsp),rspText=text(rsp),SDKToolchain=fp(tool),SDKToolchainCuts=cuts(tool,['set(CMAKE_SHARED_LINKER_FLAGS']),
  mixedSourceSpecimens=True,argv=argv,a1InputCount=len(m['inputs']),manifestUsesTruncatedSHAPrefixes=True,
  limits=['reported11876 not the exact retained command length','raw cmd/Win32 error message and manual failed rsp argv not retained here','T629 provider CMake changes compile SDK identity, does not assign link flags; trigger causality unproven','current cache/rules are a later snapshot than the log (63 vs62 groups)'],inputs=[fp(p) for p in inputs])
 return contract,e
def collect656():
 driver='test/t636/driver.cpp';source='entry/src/main/cpp/gpu_resident/cold_slot_broker.cpp';s=text(driver)
 assert 'if (g2) B.GpuFreed(gB);' in s and 'A.GpuRes(&gA, ctxA, 1)' in s and 'if(!ticket||*ticket||!context||!bytes' in text(source)
 rawLines=[s for s in text('test/reports/t636/driver-log.ndjson').splitlines() if s.strip()];rows=[json.loads(s) for s in rawLines]
 segments=[]
 for row in rows:
  if row['event']=='START':segments.append([])
  segments[-1].append(row)
 assert len(segments)==2 and all(len(g)==19 and g[-2]['event']=='M5_CTX_RETIRED' for g in segments)
 assert 'ticket=1' in next(r['detail'] for r in rows if r['event']=='M5_A_GPU_RESERVE')
 assert not any(r['event'].startswith('M4') for r in rows)
 broker='.codex/local-test/t636-harness/libgrs_cold_slots.so';release='.codex/local-test/t608-release/entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libgrs_cold_slots.so'
 assert fp(broker)['sha256']==fp(release)['sha256']=='715f314d43549d4eed991922f25faa350fedcaf6ab8dab1598abc980dd278d03'
 llvm=Path('C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/llvm/bin')
 def inspect(exe,args):
  r=subprocess.run([str(llvm/exe),*args],cwd=R,capture_output=True,timeout=15);assert r.returncode==0;return r.stdout.decode('utf-8')
 nm=inspect('llvm-nm.exe',['-D','--defined-only',broker]);exports=[s.split()[-1] for s in nm.splitlines() if ' Grs' in s];assert len(exports)==8
 needed=inspect('llvm-readelf.exe',['-d','.codex/local-test/t636-harness/libadapter_a.so','.codex/local-test/t636-harness/libadapter_b.so']);assert needed.count('Shared library: [libgrs_cold_slots.so]')==2
 contract=dict(schema='T656_GPU_TICKET_FIXTURE_CORRECTION_V1',**base(656),classification='TEST_FIXTURE_INPUT_AND_ORDER_ERROR, production semantics unchanged',
  preconditions='GpuReserve requires nonnull ticket pointer, *ticket==0, context!=0, bytes>0, no live global GPU ticket',
  actualSequence=['A Reserve out gA=1','B Reserve rejected','wrong Freed/Retired do not clear','B Freed(1)','B Reserve succeeds gB=2','B Freed(2) BEFORE ContextRetired','ContextRetired(ctxA) sees no live ticket','A Reserve(&gA) rejects stale nonzero input1 before context checks'],
  correctedSequence=['reserve fresh zero gA in ctxA','wrong ticket free and wrong context retire followed by fresh-zero probe still reject','B Freed(gA) then fresh-zero gB succeeds','KEEP gB ACTIVE','B ContextRetired(ctxA) clears matching global ticket','A reserve NEW zero gC succeeds','wrong-context retire does not release a new active ticket; correct cleanup by owned ticket'],
  semantics='matching ContextRetired clears matching GPU ticket; caller variable not rewritten; opaque context call is not proof of actual EGL destruction',
  followup='tester repairs driver/out-param logging/null export checks, no designer or developer change needed now',
  notMeasured=['M4 route accumulated credit boundary','M4 process accumulated cap boundary','production route integration','driver actual free/eglDestroyContext'],
  budgets=dict(maxRuntimeSeconds=30,maxRecords=128,recordUtf8Max=512,logBytesMax=65536,manifestBytesMax=16384))
 inputs=['docs/test-report-T627-harness-v2.md','docs/architecture-T636-v1.md','docs/architecture-T636-cross-dso-harness-v1.json',driver,'test/t636/adapter_a.cpp','test/t636/adapter_b.cpp',source,'entry/src/main/cpp/gpu_resident/cold_slot_broker.h','test/reports/t636/driver-log.ndjson',broker,release,'.codex/local-test/t636-harness/libadapter_a.so','.codex/local-test/t636-harness/libadapter_b.so']
 e=dict(schema='T656_SOURCE_BINARY_AND_SAVED_CALLS_V1',**base(656),inspectionToolsOnly=True,logRows=len(rows),uniqueRawLines=len(set(rawLines)),anonymousSegments=[dict(segment=i,rows=len(g),matrixBooleanRows=sum(r['event'].startswith('M') for r in g)) for i,g in enumerate(segments)],rows=rows,exports=exports,neededLines=[x for x in needed.splitlines() if 'File:' in x or 'NEEDED' in x],
  sourceCuts=[cuts(driver,['GpuRes','GpuFreed','GpuCtxRetired','return api.Res']),cuts(source,['GrsFullGpuReserve','if(!ticket||*ticket','GrsFullGpuContextRetired'])],
  limits=['38 raw rows split into two19-row seq-reset segments;21 unique strings is dedup count, not one ordered run','15 matrix boolean observations per segment, not a complete17-step accounting','M4 not executed','B DSO uses its A-named exports because loader chooses A first; cross-DSO still occurs but B-prefix wording is inaccurate'],inputs=[fp(p) for p in inputs])
 return contract,e
def collect657():
 trace='test/reports/t496/gltrace-t646cA.ndjson';rows=[json.loads(s) for s in text(trace).splitlines() if s.strip()];pairs={}
 for r in rows:pairs.setdefault(tuple(r.get(k) for k in ['runId','arm','pid','createAttemptId','callId','functionId']),set()).add(r['event'])
 opened=[k for k,v in pairs.items() if 'ENTER' in v and 'EXIT' not in v];assert len(opened)==1 and opened[0][-2:]==(266,411)
 assert len(rows)==533 and sum(r['event']=='ENTER' for r in rows)==267
 shadow={}
 for r in rows:
  if r['event']!='ENTER':continue
  if r['functionId']==410:shadow['lastUseProgram']=r
  if r['functionId']==404:
   m=re.search(r'index=(\d+) buffer=(\d+)',r['detail'])
   if m:shadow['binding'+m[1]]=r
 receipts=[]
 for p in (R/'.codex/local-test/t496/C0-files').rglob('*.jsonl'):
  m=json.loads(p.read_text(encoding='utf-8-sig').splitlines()[-1]);receipts.append(dict(file=fp(str(p.relative_to(R))),role=m['role'],shaderSHA=m['shaderSHA'],beginNs=m['beginNs'],linkEndNs=m['linkEndNs'],stage=m['stage'],hasPidOrAttempt=('pid' in m or 'attempt' in m)))
 contract=dict(schema='T657_T646C_A_PRECISE_EXIT_CONTINUATION_V1',**base(657),classification='PROCESS_EXIT_CAUSE_UNPROVEN; tester completes T646 facility and exit/crash evidence',
  finalOpenCall=dict(run='t496-t646cA',arm='create_C0',pid=62736,attempt='t646cA',callId=266,functionId=411,x=1,y=None,z=None),
  noInference=['no dispatch hard-crash/driver defect from missing EXIT and later tasklist absence','411 is functionId, not common callId/site identity across runs','steady_clock ns is not UTC epoch','host ANGLE outcome not hardware GPU product verdict'],
  required=['child+parent PID/start/argv/cwd/PE SHA and loaded ANGLE module bytes','child wait handle exit code unsigned+hex, exception code/address/module+stack if exception','watchdog launch/deadline/kill-request/target-start/exit timeline','same-attempt stderr/stdout/ActiveCall phase/owner stage and bounded thread/module exception dump','complete GL argument-return shadow source SHA/program generation/XYZ/binding-allocation/uniform context'],
  facility='all T646 steps remain in scope: real clock on actual CREATE_ONLY, valid recovery guard, fixed-capacity shadow, sink phase and truthful budgets',
  budgets=dict(totalSeconds=420,resourceSeconds=240,handshakeSeconds=120,diagnosticAndCleanupSeconds=60,traceMaxBytes=33554432,dumpMaxBytes=67108864,dumpCountMax=1),
  programReceiptQualification='existing cache role/SHA/time is useful correlation; absent pid/ticket/program mapping must not elevate to exact loaded program proof',
  followup='tester only now; new product readonly stage only after precise existing-interface gap shown; no larger window or three-arm rerun')
 inputs=['docs/test-report-T496-create-only-v7.md','docs/architecture-T646-v1.md','test/reports/t496/v7-c0-result.json',trace,'test/reports/t496/log-t646cA.ndjson','test/reports/t496/stages-t646cA.ndjson','test/t496/external_source_diff.cpp','test/t496/run_create_only.cmd','test/t496/gl_forward_sink.cpp','test/t496/gl_sink.inc','.codex/local-test/t496/out-cA.txt','docs/analysis-T657-host-inspection-v1.json']
 e=dict(schema='T657_SAVED_TRACE_SOURCE_REVIEW_V1',**base(657),rowCount=533,enters=267,exits=266,unpaired=[list(k) for k in opened],lastRows=rows[-8:],lastPassedArguments=shadow,firstToLastTraceNs=int(rows[-1]['wallNs'])-int(rows[0]['wallNs']),cacheReceipts=receipts,
  noArchivedChildExitCodeOrExceptionDump=True,existingHostInspection=get(inputs[-1]),sourceCuts=[cuts('test/t496/external_source_diff.cpp',['a.hostNow = []','g_hostNowNs.load();','RunCreateOnly(']),cuts('test/t496/run_create_only.cmd',['900','Start-Process','LAUNCH_AT']),cuts('test/t496/gl_forward_sink.cpp',['void GL_APIENTRY glDispatchCompute'])],inputs=[fp(p) for p in inputs])
 return contract,e
def collect658():
 helper='test/t426/PrepareReadinessV4.ets';built='.codex/local-test/t598-final/entry/src/main/ets/render/PrepareReadinessV4.ets';fault='test/reports/t491/t592b/jscrash-160328.txt'
 s=text(helper);assert s==text(built) and 'JSON.parse(JSON.stringify(o)) as PrepareAttemptOwner' in s and 'before.same(after)' in s
 assert 'same is not callable, same is undefined' in text(fault) and 'Pid:35376' in text(fault)
 fields=['prepareId','pageGeneration','branch','windowId','windowGeneration','controller','session','surface','context','run']
 contract=dict(schema='T658_PREPARE_OWNER_DATA_CONTRACT_V1',**base(658),classification='PRODUCT_ENGINEERING_PROTOTYPE_LOSS; minimal developer fix',
  uniqueImplementation='method-free owned primitive snapshot plus explicit field comparator and canonical tuple key; no method invocation on serialized/foreign objects',
  ownerFields=[dict(name=x,type='string' if x=='run' else 'number with exact existing integer-domain validity') for x in fields],canonicalKey=dict(format='JSON array of ten primitives',order=fields,notAuthority=True),
  operations=['copyPrepareOwner: validate primitives then explicit fresh field copy, never alias original','samePrepareOwner: explicit strict equality of all10 fields, independent of prototype/key insertion order','bindOwner: first copy once/checked ownerSerial, later same fields unchanged; mismatch hard invalidates','changed(reason): preserve parent change semantics; never compare one owner object with itself to infer fresh identity','noteChange: compare genuine before/after owned values; differing owner revokes','readiness/snapshot changes revoke via unchanged strict predicate even if owner IDs same','check ticket and consume compare prepare/page/observation/input/ownerSerial/ownerKey on same UI stack'],
  callbacks='actual window reference+generation, prepare/page/branch bind at registration, stale callbacks discard without revoking new attempt',
  preserved=['single authoritative gate','reserve disabled except actual fresh valid READY','3000ms applies PREPARING only, no new READY TTL','RESERVED before consume; failed consume invalidates capture','strict PNG, HOLD/ROI/geometry and original calibration thresholds'],
  artifacts='first raw Error stage/code/name/message and before/after owner+scope in bounded cold receipt, unknown not synthesized',
  forbidden=['structuredClone claimed to preserve custom class methods','swallow TypeError and continue READY','unconditional reconstruction with guessed identity','global JSON-clone refactor unrelated to this path'],
  files=[helper,'test/t574/t574_prepare_v4_overlay.cjs','actual page/frame/window preparation producer and generated product page','PrepareReadiness parent only if needed to preserve original predicate/error receipt'],
  postRole='developer complete minimal fix+compile; T592 tester independent same-version prepare/reserve/HOLD/strict PNG',
  evidenceLimit='current stack confirmed only; every-click/v1 two-run same-cause assertions require their own original version/crash evidence')
 inputs=['docs/test-report-T592-v2.md','docs/architecture-T625-v1.md','docs/architecture-T586-first-attribution-v1.md','test/reports/t491/t592b/v2-jscrash.json',fault,helper,built,'test/t574/t574_prepare_v4_overlay.cjs','.codex/local-test/t598-final/entry/src/main/ets/render/PrepareReadiness.ets','.codex/local-test/t598-final/entry/src/main/ets/pages/P051DerivedV2Demo.ets','test/reports/t257/T592-rev2.json']
 pkg=get(inputs[-1]);packages=[]
 for name in ['input','signed']:
  p=Path(pkg[name]['path']);b=p.read_bytes();actual=sha(b)
  with zipfile.ZipFile(p) as z:abc=z.read('ets/modules.abc')
  packages.append(dict(kind=name,path=str(p),actualSHA=actual,recordedSHA=pkg[name]['sha256'],matches=actual==pkg[name]['sha256'],abcSHA=sha(abc)))
 assert all(x['matches'] for x in packages) and packages[0]['abcSHA']==packages[1]['abcSHA']
 e=dict(schema='T658_STATIC_SOURCE_AND_RAW_CRASH_V1',**base(658),packages=packages,rawCrash=dict(pid=35376,uniqueId='8041312369877275180',timestamp='2026-11-19 16:03:28.166',error='TypeError same is not callable, same is undefined',stack=['noteChange PrepareReadinessV4:18:97','changed PrepareReadinessV4:19:58','P051DerivedV2Demo:130:102']),sourceMatchesTemplate=True,
  sourceCuts=[cuts(helper,['same(','JSON.parse','noteChange','changed(','checkForReserve','consumeTicket']),cuts(inputs[-2],['TYPE_KEYBOARD','prepareGate.changed','new PrepareAttemptOwner','consumeTicket'])],inputs=[fp(p) for p in inputs])
 return contract,e
def main():
 for n,fn in [(655,collect655),(656,collect656),(657,collect657),(658,collect658)]:
  c,e=fn()
  for p,v in [(f'docs/architecture-T{n}-contract-v1.json',c),(f'docs/analysis-T{n}-evidence-v1.json',e)]:
   if '--record' in sys.argv:emit(p,v)
   else:assert get(p)==v,p
  print(f'PASS: T{n} finite source/artifact/schema review only; no product/test/device execution')
if __name__=='__main__':main()
