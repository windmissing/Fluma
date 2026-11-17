"""Read-only source/raw-evidence audit plus finite architect contract vectors.

No native API, GPU, device, product test, generator or old report is executed.
Only this task's four JSON artifacts are written.
"""
from pathlib import Path
from collections import Counter
import hashlib
import json
import re
import struct
import sys

sys.stdout.reconfigure(encoding='utf-8')
ROOT=Path(__file__).resolve().parents[1]
DOC=ROOT/'docs'
FROZEN=ROOT/'.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident'


def text(path):
    return Path(path).read_text(encoding='utf-8-sig',errors='replace')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fp(path):
    p=Path(path)
    return dict(path=str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p),bytes=p.stat().st_size,sha256=sha(p.read_bytes()))


def write(name,value):
    (DOC/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def jsonlines(path):
    return [json.loads(s) for s in text(path).splitlines() if s.strip()]


class Checks:
    def __init__(self): self.rows=[]
    def yes(self,name,ok,**detail):
        assert ok,(name,detail)
        self.rows.append(dict(id=name,result='PASS_FINITE_ENGINEERING_ONLY',**detail))


STAGES=['CB_INFO','ARG_COUNT','IS_TYPEDARRAY_API','IS_TYPEDARRAY_VALUE','TYPEDARRAY_INFO',
        'ELEMENT_TYPE','ELEMENT_COUNT','BYTE_OFFSET','DATA_POINTER','DETACHED_API','DETACHED_VALUE',
        'BACKING_INFO','BASE_POINTER','BACKING_BYTES','WORD_COPY',
        'HEADER_REVISION','FEATURES','PROFILE','FULL_FLAG','COLD_REVISION','COLD_VALUES']


def audit546():
    c=Checks()
    m3=text(ROOT/'entry/src/main/cpp/gpu_resident/m3a/napi.inc')
    g9=text(ROOT/'entry/src/main/cpp/gpu_resident/g9spf/napi.inc')
    page=text(ROOT/'entry/src/main/ets/pages/GpuResidentG9.ets')
    crash=text(ROOT/'test/reports/t491/t542/jscrash-latest.log')
    c.yes('crash_confirms_start_json_syntax_error','Pid:30232' in crash and 'Reason:SyntaxError' in crash and 'GpuResidentG9.ets:43:25' in crash)
    c.yes('G9_delegates_shared_m3Words','return m3Words(e,info,out);' in g9)
    c.yes('strict_type_count_offset_and_backing_are_present',all(s in m3 for s in
          ['type!=napi_uint32_array','length!=N','||offset||','detached','base!=pointer','bytes!=N*4']))
    start=page[page.index('private start('):page.index('tick(frame:')]
    c.yes('page_start_has_unguarded_JSON_parse','JSON.parse(native.g9ResearchConfig(' in start and 'try {' not in start)
    sdk=Path('C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/sysroot/usr/include/napi/native_api.h')
    c.yes('SDK_length_is_elements','@param length The number of elements in the TypedArray.' in text(sdk))
    m3words=[4,15,192,64,7680,8388608,1048576,65536,1048576,0,0,120,524288,1048576,1,0]
    c.yes('expected_M3_words_first_branch_valid',m3words[0]==4 and m3words[1]==15 and (m3words[2],m3words[3]) in [(192,64),(192,128),(512,64),(512,128)] and m3words[15]<=1)
    c.yes('expected_G9_request_valid',[192,0][0] in [192,512] and [192,0][1]<=1)
    # Ordered result table: not execution or emulation of an actual NAPI VM.
    def classify(states):
        for name in STAGES:
            if states.get(name,'PASS')!='PASS':return name
        return 'ACCEPT'
    vectors=[]
    for name in STAGES:
        outcome=classify({name:'FAIL'})
        c.yes('first_stage_'+name,outcome==name)
        vectors.append(dict(givenFailedStages=[name],expectedFirstStage=name,laterStages='NOT_EVALUATED'))
    c.yes('first_rejection_wins',classify({'TYPEDARRAY_INFO':'FAIL','BASE_POINTER':'FAIL'})=='TYPEDARRAY_INFO')
    c.yes('do_not_weaken_to_either_unit',classify({'ELEMENT_COUNT':'FAIL'})!='ACCEPT')
    def parse_config(raw):
        try: value=json.loads(raw)
        except (ValueError,TypeError):return 'NON_JSON'
        if not isinstance(value,dict):return 'ROOT_OBJECT_REQUIRED'
        if value.get('revision')=='GPU_NAPI_ERROR_V1':return 'CONTROLLED_NATIVE_ERROR'
        if value.get('revision')!='T529_EXPLICIT_RESEARCH_CANDIDATE_1':return 'REVISION'
        w=value.get('words')
        if not isinstance(w,list) or len(w)!=24:return 'WORDS_LENGTH'
        if any(type(x) is not int or not 0<=x<=0xffffffff for x in w):return 'WORD_TYPE_RANGE'
        return 'ACCEPT'
    good=dict(revision='T529_EXPLICIT_RESEARCH_CANDIDATE_1',words=[0]*24)
    samples=[('legacy_token','G9_RESEARCH_CONFIG_REJECTED','NON_JSON'),('empty','','NON_JSON'),
             ('malformed','{"ok":','NON_JSON'),('null','null','ROOT_OBJECT_REQUIRED'),
             ('array','[]','ROOT_OBJECT_REQUIRED'),('number','1','ROOT_OBJECT_REQUIRED'),
             ('json_string','"G9_RESEARCH_CONFIG_REJECTED"','ROOT_OBJECT_REQUIRED'),
             ('error',json.dumps(dict(revision='GPU_NAPI_ERROR_V1',ok=False,code='G9_RESEARCH_CONFIG_REJECTED')),'CONTROLLED_NATIVE_ERROR'),
             ('good',json.dumps(good),'ACCEPT'),('unknown_revision','{"revision":"UNKNOWN","words":[]}','REVISION'),
             ('short_words',json.dumps(dict(revision=good['revision'],words=[0]*23)),'WORDS_LENGTH')]
    for label,bad in [('negative',-1),('fraction',1.5),('overflow',2**32),('unsafe',2**53),('boolean',True),('null_word',None)]:
        obj=dict(good);obj['words']=[bad]+[0]*23;samples.append((label,json.dumps(obj),'WORD_TYPE_RANGE'))
    for label,raw,expected in samples:
        c.yes('safe_reply_'+label,parse_config(raw)==expected)
        vectors.append(dict(id='safe_reply_'+label,input=raw,expected=expected))
    snapshot=dict(revision='GPU_INPUT_DIAGNOSTIC_V1',route='G9',api='g9ResearchConfig',
        boot='18446744073709551615',diagGeneration='18446744073709551615',attemptId='18446744073709551615',
        expectedN=566,expectedBytes=2264,firstFailedStage='COLD_VALUES',
        stages={name:dict(status=2147483647,result='NOT_EVALUATED') for name in STAGES},
        typed=True,type=4294967295,length='18446744073709551615',offset='18446744073709551615',
        dataPresent=True,basePresent=True,baseEqualsData=False,backingBytes='18446744073709551615',
        detached=False,copyComplete=True,headerWords=[4294967295]*16,rawError='\x01'*128)
    wire=json.dumps(dict(snapshot=snapshot,firstFailureRef=dict(attemptId=snapshot['attemptId'],stage='COLD_VALUES')),separators=(',',':')).encode()
    c.yes('worst_fixed_snapshot_under_4096',len(wire)<=4096,bytes=len(wire))
    c.yes('snapshot_native_credit',3*2*512==3072)
    paths=['docs/test-report-T542-g9-device-eligibility-v1.md','docs/test-report-T545-device-deadline-v1.md',
           'test/reports/t491/t542/jscrash-latest.log','test/reports/t491/t542/crash-hilog.txt',
           'test/reports/t491/t542/faultlog-hilog.txt','entry/src/main/cpp/gpu_resident/m3a/napi.inc',
           'entry/src/main/cpp/gpu_resident/m3b/napi.inc','entry/src/main/cpp/gpu_resident/g9spf/napi.inc',
           'entry/src/main/ets/pages/GpuResidentG9.ets','entry/src/main/ets/pages/GpuResidentM3.ets',
           'entry/src/main/ets/pages/GpuResidentM5.ets','test/t522/bridge_overlay.cjs','test/t529/bridge_overlay.cjs','test/t517/bridge_overlay.cjs']
    sources=[fp(ROOT/p) for p in paths]+[fp(sdk)]
    delivered=[]
    marker='template<size_t N>bool m3Words('
    original=m3[m3.index(marker):m3.index('\n}',m3.index(marker))+2]
    for name in ['t522-delivery','t529-delivery','t529-development']:
        p=ROOT/'.codex/local-test'/name/'entry/src/main/cpp/gpu_resident/app_bridge.cpp'
        if p.exists():
            s=text(p);entry=dict(tree=name,file=fp(p),helperFound=marker in s)
            if marker in s:
                body=s[s.index(marker):s.index('\n}',s.index(marker))+2]
                entry.update(normalizedHelperSha256=sha(body.encode()),equalsCanonical=body==original)
            delivered.append(entry)
    manifest=dict(schema='T546_INPUT_REJECTION_ENGINEERING_V1',date='2026-11-17',status='ENGINEERING_READY_DEVICE_PREDICATE_UNKNOWN',
       known=['G9 start unguarded JSON.parse','shared decoder hides first rejection','configure result not gated before frame scheduling'],
       notProved=['specific failed NAPI predicate','M3 and G9 same root cause','shader/physics/OOM failure'],
       expectedRequests=dict(M3Configure=m3words,G9ResearchConfig=[192,0]),
       decode=dict(stages=STAGES,strictType='napi_uint32_array',lengthUnit='elements',expectedBytes='checked(N*4)',
                   offset=0,baseEqualsDataRequired=True,detachedApiMustSucceed=True,fallbackNumberArray=False,
                   outputs='VALID / API_FAILED / NOT_EVALUATED',copy='synchronous fixed native array only after every gate'),
       diagnostics=dict(endpoint='gpuInputDiagnosticsV1(route:string,attemptId?:string):string',routes=['M3A','M3B','G9'],
                        snapshotBytes=512,perRouteSnapshots=2,nativePayloadBytes=3072,responseUtf8Max=4096,
                        expandedSnapshotsPerReply=1,firstFailure='separate reference; selected by exact attemptId',
                        attemptId='nonzero monotonic U64 decimal string; exhaustion is error',businessFirstCauseUnchanged=True),
       errorJSON=dict(revision='GPU_NAPI_ERROR_V1',ok=False,required=['state','code','api','route','attemptId','diagnosticRevision'],
                      nativePendingException='preserve exception/status; UI catch; never fake ordinary success',budgetBytes=4096),
       legacyStatusApisRemainStrings=['m3Configure','g9Configure','pulse','target','offer'],
       successJSONUnchanged=True,UI=dict(parseRoot='non-null object, not array/primitive',validateWords='exact length + safe integers U32 before construction',
           start='catch entire event including cleanup; schedule frame only after exact accepted configure token',cleanup='preserve original error; cleanup failure separately'),
       sourceOwnership=['entry/src/main/cpp/gpu_resident/napi_words_contract.h (new)','m3a/napi.inc','m3b/napi.inc','g9spf/napi.inc',
                        'native type declarations','G9/M3/M5 pages and cold parsers','T522/T529/T517 overlay generators'],
       handoff=dict(developer='development compile commit only; no test; no device',tester='new signed package then minimal one request/route with raw predicate statuses, device serial; return to architect'),
       unchanged=dict(rootDeadlineNs=250000000,gapNs=33333333,quotaHz=120,displayTargetFps=30,CPUStopped=True,render='XComponent+shader only'),
       evidence='analysis-T546-rejection-evidence-v1.json')
    write('architecture-T546-input-rejection-manifest-v1.json',manifest)
    write('analysis-T546-rejection-evidence-v1.json',dict(schema='T546_FIRST_ATTRIBUTION_EVIDENCE_V1',date='2026-11-17',
       deviceUsed=False,productTestsRun=False,status='ENGINEERING_ATTRIBUTION_COMPLETE_EXACT_DEVICE_PREDICATE_UNKNOWN',
       originalCrash=dict(pid=30232,time='2026-11-17 11:25:49.830',reason='SyntaxError',location='GpuResidentG9.ets:43:25',
                          messageMasked=True,reportReproductions=3,localCrashOriginalsAudited=1),
       sources=sources,generatedHelperComparisons=delivered,cases=c.rows,vectors=vectors,
       manifestSha256=sha((DOC/'architecture-T546-input-rejection-manifest-v1.json').read_bytes()),
       preciseMissingEvidence=['same attempt UI type/length/offset/bytes','per NAPI raw statuses and valid output values','semantic branch observations','actual runtime/package/PID/boot identity'],
       notProductPass=True))
    return len(c.rows)


def pairing(rows):
    entered={};durations=Counter();errors=[]
    for row in rows:
        key=(row['callSeq'],row['call'])
        if row['event']=='ENTER':
            if key in entered:errors.append(['duplicate_enter',key])
            entered[key]=row
        elif row['event']=='EXIT':
            if key not in entered:errors.append(['exit_without_enter',key]);continue
            old=entered.pop(key);elapsed=row['wallNs']-old['wallNs']
            if elapsed<0:errors.append(['negative_elapsed',key])
            durations[row['call']]+=elapsed
    return dict(pairs=sum(r['event']=='EXIT' for r in rows),unmatchedEnters=list(entered),errors=errors,
                enterCounts=dict(Counter(r['call'] for r in rows if r['event']=='ENTER')),durationNs=dict(durations),
                firstWallNs=str(rows[0]['wallNs']),lastWallNs=str(rows[-1]['wallNs']),spanNs=str(rows[-1]['wallNs']-rows[0]['wallNs']))


def audit547():
    c=Checks();sources=[]
    p0=ROOT/'test/reports/t496/gltrace-C0.ndjson';p2=ROOT/'test/reports/t496/gltrace-C2.ndjson'
    raw0=p0.read_bytes();raw2=p2.read_bytes();a=jsonlines(p0);b=jsonlines(p2)
    c.yes('C2_contains_exact_C0_prefix',raw2.startswith(raw0) and b[:len(a)]==a)
    own2=b[len(a):];r0=pairing(a);r2=pairing(own2)
    c.yes('C0_actual_record_count',len(a)==378 and r0['pairs']==189)
    c.yes('C2_own_record_count',len(b)==660 and len(own2)==282 and r2['pairs']==141)
    c.yes('covered_calls_all_pair',not r0['unmatchedEnters'] and not r2['unmatchedEnters'] and not r0['errors'] and not r2['errors'])
    c.yes('C2_own_has_no_compile_link',r2['enterCounts']=={'glCreateProgram':141})
    c.yes('sink_run_arm_missing',all(r.get('runId')=='' and r.get('arm')=='' for r in a+b))
    host=text(ROOT/'test/t496/external_source_diff.cpp');sink=text(ROOT/'test/t496/gl_sink.inc');build=text(ROOT/'test/t496/build.cmd')
    c.yes('sink_shared_append_path_confirmed','std::ios::app' in sink and 't496-gltrace.ndjson' in sink and 't496sink::RunId()' not in host)
    c.yes('wait_heartbeat_is_coordinator',host.index('std::thread worker') < host.index('if (i % 15000 == 0) Emit("BOOT", "CREATE_WAIT"'))
    c.yes('layout_automatic_gate_empty',all((ROOT/'.codex/local-test/t496'/name).stat().st_size==0 for name in ['sizeof-driver.txt','sizeof-impl.txt']) and 'findstr /C:"sizeof"' in build)
    layouts={}
    for name in ['layout-driver.txt','layout-impl.txt']:
        s=text(ROOT/'.codex/local-test/t496'/name)
        match=re.search(r'class grs::m1::FullM1Transport\s+size\((\d+)\):(.*)',s,re.S)
        assert match,name
        layouts[name]=dict(size=int(match[1]),memberLines=[line.strip() for line in match[2].splitlines() if '|' in line])
    c.yes('raw_class_layouts_match_separately',layouts['layout-driver.txt']==layouts['layout-impl.txt'])
    runner=text(ROOT/'test/t496/run_create_only.cmd')
    c.yes('runner_target_and_kill_defects_present','\ncodex\\local-test' in runner and 'taskkill /f /im t496.exe' in runner)
    logs={n:jsonlines(ROOT/'test/reports/t496'/('log-'+n+'.ndjson')) for n in ['C0','C2']}
    c.yes('no_commit_assert_in_raw',all(not any(str(r.get('assertId','')).startswith('CREATE_RESOURCE_COMMIT') for r in rows) for rows in logs.values()))
    contextual=[]
    for arm in ['C0','C2']:
        for p in sorted((ROOT/'.codex/local-test/t496'/f'{arm}-files/gpu-program-cache/v1').glob('*.jsonl')):
            rows=jsonlines(p);own=[r for r in rows if str(r.get('packageSource','')).startswith('T496_')]
            contextual.append(dict(file=fp(p),directoryArm=arm,rows=len(rows),taggedT496Rows=len(own),
                packageSources=dict(Counter(r.get('packageSource','') for r in rows)),
                environmentSHAs=sorted({r.get('environmentSHA','') for r in own}),
                taggedStages=dict(Counter(r.get('stage','') for r in own)),
                status='CONTEXTUAL_MUTABLE_CACHE_RECEIPTS_NOT_UNIQUE_RUN_PROOF'))
    envs={e for r in contextual for e in r['environmentSHAs']}
    c.yes('contextual_environment_not_reported_mismatch',envs=={'2aabb8c88eabed25386ede5af0b3fd16cfe498a5c53ba42f5239900f2258a5b0'})
    header=ROOT/'.codex/local-test/t492-m1-delivery/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/m1_scene_sources.h'
    contents=text(header)
    shader={name:body.encode() for name,tag,body in re.findall(r'(\w+)\s*=R"(\w+)\((.*?)\)\2"',contents,re.S)}
    env=next(iter(envs));link='VERTEX+FRAGMENT|SEPARABLE=0|GLSL_EXPLICIT_ATTRIBUTES|NO_PRELINK_ATTRIBUTE_OR_OUTPUT_CALLS'
    abi='M1_SCENE_ABI_V1|color:vec4@0|Publication:std430:uint[]@0|uResolution:vec2|uBox:vec4|uMirror:bool|uColumn:uint|uLayer:int|uFrame:sampler2D|uOptics:vec4[4]'
    keyparts=[]
    for kind,name in [(35633,'kM1FullVertex'),(35632,'kM1SceneFragment')]:keyparts += [struct.pack('<I',kind),shader[name]]
    keyparts += [link.encode(),abi.encode(),env.encode()]
    exact_key=sha(b'T480_GRAPHICS_KEY_V1'+b''.join(struct.pack('<Q',len(x))+x for x in keyparts))
    c.yes('independent_current_SCENE_FULL_key',exact_key=='e3c4f777656fdd529b6a20ac19aa2df8975f8436677c4a544acd366ac4cfd9a5')
    seeds=[]
    for p in sorted((ROOT/'.codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1').glob('*.bin')):
        data=p.read_bytes();fmt,n=struct.unpack_from('<II',data,8)
        ok=data[:8]==b'T475B001' and len(data)==144+n and data[16:80].decode()==p.stem and data[80:144].decode()==sha(data[144:])
        c.yes('seed_envelope_'+p.stem[:8],ok)
        seeds.append(dict(file=fp(p),format=fmt,key=p.stem,payloadBytes=n,payloadSha256=sha(data[144:]),status='FILE_INTEGRITY_ONLY_NOT_CURRENT_GL_QUALIFICATION'))
    canonical=ROOT/'test/t468/M2CreateColdFiles.ets';delivered=ROOT/'.codex/local-test/t482-final/entry/src/main/ets/physics/M2CreateColdFiles.ets'
    code=text(canonical)
    c.yes('canonical_T468_writer_matches_T482',canonical.read_bytes()==delivered.read_bytes())
    c.yes('empty_allowed_but_crypto_update_unconditional','!/^[a-f0-9]*$/.test(hex)' in code and "await md.update({data:bytes})" in code and 'if(bytes.length===0)' not in code)
    empty_sha='e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855'
    c.yes('empty_sha_mathematical_identity',sha(b'')==empty_sha)
    raw=ROOT/'test/reports/t491/t461r2/create-raw0.bin'
    c.yes('local_empty_raw_original',raw.stat().st_size==0 and sha(raw.read_bytes())==empty_sha)
    layout=json.loads(text(ROOT/'test/reports/t491/t461r2/lA.json'));operations=[]
    def walk(v):
        if isinstance(v,dict):
            for k,x in v.items():
                if k=='text' and isinstance(x,str) and 'CREATE_EXPORT_FAILED' in x:operations.append(x)
                else:walk(x)
        elif isinstance(v,list):
            for x in v:walk(x)
    walk(layout)
    c.yes('raw_UI_update_failure_present',any('Error: update failed' in x for x in operations))
    units=text(ROOT/'test/t468/page_units.cjs')
    c.yes('old_mock_accepts_empty_crypto','b=Buffer.from(data)' in units and 'empty.log' in units)
    vectors=[]
    for name,hexdata,valid in [('empty','',True),('one','00',True),('nonempty','000afffe',True),('max','00'*4096,True),('odd','0',False),('bad','zz',False),('too_large','00'*4097,False)]:
        okay=len(hexdata)<=8192 and len(hexdata)%2==0 and re.fullmatch('[a-f0-9]*',hexdata) is not None
        c.yes('raw_domain_'+name,okay==valid)
        if okay:
            blob=bytes.fromhex(hexdata);outcome=dict(bytes=len(blob),sha256=sha(blob),cryptoUpdateCalls=0 if not blob else 1)
        else:outcome=dict(status='CREATE_RAW_FORMAT')
        vectors.append(dict(id=name,hexBytes=len(hexdata),expected=outcome))
    def receipt_permitted(blob,stages):
        required=['FORMAT','OPEN','FSYNC','CLOSE']
        if blob:required+=['WRITE','HASH_NONEMPTY']
        return sha(blob) if all(stages.get(name,True) for name in required) else None
    for stage in ['OPEN','WRITE','FSYNC','CLOSE','HASH_NONEMPTY']:
        # I/O failures of the empty domain and nonempty hash/write failures must
        # not produce the empty receipt. This checks the contract truth table.
        blob=b'\x00' if stage in ['WRITE','HASH_NONEMPTY'] else b''
        c.yes('failure_cannot_be_empty_success_'+stage,receipt_permitted(blob,{stage:False}) is None)
        vectors.append(dict(failAt=stage,expected='ERROR_PRESERVE_FIRST_CAUSE_NO_SUCCESS_RECEIPT'))
    api_groups={
        'egl':['eglGetPlatformDisplayEXT','eglInitialize','eglBindAPI','eglChooseConfig','eglCreateContext','eglCreatePbufferSurface','eglMakeCurrent','eglQuerySurface','eglQueryContext','eglSwapBuffers','eglDestroySurface','eglDestroyContext','eglTerminate','eglGetProcAddress','eglGetCurrentContext','eglGetCurrentDisplay','eglGetCurrentSurface','eglGetError'],
        'compile_binary':['glCreateShader','glShaderSource','glCompileShader','glCreateProgram','glAttachShader','glLinkProgram','glProgramParameteri','glProgramBinary','glGetProgramBinary','glGetProgramiv','glGetShaderiv','glGetShaderInfoLog','glGetProgramInfoLog','glDeleteProgram','glDeleteShader'],
        'environment':['glGetString','glGetStringi','glGetIntegerv','glGetInteger64v','glGetError'],
        'reflection':['glGetActiveUniform','glGetUniformLocation','glGetProgramInterfaceiv','glGetProgramResourceIndex','glGetProgramResourceiv','glGetActiveAttrib','glGetAttribLocation','glGetFragDataLocation','glIsProgram'],
        'resource_prepare':['glGenBuffers','glBindBuffer','glBufferData','glBufferSubData','glBindBufferBase','glCopyBufferSubData','glDeleteBuffers','glGenVertexArrays','glBindVertexArray','glDeleteVertexArrays','glUseProgram','glDispatchCompute','glMemoryBarrier','glFlush','glFinish','glFenceSync','glClientWaitSync','glDeleteSync','glMapBufferRange','glUnmapBuffer'],
        'scene_if_reached':['glGenFramebuffers','glBindFramebuffer','glDeleteFramebuffers','glGenTextures','glBindTexture','glDeleteTextures','glTexImage2D','glTexStorage2D','glTexParameteri','glFramebufferTexture2D','glCheckFramebufferStatus','glDrawArrays','glDrawArraysInstanced','glViewport']}
    direct_sources=[FROZEN/name for name in ['app_bridge.cpp','m1/full_transport.cpp','m1/source_transport.cpp',
        'm1/quantum_candidate.cpp','m1/scene.cpp','program_cache/program_factory.h','program_cache/graphics_program_factory.h',
        'm2/create_diagnostics.h']]
    direct_sources += [FROZEN.parent/'p013renderer/fullframe_targets.cpp']
    direct_calls=set()
    for p in direct_sources:
        assert p.exists(),str(p)
        direct_calls.update(re.findall(r'\b((?:gl|egl)[A-Z]\w*)\s*\(',text(p)))
    api_groups['uniform_exact']=sorted(name for name in direct_calls if name.startswith('glUniform'))
    api_groups['additional_direct_calls']=sorted(direct_calls-set(name for group in api_groups.values() for name in group))
    c.yes('all_named_direct_calls_catalogued',direct_calls<=set(name for group in api_groups.values() for name in group),count=len(direct_calls))
    high=[
        ('TRANSPORT_CREATE','m1/full_transport.cpp','FullM1Transport::Create','return TransportCode, first raw GL, allocatedBytes'),
        ('SOURCE_CREATE','m1/source_transport.cpp','SourceCoreTransport::CreateUsing','return TransportCode + resources/reflect/prepare'),
        ('INTERMEDIATE_CREATE','m1/quantum_candidate.cpp','QuantumCandidate::Create','return TransportCode + context'),
        ('FINAL_BUILD','m1/full_transport.cpp','programcache::Build(2','return bool/program/first GL'),
        ('REFLECT_PREPARE','program_cache/program_factory.h','ObservePrepare','original prepare status; no added GL read'),
        ('COLD_PREFIX','m1/full_transport.cpp','PrepareColdPrefix','return TransportCode/error + warmup wall'),
        ('SCENE_PREPARE','m1/scene.cpp','Scene::PreparePrograms','bool/PrepareFailure + generation/current check'),
        ('SCENE_ROLE_BUILD','program_cache/graphics_program_factory.h','BuildGraphics','each role bool/key/route/link/reflect/prepare'),
        ('RESOURCE_COMMIT','app_bridge.cpp','recovery.Resources','scope/current/resources result + first rejected predicate'),
        ('CAPTURE_EPOCH','app_bridge.cpp','captureCommittedEpoch=seen','committed epoch written only in real committed branch'),
        ('SESSION_HANDOFF','app_bridge.cpp','RecoverySessionLocked','expected serial/epoch/run/page + returned session'),
        ('FIRST_PAGE_PULSE','app_bridge.cpp','PagePulseLocked','sequence/UI/receipt/baseline + result'),
        ('READY','app_bridge.cpp','TryReadyV2Locked','phase/ready permission/current scope'),
        ('RETIRE_RETRY','app_bridge.cpp','auto retire=','previous attempt result, release/context retirement, next attempt relation')]
    seams=[]
    for ident,path,token,returns in high:
        p=FROZEN/path
        s=text(p)
        indexes=[i+1 for i,line in enumerate(s.splitlines()) if token in line]
        assert indexes,(ident,path,token)
        seams.append(dict(id=ident,source=str(p.relative_to(ROOT)),token=token,lines=indexes,returnEvidence=returns,
                          instrumentation='existing hook/forward first; missing high-level seam -> developer',tokenFound=bool(indexes)))
    paths=['docs/test-report-T496-create-only-v2.md','docs/test-report-T461-device-evidence-v2.md',
           'docs/architecture-T518-host-create-triage-v1.md','docs/architecture-T521-m2-zero-ledger-triage-v1.md',
           'test/t496/external_source_diff.cpp','test/t496/gl_sink.inc','test/t496/gl_forward_sink.cpp','test/t496/build.cmd','test/t496/run_create_only.cmd',
           'test/t488/gl_wrappers.inc','test/t485/gl_exports.inc','test/t324/gl_extra.cpp','test/t468/M2CreateColdFiles.ets','test/t468/generate.cjs','test/t468/page_units.cjs',
           'test/reports/t496/gltrace-C0.ndjson','test/reports/t496/gltrace-C2.ndjson','test/reports/t496/log-C0.ndjson','test/reports/t496/log-C2.ndjson',
           'test/reports/t491/t461r2/lA.json','test/reports/t491/t461r2/create-raw0.bin','test/reports/t491/t461r2/files-ls-before.txt','test/reports/t491/t461r2/files-ls-after-both.txt']
    sources=[fp(ROOT/p) for p in paths]+[fp(delivered),fp(header)]
    sources += [fp(p) for p in direct_sources if str(p.relative_to(ROOT)) not in {x['path'] for x in sources}]
    for p in [FROZEN/'app_bridge.cpp',FROZEN/'m1/full_transport.cpp',FROZEN/'m1/scene.cpp',FROZEN/'program_cache/program_factory.h',FROZEN/'program_cache/graphics_program_factory.h']:
        sources.append(fp(p))
    manifest=dict(schema='T547_CREATE_EXPORT_ENGINEERING_V1',date='2026-11-17',deviceUsed=False,
        status='ENGINEERING_TRIAGE_COMPLETE_CREATE_CALL_UNKNOWN_EXPORT_DEFECT_CONFIRMED',
        create=dict(frozenTree=str(FROZEN.relative_to(ROOT)),maxAttemptWallNs='900000000000',extendDeadline=False,
            sourceGate='nonempty raw size/member layout + showIncludes + full source/link/DLL/EXE hashes',
            runArms=['C0_EMPTY','C1_COMPUTE3','C2_ALL6'],armIsolation='new process, unique create-new output dir, no inherited receipts/traces',
            sink=dict(recordUtf8Max=4096,attemptFileBytesMax=32*1024*1024,
                identity=['runId','arm','pid','ownerTid','contextGeneration','createAttemptId','callId','parentCallId','functionId','role','key','wallNs','scriptNs'],
                activeCallPhases=['ENTER_EMIT','IN_API','RETURNED','EXIT_EMIT'],
                errorConsumption='forward original glGetError/eglGetError only; never extra reads',
                crossTU='single initialized process instance; no independent inline sink state',
                proof='same attempt durable ENTER + IN_API + no return; not a coordinator heartbeat'),
            apiGroups=api_groups,highLevelSeams=seams,
            cache='full exact source/ABI/link/environment tuple and 144-byte T475B001 envelope; only .bin seeded, no historical .jsonl/.tmp',
            current_SCENE_FULL_IndependentKey=exact_key,
            watchdog='exact child PID/tree only; no image/window wildcard kill; wall deadline including startup',
            handoff='tester owns fixture/sink/seed; missing high-level observation seam -> developer; no canonical business changes',
            readiness='resource commit first, then production session handoff + first pulse baseline; commit is not READY'),
        export=dict(canonical='test/t468/M2CreateColdFiles.ets',generator='test/t468/generate.cjs',
            rawHexLimit=8192,rawByteLimit=4096,emptyAllowed=True,emptySha256=empty_sha,
            emptyOrder=['validate','open/truncate','fsync','close','return bytes0 + exact empty SHA'],
            emptyCryptoUpdateCalls=0,nonempty='original crypto SHA256 over exact bytes; no empty fallback on failure',
            noSuccessBeforeAllDurable=True,existingManifestAndEndpointProtocolUnchanged=True,
            failureStages=['FORMAT','OPEN','WRITE','FSYNC','CLOSE','HASH','JSON','MANIFEST'],
            testerMock='empty crypto update throws; empty path must never invoke it',
            productCreateAndRoot='NOT_REACHED until native JSON/scope proof; no inference from raw0B'),
        developer='development compile commit only; no tests; no device',
        unchanged=dict(rootDeadlineNs=250000000,gapNs=33333333,quotaHz=120,displayTargetFps=30,CPUStopped=True,render='XComponent+shader only'),
        evidence='analysis-T547-create-export-evidence-v1.json')
    write('architecture-T547-create-export-manifest-v1.json',manifest)
    write('analysis-T547-create-export-evidence-v1.json',dict(schema='T547_READONLY_EVIDENCE_AUDIT_V1',date='2026-11-17',
        deviceUsed=False,productTestsRun=False,GLRun=False,unknowns=['specific Create/scene/commit failure','unique run qualified six-role warm success','M2 native Create code/firstFailure'],
        C0=dict(records=len(a),audit=r0),C2=dict(records=len(b),inheritedC0Records=len(a),ownRecords=len(own2),audit=r2),
        rawClassLayout=layouts,automaticLayoutGate='INVALID_EMPTY_COMPARISON',rawLogs=logs,
        contextualCacheReceipts=contextual,seedEnvelopeChecks=seeds,
        rawExport=dict(localRawCopiesAudited=1,bytes=0,sha256=empty_sha,reportedDeviceExports=3,
                       UIErrors=operations,canonicalMatchesDelivered=True,productStatus='NOT_REACHED'),
        sources=sources,cases=c.rows,exportContractVectors=vectors,
        manifestSha256=sha((DOC/'architecture-T547-create-export-manifest-v1.json').read_bytes()),notProductPass=True))
    return len(c.rows)


if __name__=='__main__':
    print(json.dumps(dict(status='PASS_FINITE_ENGINEERING_ONLY',checks=dict(T546=audit546(),T547=audit547()),
                         deviceUsed=False,productTestsRun=False,GLRun=False)))
