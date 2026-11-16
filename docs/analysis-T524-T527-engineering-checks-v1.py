"""Architect schema generation and finite contract checks. No CPU/GPU stepper or device use."""
from pathlib import Path
from fractions import Fraction
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT/'docs'


def read(name):
    return json.loads((DOCS/name).read_text(encoding='utf-8-sig'))


def fp(path):
    p = ROOT/path
    b = p.read_bytes()
    return dict(path=path, bytes=len(b), sha256=hashlib.sha256(b).hexdigest())


def save(name, data):
    (DOCS/name).write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


def segment(name, size, cursor, stride=4):
    assert cursor % 16 == size % 16 == 0
    return dict(name=name, byteStart=cursor, byteEnd=cursor+size, bytes=size, strideBytes=stride)


def contiguous(parts):
    assert parts[0]['byteStart'] == 0
    for i, p in enumerate(parts):
        assert p['byteEnd']-p['byteStart'] == p['bytes']
        assert p['byteStart'] % 16 == 0
        if i:
            assert p['byteStart'] == parts[i-1]['byteEnd']
    return parts[-1]['byteEnd']


def g9_scope(role, j):
    out = []
    for name in ('INIT_COPY', 'INPUT_AND_CFL', 'GRAVITY_WORK', 'TOUCH_ATOMIC', 'AFTER_TOUCH_CFL'):
        out.append(dict(op=name, role=role, Jslot=j, micro=-1, bindingCount=4))
    stages = ('ACTIVATE_REQUEST', 'ACTIVATE_APPLY', 'FLIGHT_PATH', 'PARTICLE_FOLD',
              'FACE_FLUX_MOBILITY', 'FLOW_ORIGIN_RESERVE', 'FLOW_PAIR_APPLY',
              'FRICTION_REST_DEPOSIT', 'ORDERED_CAPTURE', 'MICRO_AUDIT_SEAL')
    for micro in range(j):
        for name in stages:
            out.append(dict(op=name, role=role, Jslot=j, micro=micro, bindingCount=4))
    for name in ('FINAL_MASS_ORIGIN_MOMENTUM', 'FINAL_ENERGY_CURSORS', 'SCOPE_SEAL_AND_CHOICE'):
        out.append(dict(op=name, role=role, Jslot=j, micro=-1, bindingCount=4))
    assert len(out) == 8+10*j
    return out


def g9_schedule():
    out = []
    def root(name):
        out.append(dict(op=name, role='ROOT', Jslot=0, micro=-1, bindingCount=4))
    for n in ('ROOT_INIT', 'BASE_FREEZE', 'CONFIG_INPUT_GATE', 'TRIAL_SCHEDULE_BEGIN'):
        root(n)
    for j in (1,2,4,8,16,32):
        out.extend(g9_scope('DISCOVERY', j))
    root('DISCOVERY_COMMIT_SELECTION'); root('AUTHORITATIVE_BEGIN')
    out.extend(g9_scope('AUTHORITATIVE', 32))
    root('REJECT_PREFIX_BEGIN')
    for j in (1,2,4,8,16,32):
        out.extend(g9_scope('REJECTED_PREFIX_CHECK', j))
    root('REPLAY_BEGIN')
    out.extend(g9_scope('REPLAY', 32))
    for n in ('COMPARE_SELECTED_AUTHORITATIVE', 'COMPARE_REPLAY_STREAM', 'PACK_SCENE_METADATA', 'ROOT_SEAL'):
        root(n)
    for i, row in enumerate(out):
        row['index'] = i
    assert len(out) == 2024
    return out


def g9_manifest():
    design = read('design-T515-three-state-manifest-v1.json')
    profiles = []
    for r in design['resources']:
        k = r['K']
        state_spec = [('header256',1024,4), ('lastSourceInput128',512,4),
                      ('lastImpulseInput128',512,4), ('column32',128*128,128),
                      ('bedCohort4',128*64*16,16), ('flowOrigin4',128*64*16,16),
                      ('packet64',k*256,256), ('sourceRow16',4096*64,64)]
        state, at = [], 0
        for name, size, stride in state_spec:
            state.append(segment(name,size,at,stride)); at += size
        assert contiguous(state) == r['stateBytes']
        work, at = [], 0
        for name, size in r['workBytesBySegment'].items():
            work.append(segment(name,size,at)); at += size
        assert at == r['workBytes']
        extras = [('researchConfigAndQuota',1536), ('rootControlExtension',512),
                  ('columnWideLedgers',9216), ('particleContactStaging',32768),
                  ('originAuditExtension',65536), ('stageReplayDiagnostics',5120)]
        for name, size in extras:
            work.append(segment(name,size,at)); at += size
        assert sum(s for _,s in extras) == 114688
        assert contiguous(work) == r['workBytes']+114688
        arena = [segment('event64',r['eventsPerArena']*256,0,256),
                 segment('path16',r['pathsPerArena']*64,r['eventsPerArena']*256,64)]
        assert contiguous(arena) == r['arenaBytes']
        coldroot = 32768 + 2*r['stateBytes'] + 2*r['arenaBytes']
        coldfull = coldroot + 6*r['arenaBytes']
        profiles.append(dict(K=k, stateSegments=state, stateBytes=r['stateBytes'], stateBanks=5,
                             workSegments=work, workBytes=at, arenaSegments=arena,
                             arenaBytes=r['arenaBytes'], normalArenas=4,
                             eventCapacity=r['eventsPerArena'], pathCapacity=r['pathsPerArena'],
                             designPayloadBytes=r['payloadBytes'], engineeringWorkExtraBytes=114688,
                             payloadBytes=5*r['stateBytes']+4*r['arenaBytes']+at,
                             fullTrialTraceGPUAdditionalBytes=6*r['arenaBytes'],
                             maxBoundObjectBytes=max(at,r['arenaBytes'],r['stateBytes']),
                             coldRootSlotBytes=coldroot, coldFullSlotBytes=coldfull,
                             coldTwoFullSlotsBytes=2*coldfull,
                             coldFullPageMax=1+(coldfull+4095)//4096+32,
                             coldFullArchiveUtf8Max=(2+(coldfull+4095)//4096+32)*16384))
    names = ['magic','revision','words','seal','profile','rootKind','outcome','reason',
             'nonceLo','nonceHi','runLo','runHi','ownerLo','ownerHi','contextLo','contextHi',
             'surfaceLo','surfaceHi','geometryLo','geometryHi','configLo','configHi',
             'inputGenerationLo','inputGenerationHi','pageSessionLo','pageSessionHi',
             'planIdLo','planIdHi','baseCommitLo','baseCommitHi','proposedCommitLo','proposedCommitHi',
             'baseBank','candidateBank','arena','arenaGenerationLo','arenaGenerationHi',
             'selectedJ','eligibleJ','completedDispatches','requiredDispatches','barriers',
             'auditMask','eventPlanned','eventSealed','pathPlanned','pathSealed','trialValidMask',
             'firstErrorStage','firstErrorMicro','firstErrorEntity','firstErrorOperand','originalGL','fenceStatus',
             'acceptedQuantaLo','acceptedQuantaHi','totalMassLo','totalMassHi',
             'sourceSeqLo','sourceSeqHi','impulseSeqLo','impulseSeqHi','sourceAdmittedLo','sourceAdmittedHi']
    for prefix in ('parameterSha','layoutSha','shaderSha','inputSha'):
        names += [prefix+str(i) for i in range(8)]
    for prefix in ('movingPx','movingPy','gravityWork','touchWork','sourceWork','dissipationLower'):
        names += [prefix+str(i) for i in range(4)]
    for t in range(6):
        names += [f'trial{t}_{s}' for s in ('J','outcome','firstStage','firstMicro','firstEntity','firstOperand')]
    assert len(names) <= 256
    names += [f'reserved_{i}' for i in range(len(names),256)]
    source = ['SOURCE_INIT_BASE','SOURCE_VALIDATE_INPUT','SOURCE_DEDUP','SOURCE_DECODE',
              'SOURCE_CREDIT','SOURCE_PRIVATE_BIRTH','SOURCE_ORIGIN_CERTIFICATE','SOURCE_LEDGER',
              'SOURCE_AUDIT_REPLAY','SOURCE_COMPARE','SOURCE_PACK_SCENE','SOURCE_SEAL']
    return dict(schema='T524_G9SPF_ENGINEERING_V1',date='2026-11-16',status='ENGINEERING_READY_NOT_IMPLEMENTED',
                route='gpu-spf-depth-averaged-coupling-v1',profile='G9_SPF_Q20_V1',enabledByDefault=False,
                design=fp('docs/design-T515-three-state-manifest-v1.json'), designTables=design,
                identities={kind:dict(magic=int.from_bytes(code.encode(),'little'),revision=1)
                            for kind,code in [('state','G9S1'),('event','G9E1'),('path','G9P1'),('input','G9I1'),('completion','G9C1')]},
                seal=0xC0DE0901,profileCode=int.from_bytes(b'SPF1','little'),
                profiles=profiles, sourcePasses=[dict(index=i,op=n,bindingCount=4)for i,n in enumerate(source)],
                quantumPasses=g9_schedule(),sourceDispatch=12,quantumDispatchMax=2024,planDispatchMax=8108,
                maxBindings=4,workgroupInvocations=64,workgroupsPerPass=1,sharedBytesMax=4096,
                copyCallsSeparateFromDispatch=True,semanticTrialIndex='selected discovery trial index preserved in authoritative/replay',
                copies=dict(baseFreeze='one complete state/root', selected='one state and valid event/path pair',
                            authoritativeAndReplay='scope init copies full BASE each execution'),
                completion256=names,requiredAuditMask=65535,
                outcomeControlMap={'ACCEPTED_QUANTUM':'AcceptQuantum once after full gate',
                                   'ACCEPTED_SOURCE':'zero quota, source receipt accepted',
                                   'ALREADY_CONSUMED':'metadata only; never repeat physical',
                                   'SOURCE_ZERO_ADMISSION':'metadata source receipt, zero external mass',
                                   'G9_NUMERIC_OR_CAPACITY_BLOCKED':'typed normal blocked; never integer-cast to V4 outcome',
                                   'FAULT':'fault/revoke; never normal blocked'},
                impulseAloneRearmsBlocked=False,clockRevision='GRS_M3_PROGRESS_CLOCK_V3',
                rootDeadlineNs=250000000,gapNs=33333333,quotaHz=120,displayTargetFps=30,
                rootFenceCount=1,completionReadBytes=1024,
                controlHostStateAndSnapshotExtraBytes=2048,
                cold=dict(capability='G9_SPF_COLD_V1',slots=2,builderMax=1,readerMax=1,
                          maxCumulativeCapacityBytes=max(x['coldTwoFullSlotsBytes']for x in profiles),
                          ttlFromRequestNs=60000000000,pageWords=1024,pageUtf8Bytes=16384,
                          descriptorMax=32,oldReaderPolicy='reject unknown; no invented V5 provenance stream'),
                independentPipeline=True,requiresV5Implementation=False,
                productParametersPending=design['productParametersAndThresholdsPending'],
                exclusions=['GL alignment and driver/JIT','cache binaries','FBO 8wh and resize overlap',
                            'native cold slots and JS/files','other resident profiles','device timing and effect calibration'])


def v5_manifest():
    v5 = read('architecture-T512-v5-engineering-manifest-v1.json')
    v4 = read('architecture-T498-m3-engineering-manifest-v1.json')
    context_spec = [('control96',384,4),('dedup64x4096',4096*64*4,256),
                    ('sourceLedger16x4096',4096*16*4,64),('rawBulk512x4096',4096*512*4,2048),
                    ('sourceMeta32x4096',4096*32*4,128)]
    context, at = [], 0
    for name,size,stride in context_spec:
        context.append(segment(name,size,at,stride)); at += size
    assert contiguous(context) == 10224000
    profiles = []
    for p, b in zip(v5['profiles'],v4['profiles']):
        k,kw,s,e,ea = p['K'],p['Kw'],p['stateBytesV5'],p['physicalRowsPerScope'],p['provenanceRowsPerScope']
        src,phy,aux,c,h,d = b['source128ArenaBytes'],(k+4*e)*256,4*ea*128,8192,32768,at
        common = [('prefix',h,4),('FINAL_STATE',s,4),('FINAL_SOURCE_CONTEXT',d,4),
                  ('SOURCE_FRAME128',src,512),('PHYSICAL_FRAME64',phy,256),('PROVENANCE_FRAME32',aux,128)]
        def layout(full):
            spec = list(common)
            spec += [(f'CONTROL_ROOT_{i}',c,512)for i in range(5 if full else 1)]
            if full:
                for i in range(5):
                    spec += [(f'BASE_STATE_{i}',s,4),(f'BASE_SOURCE_CONTEXT_{i}',d,4)]
                for q in range(4):
                    for role in range(1,6):
                        spec += [(f'Q{q}_ROLE{role}_PHYSICAL',e*256,256),
                                 (f'Q{q}_ROLE{role}_PROVENANCE',ea*128,128)]
            parts, cursor = [], 0
            for name,size,stride in spec:
                parts.append(segment(name,size,cursor,stride));cursor += size
            assert len(parts) <= 64 and contiguous(parts) == cursor
            return parts,cursor
        normal,n = layout(False); full,f = layout(True)
        assert n == h+s+d+src+phy+aux+c
        assert f == h+6*(s+d)+src+phy+aux+5*c+20*(e*256+ea*128)
        extra = 20*(e*256+ea*128)+5*(s+d)+5*c+256
        oldfull = b['fullBranchTraceExtraBytes']+p['fullBranchTraceAdditionalBytes']
        assert 20*(e*256+ea*128) == oldfull
        profiles.append(dict(K=k,Kw=kw,stateBytes=s,sourceContextBytes=d,
                             sourceRows=src//512,physicalRowsPerScope=e,auxRowsPerScope=ea,
                             framePhysicalRows=k+4*e,frameAuxRows=4*ea,
                             rootSegments=normal,fullSegments=full,rootSlotBytes=n,fullSlotBytes=f,
                             twoRootSlotsBytes=2*n,twoFullSlotsBytes=2*f,
                             rootPageMax=1+(n+4095)//4096+64,
                             fullPageMax=1+(f+4095)//4096+64,
                             fullArchiveUtf8Max=(2+(f+4095)//4096+64)*16384,
                             rootArchiveUtf8Max=(2+(n+4095)//4096+64)*16384,
                             residentPayloadBytes=p['payloadBytes'],existingT512FullGPUExtraBytes=oldfull,
                             fullGPUReservationBytes=extra,
                             knownGPUPayloadWithFullBytes=p['payloadBytes']+extra,
                             sourceContextBaseAnchorCopyBytesMax=5*(s+d),
                             extraPhysicsDispatch=0,baseAnchorMax=5,
                             fullGPUBufferObjectMaxBytes=max(d,s,src,phy,aux,e*256,ea*128)))
    fields = ['kind','rootIndex','role','trial','branchEpochLo','branchEpochHi','arena',
              'generationLo','generationHi','byteOffsetLo','byteOffsetHi','byteLengthLo','byteLengthHi',
              'strideWords','validRows','plannedRows','sealedRows','physicalFirst','physicalLast',
              'auxFirst','auxLast','flags','segmentRevision']+[f'sha256_{i}'for i in range(8)]+['seal']
    assert len(fields) == 32
    prefix_spec = [('header256',1024),('descriptors64x32',8192),('completion5x256',5120),
                   ('input5x64',1280),('quota5x256',5120),('cursor5x64',1280),
                   ('sourceCommand544',2176),('configAndRootRecords',8576)]
    prefix, cursor = [], 0
    for name,size in prefix_spec:
        prefix.append(segment(name,size,cursor));cursor += size
    assert contiguous(prefix) == 32768
    header = ['magic','revision','words','seal','physicalWire','features','channels','mode',
              'K','Kw','rootCount','segmentCount','flags','headerPage','dataPageCount','reserved15']
    for name in ('requestId','traceId','createdNs','deadlineNs','pid','epoch','run','context','surface',
                 'pageSession','resetSerial','permissionRevision','inputEpoch','actualBytes','capacityBytes',
                 'sourceContextBytes','planId','sourceBoundary'):
        header += [name+'Lo',name+'Hi']
    for name in ('parameterSha','layoutSha','shaderSha','declaredPackageSha','declaredElfSha','sourceDeclarationSha'):
        header += [name+str(i) for i in range(8)]
    header += [f'reserved_{i}'for i in range(len(header),256)]
    record = ['rootKind','outcome','firstErrorStage','firstErrorReason','requiredAuditMask','actualPasses',
              'publicationEligible','flags']
    for name in ('nonce','baseSourceCommit','basePhysicsCommit','proposedSourceCommit','proposedPhysicsCommit',
                 'frame','planId','rootStartedNs','rootDeadlineNs','sampleCheckedNs','sourceControlGeneration'):
        record += [name+'Lo',name+'Hi']
    record += ['completionPrefixWord','inputPrefixWord','quotaPrefixWord','cursorPrefixWord',
               'baseStateSegment','baseSourceContextSegment','sourceSegment','controlSegment',
               'firstScopeSegment','scopeCount','currentStateSegment','currentSourceContextSegment']
    for name in ('parameterSha','inputSha','rootTokenSha'):
        record += [name+str(i)for i in range(8)]
    record += [f'reserved_{i}'for i in range(len(record),128)]
    assert len(header)==len(set(header))==256 and len(record)==len(set(record))==128
    return dict(schema='T527_V5_COLD_ENGINEERING_V1',date='2026-11-16',status='ENGINEERING_READY_NOT_IMPLEMENTED',
                capability='T527_V5_BOUNDED_COLD_V1',payload='GRS_M3B_LINEAGE_COLD_V5_1',
                pageSchema='T527_SEGMENT_PAGE_V1',physicalWire=5,features=31,channelBitmap=15,
                sourceContextSegments=context,prefixSegments=prefix,descriptor32=fields,
                header256=header,headerMagic=int.from_bytes(b'V5C1','little'),headerRevision=1,
                rootRecord128=record,rootRecordCountMax=5,
                configRootSubsegments=[dict(name='rootRecords5x128',relativeStart=0,bytes=2560),
                                       dict(name='fullColdConfigCapsule',relativeStart=2560,bytes=4096),
                                       dict(name='zeroReserved',relativeStart=6656,bytes=1920)],
                descriptorMax=64,descriptorFlags={'NOT_EXECUTED':1,'SEALED_VALID':2,'FORENSIC_PENDING':4},
                descriptorSeal=0xC0DE0501,profiles=profiles,
                lineageDictionaries={k:v5['designTables'][k]for k in ('cohort4','certificate16','aux4','sensorInput64',
                                                                   'provenance32','physicalLiftEvent64','header256','completion256')},
                stateComparison='all full V5 words plus accepted source-context prefixes; no digest substitute',
                physicalAuxRule='paired sealed prefix; ENERGY/REQUEST physicalOrdinal0 else actual same-scope reference',
                slots=2,builders=1,readers=1,GPUFullRetentionSets=1,
                detachedBlockCredit='count until actual destructor/reset acknowledgment; Trim pointer move is not free',
                maxCumulativeSlotCapacityBytes=max(x['twoFullSlotsBytes']for x in profiles),
                worstKnownConcurrentPayload=dict(
                    gpuResidentAndFull=profiles[-1]['knownGPUPayloadWithFullBytes'],
                    nativeTwoSlots=2*profiles[-1]['fullSlotBytes'],
                    diagnosticCopyAndWriter=3501224,pageWriter=16384,
                    total=profiles[-1]['knownGPUPayloadWithFullBytes']+2*profiles[-1]['fullSlotBytes']+3501224+16384,
                    excludes=['alignment','FBO','JS','fixed objects','cache','driver retirement/JIT']),
                ttlFromRequestNs=60000000000,readRenewsTTL=False,terminalCount=32,
                pageWords=1024,pageUtf8Bytes=16384,pageDirectory='segment-local whole rows, header0 bounded summary',
                diagnostic=dict(captureBytesMax=1404072,writerUtf8Max=2097152,activeCopies=1,
                                fallbackUtf8Max=16384,v5SummaryUtf8Max=2048,
                                knownT507WorstFallbackBytes=13218,combinedDesignBoundBytes=15266,
                                timelineAndDiagnosticUtf8Max=4194304),
                legacyOnline={'V3':'T503 route m2 only; reject V5 IDs','V4':'explicit independent adapter required',
                              'unknown':'UNSUPPORTED_COLD_REVISION; never downgrade'},
                legacyOffline={'V3':'existing reader','V4':'its versioned reader or READER_NOT_IMPLEMENTED',
                               'V5':'this new complete lineage reader'},
                rootDeadlineNs=250000000,gapNs=33333333,requiresV4ToWaitForThis=False,
                sourceInputs=[fp('docs/architecture-T512-v5-engineering-manifest-v1.json'),
                              fp('docs/architecture-T498-m3-engineering-manifest-v1.json'),
                              fp('docs/architecture-T503-cold-manifest-v1.json')])


def main():
    g, v = g9_manifest(), v5_manifest()
    assert [p['payloadBytes']for p in g['profiles']] == [51120768,93803648]
    assert [p['rootSlotBytes']for p in v['profiles']] == [28524416,29577088,47166336,48219008]
    assert [p['fullSlotBytes']for p in v['profiles']] == [157537024,163853056,255871744,262187776]
    assert v['maxCumulativeSlotCapacityBytes'] == 524375552
    assert g['cold']['maxCumulativeCapacityBytes'] == 359342080
    assert v['profiles'][-1]['knownGPUPayloadWithFullBytes'] == 366564432
    assert v['profiles'][-1]['fullPageMax'] == 64076
    assert v['profiles'][-1]['fullArchiveUtf8Max'] == 1049837568
    assert v['worstKnownConcurrentPayload']['total'] == 894457592
    counts = {}
    for row in g['quantumPasses']:
        counts[row['role']] = counts.get(row['role'],0)+1
    assert counts == {'ROOT':12,'DISCOVERY':678,'AUTHORITATIVE':328,'REJECTED_PREFIX_CHECK':678,'REPLAY':328}
    assert g['designTables']['enums']['outcome']['G9_NUMERIC_OR_CAPACITY_BLOCKED'] == 4
    assert g['designTables']['enums']['outcome']['FAULT'] == 5
    assert len(g['completion256']) == len(set(g['completion256'])) == 256
    # Conservative simultaneous ownership arithmetic, no allocator/RSS claim.
    maxslot = v['profiles'][-1]['fullSlotBytes']
    pin_cases = []
    for old in (0,maxslot):
        for candidate in (0,maxslot):
            used = old+candidate
            assert used <= v['maxCumulativeSlotCapacityBytes']
            pin_cases.append(dict(oldReadyOrRetiringBytes=old,builderOrNewReadyBytes=candidate,total=used))
    assert maxslot*3 > v['maxCumulativeSlotCapacityBytes']
    # Numeric primitive boundary expectations only: no simulation implementation.
    rounding = [(-1,2,0),(-3,2,-2),(-5,2,-2),(1,2,0),(3,2,2),(5,2,2)]
    for n,d,expect in rounding:
        assert round(Fraction(n,d)) == expect
    interval_cases = 0
    for mass in (1,3,7,268435456):
        for p in (0,1,mass*1073741824):
            q,r = divmod(p*p,mass)
            lo = (q<<24)+(r<<24)//mass
            hi = lo + bool((r<<24)%mass)
            assert lo <= Fraction((p*p)<<24,mass) <= hi and hi-lo <= 1
            interval_cases += 1
    fixture = read('design-T515-three-state-fixtures-v1.json')
    assert len(fixture['cases']) == len({c['id']for c in fixture['cases']}) == 43
    for kind in g['designTables']['event']['kinds']:
        assert len(kind['words']) == 64 and kind['words'][63] == 'seal'
        for i, name in enumerate(kind['words']):
            known = bool(kind['usedMaskU32'][i//32] & (1<<(i%32)))
            assert known == (not name.startswith('reserved_'))
    save('architecture-T524-g9-spf-manifest-v1.json',g)
    save('architecture-T527-v5-cold-manifest-v1.json',v)
    names = ['docs/algorithm-T515-gpu-three-state-dynamics-v1.md',
             'docs/design-T515-three-state-manifest-v1.json','docs/design-T515-three-state-fixtures-v1.json',
             'docs/design-T515-three-state-evidence-v1.json','docs/architecture-T512-gpu-V5-lineage-lift-v1.md',
             'docs/architecture-T512-v5-engineering-manifest-v1.json',
             'docs/architecture-T498-m3-blocked-control-v2.md','docs/architecture-T498-m3-engineering-manifest-v1.json',
             'docs/algorithm-T508-m3-blocked-quota-v1.md','docs/architecture-T503-bounded-cold-protocol-v1.md',
             'docs/architecture-T503-cold-manifest-v1.json','docs/developer-T507-bounded-cold.md',
             'entry/src/main/cpp/gpu_resident/cold_trace.h','entry/src/main/cpp/gpu_resident/cold_protocol.h',
             'entry/src/main/cpp/gpu_resident/cold_diagnostics.h','test/t507/generate.cjs',
             'docs/architecture-T524-g9-spf-engineering-v1.md','docs/architecture-T527-v5-cold-lineage-v1.md',
             'docs/architecture-T524-g9-spf-manifest-v1.json','docs/architecture-T527-v5-cold-manifest-v1.json',
             'docs/analysis-T524-T527-engineering-checks-v1.py']
    save('analysis-T524-T527-engineering-checks-v1.json',dict(
        schema='T524_T527_ARCHITECT_STATIC_CHECKS_V1',date='2026-11-16',status='CONTRACT_CHECKS_PASS',
        product='NOT_IMPLEMENTED',device='NOT_RUN',GLExecuted=False,CPUSimulatorAdded=False,
        g9ScheduleRoleCounts=counts,g9Dispatches=2024,g9SourceDispatches=12,g9PlanDispatches=8108,
        g9PayloadBytes=[p['payloadBytes']for p in g['profiles']],
        v5RootSlotBytes=[p['rootSlotBytes']for p in v['profiles']],
        v5FullSlotBytes=[p['fullSlotBytes']for p in v['profiles']],
        v5MaxTotalNativeCapacityBytes=v['maxCumulativeSlotCapacityBytes'],
        checkedOwnershipCases=pin_cases,thirdSlotRejectedByCapacity=True,
        signedRNEBoundaryCases=rounding,energyIntervalArithmeticCases=interval_cases,
        g9DesignFixtureSchemaCount=43,g9EventUsedMasksChecked=14,
        limitations=['schema/layout/arithmetic contracts only; not native state-machine or GPU tests',
                     'real shared/compiler/SSBO/alignment/driver/allocator peak unmeasured',
                     '2024 dispatch timing/250ms feasibility and effects not validated',
                     'diagnostic fallback new V5 2048B adapter needs actual writer verification'],
        inputs=[fp(n)for n in names]))
    print(json.dumps(dict(status='CONTRACT_CHECKS_PASS',g9RoleCounts=counts,
                          g9PayloadBytes=[p['payloadBytes']for p in g['profiles']],
                          v5FullSlotBytes=[p['fullSlotBytes']for p in v['profiles']]),ensure_ascii=False))


if __name__ == '__main__':
    main()
