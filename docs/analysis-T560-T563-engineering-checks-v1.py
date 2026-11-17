"""Finite engineering/layout audit; no GL, device, CPU physics or product tests.
Only writes this task's new manifests and analysis artifacts. Run from repo root.
"""
import base64
import hashlib
import json
from collections import Counter
from fractions import Fraction
from itertools import product
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOC = ROOT / 'docs'


def read(name):
    return json.loads((DOC / name).read_text(encoding='utf-8-sig'))


def emit(name, obj):
    (DOC / name).write_text(json.dumps(obj, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def receipt(path):
    data = (ROOT / path).read_bytes()
    return dict(path=path, bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def segments(items, start=0):
    out = []
    for name, count, stride in items:
        size = count * stride
        out.append(dict(name=name, byteStart=start, byteEnd=start+size,
                        bytes=size, count=count, strideBytes=stride))
        start += size
    return out


def fields(names):
    return [dict(word=i, name=n) for i, n in enumerate(names)]


assert (ROOT / 'entry/src/main/cpp/gpu_resident/g9spf/packet.vert').exists()
r1 = read('architecture-T524-g9-spf-manifest-v1.json')
dm = read('architecture-T554-g9-dm-manifest-v2.json')
dm_dispatch = read('architecture-T554-g9-dm-dispatch-v2.json')
inputs = [receipt('docs/'+n) for n in [
    'algorithm-G11-surface-lateral-rendering-v1.md', 'design-T557-T558-fixtures-v1.json',
    'architecture-T524-g9-spf-manifest-v1.json', 'architecture-T554-g9-dm-manifest-v2.json',
    'architecture-T554-g9-dm-dispatch-v2.json']]

stage = {
    'schema': 'T560_STAGE_LAB_ENGINEERING_V1', 'status': 'ENGINEERING_READY_NOT_IMPLEMENTED_NOT_VALIDATED',
    'notUsingDevice': True, 'productionDefaultChanged': False,
    'compute': {'ES': [3, 1], 'ssboBindings': 4, 'computeSSBOBlocks': 4,
                'combinedSSBOBlocks': 4, 'vertexSSBOBlocks': 0, 'fragmentSSBOBlocks': 0,
                'sharedBytes': 4096, 'invocations': 64, 'sizeX': 64, 'countX': 1,
                'maxBlockBytes': 'max(profile.stateBytes, profile.workBytes, profile.arenaBytes)',
                'additionalGates': ['actual link', 'reflection', 'prewarm', 'GL error status']},
    'scene': {'vertexSSBOBlocks': 0, 'fragmentSSBOBlocks': 1, 'combinedSSBOBlocks': 1,
              'packetAttributes': 3, 'G11PacketAttributes': 6, 'glyphAttributes': 2,
              'actualGates': ['integer attribute types/offsets/divisors', 'shader reflection',
                              'FBO complete', 'same CURRENT lease', 'scene fence retirement']},
    'present': ['actual XComponent window/surface/context generation', 'token', 'Swap result'],
    'packetABI': {'id': 'G9_PACKET_ATTR_U32_V2', 'source': 'same CURRENT state GL buffer',
                  'packetByteStart': 280576, 'strideBytes': 256, 'verticesPerInstance': 6,
                  'attributes': [dict(location=i, type='UNSIGNED_INT', components=1,
                                      offsetBytes=i*4, divisor=1) for i in range(3)],
                  'API': 'glVertexAttribIPointer', 'extraGPUBytes': 0, 'extraDispatch': 0,
                  'extraHotReadbackBytes': 0, 'barriers': ['SHADER_STORAGE', 'VERTEX_ATTRIB_ARRAY']},
    'lab': {'queueCapacity': 128, 'fault': '129th pending enqueue rejected while worker blocked',
            'race': 'release barrier; Drain enqueue can execute before first worker pop',
            'fix': 'test-only DrainForTest atomically waits for space and enqueues fence under same mutex',
            'deadlineMs': 5000, 'productionEnqueueAndDrainUnchanged': True,
            'lifecycle': ['RUNNING', 'FINISHED'],
            'outcomes': ['COMPLETED_FIXTURE', 'CANCELLED', 'CASE_FAILED', 'INCOMPLETE'],
            'oldFAILReclassified': False, 'oldExportV1IsPollAlias': True},
    'files': {'schema': 'RESOURCE_LAB_FILES_V2', 'maxFiles': 32, 'maxFileBytes': 2097152,
              'maxTotalFileBytes': 67108864, 'chunkBytes': 4096, 'base64BytesMax': 5464,
              'replyUtf8BytesMax': 8192, 'indexEntriesPerPage': 8, 'ttlNs': '60000000000',
              'readerMax': 1, 'renewTTL': False, 'nativePayloadBytes': 20480,
              'extraFixedHostState': 'fd/mutex/identities/hash state separately declared by implementation',
              'catalogEntryBytes': 256, 'producer': 'explicit successful SaveBytes allowlist only',
              'paths': 'openat pinned job root fd + O_NOFOLLOW; no caller path or recursive cache export',
              'endpoints': ['resourceLabFilesBeginV2(jobId)', 'resourceLabFilesIndexV2(exportId,page)',
                            'resourceLabFilePageV2(exportId,fileId,offset)', 'resourceLabFilesEndV2(exportId)'],
              'acceptance': 'actual sequential raw bytes, chunk SHA, file SHA after fsync/close',
              'newStartWhilePinned': 'LAB_EXPORT_BUSY', 'fileMutation': 'FILE_CHANGED/INCOMPLETE'},
    'evidenceLimits': ['vertexSSBO0/lanes1024/fragment16 are T541 report observations; full probe stdout not saved',
                       'CACHE_QUEUE_FULL second throw location is source-supported race, no runtime stack',
                       'FILE_SHA memory-input hash alone is not independently retrieved disk bytes'],
    'handoff': {'developer': 'implementation/compile/commit only, no tests/device',
                'tester': 'new version qualification, scene and lab/file evidence independently',
                'manager': 'register existing FAIL and follow-up work; no new acceptance gate'}
}
emit('architecture-T560-stage-lab-manifest-v1.json', stage)

raw_paths = ['test/reports/t541/g9d-execution-k192.jsonl', 'test/reports/t541/glver-out.txt',
             'test/reports/t541/glext-out.txt', 'test/reports/t491/t519v2/lab-cache-fault.json']
raw = [receipt(p) for p in raw_paths]
queue = json.loads((ROOT / raw_paths[-1]).read_text(encoding='utf-8-sig'))
# Preserve the raw whole artifact. Its exact JSON shape is not assumed by the source audit.
serialized = json.dumps(queue, ensure_ascii=False)
assert 'CACHE_QUEUE_FULL' in serialized and 'FAIL' in serialized
file_pairs = [(x['result'], queue['records'][i+1]['result'])
              for i, x in enumerate(queue['records'][:-1]) if x['stage']=='FILE'
              and queue['records'][i+1]['stage']=='FILE_SHA256']
assert len(file_pairs) == 4 and queue['records'][-1]['stage']=='FAIL'
stage_matrix = []
for vertex in [0, 1]:
    for compute in [0, 4]:
        for fragment in [0, 1]:
            stage_matrix.append(dict(vertex=vertex, compute=compute, fragment=fragment,
                                     computeEligible=compute >= 4,
                                     sceneEligible=fragment >= 1,
                                     note='other numeric and actual gates assumed satisfied; independent layers'))
for p in r1['profiles']:
    end = 280576 + (p['K']-1)*256 + 8 + 4
    assert end <= p['stateBytes']
assert len(base64.b64encode(bytes(4096))) == 5464
assert 4096+8192+32*256 == 20480
assert 32*2097152 == 67108864
emit('analysis-T560-evidence-v1.json', {
    'schema': 'T560_SOURCE_AND_FINITE_AUDIT_V1', 'rawReceipts': raw,
    'sourceReceipts': [receipt(p) for p in [
        'entry/src/main/cpp/gpu_resident/g9spf/packet.vert',
        'entry/src/main/cpp/gpu_resident/g9spf/scene.vert',
        'entry/src/main/cpp/gpu_resident/g9spf/scene.frag',
        'entry/src/main/cpp/gpu_resident/resource_lab/lab.h',
        'entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h']],
    'qualificationMatrix': stage_matrix,
    'queueInterleavings': [
        {'afterReleaseBeforeDrain': 'no worker pop', 'queueSize': 128, 'legacyDrain': 'CACHE_QUEUE_FULL'},
        {'afterReleaseBeforeDrain': 'worker pops >=1', 'queueSizeMax': 127, 'legacyDrain': 'can enqueue'}],
    'actualFileAndSHARecordPairs': file_pairs,
    'result': 'FINITE_ENGINEERING_CHECKS_PASS_NOT_PRODUCT_PASS',
    'deviceExecuted': False, 'GPUCapabilityMeasuredThisTask': False,
    'fullPredicateProbeStdoutAvailable': False,
    'stackAtSecondQueueThrowAvailable': False,
    'labOutcome': 'HISTORICAL_FAIL_PRESERVED'})

marker_names = ['owner', 'column', 'cohortRank', 'packetSlot', 'packetGeneration', 'packetId',
                'originLo', 'originHi', 'sourceId', 'rank', 'massSnapshot', 'parkedXiNumerator',
                'parkedXiDenominator', 'oldXQ24', 'oldYQ23', 'newXQ24', 'newYQ23',
                'phaseStartTickLo', 'phaseStartTickHi', 'phaseSpanTicks', 'visualIdLo', 'visualIdHi',
                'flags', 'sampleGeneration', 'appearanceSampleKey', 'cohortMapEpochLo',
                'cohortMapEpochHi', 'lastPlanOrdinal', 'lastMaterialPhase', 'phaseOldKind', 'reserved0', 'seal']
config_names = ['magic', 'revision', 'compositeProfile', 'parentProfile', 'K', 'V', 'markerWords',
                'glyphWords', 'R', 'blendTicks', 'depthQ20', 'diameterQ20', 'speedNumerator',
                'speedDenominator', 'minimumSpeedQ20', 'samplingPolicy', 'descriptorRevision',
                'markerRevision', 'glyphRevision', 'readerRevision']
config_names += ['parentParameterSHA'+str(i) for i in range(8)]
config_names += ['designSHA'+str(i) for i in range(8)]
config_names += ['recipeSHA'+str(i) for i in range(8)]
config_names += ['reserved'+str(i) for i in range(20)]
assert len(config_names) == 64 and len(marker_names) == 32
control = segments([
    ('bankHeaders', 3, 512), ('packetStyle', 512, 16), ('columnIndex', 128, 8),
    ('markerIndex', 8192, 4), ('batchDigest', 258, 64), ('planHeader', 1, 512),
    ('visualConfig', 1, 256), ('scopeCounters', 2, 256), ('reservedZero', 4224, 1)])
assert control[-1]['byteEnd'] == 65536
recipe = {
    'schema':'G11_VISUAL_RECIPE_V1', 'markerWords':32, 'glyphWords':8,
    'sampling':{'M':'V/128','xi':'(2*j+1)/(2*M)','eta':'(2*min(j,M-1-j)+1)/M',
                'appearanceKey':'(M<<16)|min(j,M-1-j)',
                'inventoryOrder':['S bottom-to-top','F physical map order','P physical slot order at actual x column'],
                'refill':'accepted event/column order then INACTIVE slot order; j=slot%M; no periodic refill'},
    'blendTicks':[0,256], 'depthQ20':131072,'diameterQ20':32768,
    'speed':[1,1],'minimumSpeedQ20':0, 'alphaMax':'1/2','linearContrastMax':'2/25',
    'physicalSource':'exact sealed final per-origin accepted shares and conversions only',
    'algorithmRevision':1,'markerRevision':1,'planRevision':1,'sceneRevision':1}
emit('architecture-T563-visual-recipe-v1.json',recipe)
recipe_receipt = receipt('docs/architecture-T563-visual-recipe-v1.json')
resources = []
dispatch_stats = []


def visual_dispatch(parent, source, quantum, natural):
    def extra(op, original=None, phase=None, role='ROOT'):
        row = dict(op=op, role=role, Jslot=0, micro=-1, bindingCount=4, parentIndex=None)
        if original:
            row.update({k: original[k] for k in ['role', 'Jslot', 'micro'] if k in original})
        if phase:
            row['batchPhase'] = phase
        row['uniformGuard'] = 'parent root fault/selected scope/micro guard; every lane participates in barriers'
        return row
    outq = []
    begin = {'TOUCH_ATOMIC': 'TOUCH', natural: 'NATURAL', 'FLOW_ORIGIN_RESERVE': 'FLOW',
             'FRICTION_REST_DEPOSIT': 'DEPOSIT', 'ORDERED_CAPTURE': 'CAPTURE'}
    end = {'TOUCH_ATOMIC': 'TOUCH', natural: 'NATURAL', 'FLOW_PAIR_APPLY': 'FLOW',
           'FRICTION_REST_DEPOSIT': 'DEPOSIT', 'ORDERED_CAPTURE': 'CAPTURE'}
    for row in quantum:
        visual = row.get('role') in ['AUTHORITATIVE', 'REPLAY']
        if visual and row['op'] in begin:
            outq.append(extra('VIS_PLAN_BEGIN', row, begin[row['op']]))
        if row['op'] == 'ROOT_SEAL':
            outq.extend([extra('VIS_ROOT_AUDIT'), extra('VIS_GLYPH')])
        outq.append(dict(row, parentIndex=row['index']))
        if row['op'] == 'BASE_FREEZE':
            outq.append(extra('VIS_ROOT_COPY'))
        if visual and row['op'] in end:
            outq.append(extra('VIS_PLAN_APPLY', row, end[row['op']]))
    outs = []
    for row in source:
        op = row['op']
        if op in ['SOURCE_PRIVATE_BIRTH', 'SOURCE_AUDIT_REPLAY']:
            role = 'AUTHORITATIVE' if op == 'SOURCE_PRIVATE_BIRTH' else 'REPLAY'
            b = extra('VIS_PLAN_BEGIN', phase='SOURCE', role=role)
            b['oldMapBank'] = 'BASE' if role == 'REPLAY' else 'C'
            outs.append(b)
        if op == 'SOURCE_SEAL':
            outs.extend([extra('VIS_ROOT_AUDIT'), extra('VIS_GLYPH')])
        outs.append(dict(row, parentIndex=row['index']))
        if op == 'SOURCE_VALIDATE_INPUT':
            outs.append(extra('VIS_ROOT_COPY'))
        if op in ['SOURCE_PRIVATE_BIRTH', 'SOURCE_AUDIT_REPLAY']:
            outs.append(extra('VIS_PLAN_APPLY', phase='SOURCE', role=role))
    for table in [outq, outs]:
        for i, row in enumerate(table):
            row['index'] = i
        assert all(x['bindingCount'] == 4 for x in table)
    # Removing visual hooks reproduces every parent pass and its ordering exactly.
    for before, after in [(source, outs), (quantum, outq)]:
        recovered = [{k: v for k, v in x.items() if k not in ['index', 'parentIndex']}
                     for x in after if x['parentIndex'] is not None]
        expected = [{k: v for k, v in x.items() if k != 'index'} for x in before]
        assert recovered == expected
    assert len(outs) == 19 and len(outq)-len(quantum) == 519
    assert sum(x['op'] == 'VIS_PLAN_BEGIN' for x in outq) == 258
    assert sum(x['op'] == 'VIS_PLAN_APPLY' for x in outq) == 258
    assert outq[-1]['op'] == 'ROOT_SEAL' and outs[-1]['op'] == 'SOURCE_SEAL'
    # A FLOW begin must precede reserve, not only the later pair apply.
    for i, row in enumerate(outq):
        if row['op'] == 'VIS_PLAN_BEGIN' and row['batchPhase'] == 'FLOW':
            assert outq[i+1]['op'] == 'FLOW_ORIGIN_RESERVE'
    emit('architecture-T563-g11-'+parent.lower()+'-dispatch-v1.json',
         {'schema': 'T563_G11_FULL_DISPATCH_V1', 'parent': parent, 'source': outs, 'quantum': outq,
          'opcodes': 'new independent generated namespace; never reuse legacy requiredPasses as root kind',
          'dispatchGeometry': {'groups': [1, 1, 1], 'localSize': [64, 1, 1]},
          'coldVisualInitializationDispatches': 1})
    dispatch_stats.append(dict(parent=parent, SOURCE=len(outs), QUANTUM=len(outq),
                               G1F4=len(outs)+4*len(outq), H=258, newQuantumPasses=519))


visual_dispatch('R1', r1['sourcePasses'], r1['quantumPasses'], 'ACTIVATE_APPLY')
visual_dispatch('DM', dm_dispatch['source'], dm_dispatch['quantum'], 'DM_APPLY')
for parent, source in [('R1', r1['profiles']), ('DM', dm['resources'])]:
    for p in source:
        for V in [2048, 8192]:
            vis = segments([('markerCURRENT', V, 128), ('markerACTUAL', V, 128),
                            ('markerREPLAY', V, 128), ('glyph', V, 32),
                            ('materialPlan', 65536, 32), ('control', 1, 65536),
                            ('fourMaps', 4*128*64, 16)], p['workBytes'])
            extra = vis[-1]['byteEnd'] - p['workBytes']
            assert extra == (3538944 if V == 2048 else 6094848)
            resident = p['payloadBytes'] if parent == 'R1' else p['residentPayloadBytes']
            fullgpu = p['fullTrialTraceGPUAdditionalBytes'] if parent == 'R1' else p['fullGPUAdditionalBytes']
            full = p['coldFullSlotBytes'] if parent == 'R1' else p['FULL']['bytes']
            coldroot = p['coldRootSlotBytes'] if parent == 'R1' else p['ROOT']['bytes']
            witness = 2097152 + 524288 + 2*128*V + 4096
            known = resident + fullgpu + 2*full + 22528 + 16384 + 3*extra
            resources.append(dict(parent=parent, K=p['K'], V=V, workSegments=vis,
                visualBytes=extra, totalWorkBytes=vis[-1]['byteEnd'], stateBytes=p['stateBytes'],
                arenaBytes=p['arenaBytes'], residentPayloadBytes=resident+extra,
                fullGPUAdditionalBytes=fullgpu, coldROOTSlotBytes=coldroot+extra,
                coldFULLSlotBytes=full+extra, twoFULLSlotsBytes=2*(full+extra),
                largestGLObjectBytes=max(p['stateBytes'], p['arenaBytes'], vis[-1]['byteEnd']),
                knownPayloadWithGPUFullTwoColdInputAndPage=known,
                optionalSinglePhaseWitnessBytes=witness,
                knownPayloadWithWitnessGPUAndNative=known+2*witness,
                visualColdSegments=7, maxCombinedFULLSegments=17,
                visualColdDataPages=sum((s['bytes']+4095)//4096 for s in vis),
                combinedFULLPageUpperBound=1+(full+extra+4095)//4096+32,
                combinedFULLArchiveUtf8UpperBound=(2+(full+extra+4095)//4096+32)*16384,
                inputSnapshotAndPageBytes=22528+16384,
                excluded=['driver/JIT/host objects', 'FBO>=8*w*h', 'alignment', 'JS/runtime', 'files'],
                simultaneousOldProfileAllowed=False))

manifest = {
    'schema': 'T563_G11_VIS_ENGINEERING_V1', 'date': '2026-11-17',
    'status': 'ENGINEERING_READY_NOT_IMPLEMENTED_NOT_VALIDATED', 'notUsingDevice': True,
    'defaultOFF': True, 'CPUPhysicsAdded': False, 'goal10': 'PAUSED', 'inputs': inputs,
    'profiles': [dict(profile='G11_VIS_G9_R1_V1', code=int.from_bytes(b'G11R','little'), parent=r1['profile']),
                 dict(profile='G11_VIS_G9_DM_V2_V1', code=int.from_bytes(b'G11D','little'), parent=dm['profile'])],
    'identity': {'parentPhysicsStateInputParameters': 'unchanged exact parent bytes and identities',
                 'new': ['compositeProfile', 'pipelineSHA', 'layoutSHA', 'shaderSHA', 'visualConfigSHA',
                         'visualSeedRecipeSHA', 'readerSchema', 'visualGeneration'],
                 'SHA': 'SHA256 of exact little-endian u32 bytes; digest stored as 8 LE u32 words',
                 'actualVisualSeedSHA': 'only after GPU initialization and raw cold retrieval',
                 'permit': 'parent scope + composite/config/pipeline + physical/visual base generations'},
    'visualSeedRecipe':recipe_receipt,
    'configHashBindings':{'recipeSHA':'exact bytes in visualSeedRecipe receipt',
                         'designSHA':'exact algorithm-G11 md input receipt',
                         'parentParameterSHA':'exact already-selected parent parameter block; no new approval',
                         'configSHA':'exact 256 bytes little-endian VisualConfig64; hash external, reserved zero'},
    'visualConfig64': fields(config_names),
    'configConstants': {'magic': int.from_bytes(b'G11C','little'), 'revision': 1, 'blendTicks': [0,256],
                        'depthQ20': 131072, 'diameterQ20': 32768, 'speedNumerator': 1,
                        'speedDenominator': 1, 'minimumSpeedQ20': 0, 'samplingPolicy': 1,
                        'markerWords': 32, 'glyphWords': 8, 'R': 65536, 'reserved': 0},
    'marker32': fields(marker_names), 'markerSeal': 0xC0DE1101,
    'owner': {'INACTIVE': 0, 'S': 1, 'F': 2, 'P': 3},
    'flags': {'PHASE_ACTIVE': 1, 'PHASE_REBASED': 2, 'CLIPPED_ANCHOR': 4,
              'VISUAL_REQUANTIZATION': 8, 'P_GHOST_REPRESENTATIVE': 16},
    'glyph8': fields(['oldXQ24','oldYQ23','newXQ24','newYQ23','kindFlags','blendQ16',
                      'oldPacketMassOr0','appearanceKey']),
    'glyphKindFlags': {'oldOwnerBits': [0,1], 'newOwnerBits': [2,3], 'oldVisible': 16,
                       'newVisible': 32, 'oldPacketGhost': 64, 'knownMask': 127},
    'appearance': {'recipe': 'mirror-paired stratified column slots; no periodic reseeding',
                   'M': 'V/128', 'xi': '(2*j+1)/(2*M)',
                   'eta': '(2*min(j,M-1-j)+1)/M', 'mirror': '(c,j)->(127-c,M-1-j)',
                   'key': '(M<<16)|min(j,M-1-j), stable across flow/conversion; new sample uses same candidate',
                   'verticalEta': '(2*(appearanceSampleKey & 65535)+1)/(appearanceSampleKey>>16)',
                   'sourceOfOrigin': 'actual cumulative inventory, never color hash',
                   'poolFull': 'VIS_DENSITY_LIMITED; physical transaction unchanged'},
    'controlSegments': control,
    'plan': {'R': 65536, 'words': 8, 'maxValid': '49152+K <=49664',
             'fields': fields(['type','ownerRef','originRef','oldRef','newRef','amount','rangeStartOrEventRef','seal']),
             'seal': 0xC0DE1102, 'referenceTagBits': [28,31], 'referenceIndexBits': [0,27],
             'referenceTags': {'OLD_BED':1,'NEW_BED':2,'OLD_FLOW':3,'NEW_FLOW':4,'PACKET':5,'EVENT64':6,'COLUMN':7},
             'NONE': 4294967295,
             'types': {'MAP_OLD_BED':1,'MAP_NEW_BED':2,'MAP_OLD_FLOW':3,'MAP_NEW_FLOW':4,
                       'DONOR_LEFT':5,'DONOR_RIGHT':6,'S_CUT':7,'DEPOSIT':8,
                       'PACKET_LAUNCH':9,'PACKET_CAPTURE':10,'SOURCE_BIRTH':11},
             'partitions': [dict(start=0,end=32768,domain='4 maps x128x64 MAP_REF'),
                            dict(start=32768,end=49152,domain='2 conversion/donor planes x128x64'),
                            dict(start=49152,end='49152+K',domain='one actual action per packet slot'),
                            dict(start='49152+K',end=65536,domain='zero')],
             'phaseTypes': {'TOUCH':['S_CUT','PACKET_LAUNCH'], 'NATURAL':['S_CUT'],
                            'FLOW':['DONOR_LEFT','DONOR_RIGHT'], 'DEPOSIT':['DEPOSIT'],
                            'CAPTURE':['PACKET_CAPTURE'], 'SOURCE':['SOURCE_BIRTH']},
             'mapTypes': 'four MAP_* slots valid only for active nonempty cohort rows; other rows zero',
             'count': 'number of valid sealed records; never a dense prefix length',
             'capacityProofAssumption': 'one material phase per batch, no packet merge, <=K actual packet actions',
             'lifetime': 'old maps before writer; exact final Hamilton shares before scratch reuse; new maps after writer; consume before reuse',
             'typedReferences': 'validate type/tag/bounds/header generation/full origin words and key; no truncated origin'},
    'planUnions': {
        'MAP_REF':{'ownerRef':'COLUMN(c)','originRef':'nonempty cohort side',
                   'oldRef':'old physical cohort row or NONE','newRef':'actual surviving physical cohort row or NONE',
                   'amount':'referenced side mass','rangeStartOrEventRef':0},
        'S_CUT':{'ownerRef':'COLUMN(c)','originRef':'OLD_BED', 'oldRef':'OLD_BED',
                 'newRef':'NEW_FLOW or NONE if all launched', 'amount':'actual cut',
                 'rangeStartOrEventRef':'old cohort mass minus cut'},
        'DONOR':{'ownerRef':'COLUMN(donor)','originRef':'OLD_FLOW','oldRef':'OLD_FLOW',
                 'newRef':'neighbor NEW_FLOW','amount':'exact final Hamilton share',
                 'rangeStartOrEventRef':'left 0; right old origin mass minus right share'},
        'DEPOSIT':{'ownerRef':'COLUMN(c)','originRef':'OLD_FLOW','oldRef':'OLD_FLOW',
                   'newRef':'actual NEW_BED row','amount':'actual deposition',
                   'rangeStartOrEventRef':'preexisting mass in merged old top, otherwise 0'},
        'PACKET':{'ownerRef':'COLUMN(c)','originRef':'EVENT64','oldRef':'actual owner side or NONE',
                  'newRef':'actual owner side or NONE','amount':'event q',
                  'rangeStartOrEventRef':'EVENT64 index; launch prefix starts 0'},
        'unused':'all 32 bytes zero; otherwise fields not applicable use NONE for refs and zero for scalars',
        'touchOrder':'recruit projection on old F + actual cuts, then launch exact origin prefix, then final F map',
        'cohortContinuity':'writer row mapping, never merge distinct bed rows solely because origin matches'},
    'controlABI': {
        'bankHeader128': '0 magic 1 rev 2 composite 3 parent 4 K 5 V; 6..7 generation; 8..9 acceptedTicks; 10..11 nextVisualId; 12..13 baseCommit; 14..15 rootNonce; 16..17 owner; 18..19 context; 20..21 run; 22..23 surface; 24..31 visualConfigSHA; 32..39 pipelineSHA; 40 flags; 41 activeCount; 42 fault; 43..47 counters; 48..126 zero; 127 seal',
        'bankHeaderMagic': int.from_bytes(b'G11B','little'), 'bankHeaderSeal': 0xC0DE1103,
        'planHeader128': '0 magic 1 rev 2 composite 3 parent; 4..5 generation; 6..7 rootNonce; 8..9 baseCommit; 10 role; 11 micro; 12 phase; 13 ordinal; 14 validCount; 15 zero; 16..23 visualConfigSHA; 24..31 pipelineSHA; 32..33 run; 34..35 owner; 36..37 context; 38..39 surface; 40 K; 41 V; 42 oldBank; 43 newBank; 44 materialStatus; 45..126 zero; 127 seal',
        'planHeaderMagic': int.from_bytes(b'G11P','little'), 'planHeaderSeal': 0xC0DE1104,
        'materialPhases': {'TOUCH':1,'NATURAL':2,'FLOW':3,'DEPOSIT':4,'CAPTURE':5,'SOURCE':6},
        'batchDigest16': '0 ordinal 1 role 2 micro 3 phase 4 validCount 5 physicalReason 6 markerCount 7 flags; 8..15 SHA256 locator only',
        'scopeCounter64': '0 consumedBatches; 1..2 nextVisualId; 3 rebaseCount; 4 clippedCount; 5 requantizationCount; 6 densityLimitedCount; 7 noVisibleCount; 8..63 zero',
        'pstyle4': ['physicsId','generation','opacityQ16','representativeMarkerIndex'],
        'columnIndex2': ['start','count'], 'markerIndex': 'stable marker slot order grouped by column',
        'roleNormalization': 'ACTUAL/REPLAY per-role plan header and locators differ; compare canonical marker/header state excluding role-specific locators; reserved always zero'},
    'root': {'banks': ['BASE(previous CURRENT)', 'ACTUAL', 'REPLAY'],
             'hotBindings': 4, 'scopeH': '1+4J', 'rootHMax': 258, 'sourceHMax': 2,
             'parentPassesPreserved': True, 'initializationDispatches': 1,
             'auditBits': {'PLAN_MAPPING':18, 'VIS_REPLAY_GLYPH':19},
             'requiredMasks': {'R1_SOURCE':0xcffff,'R1_QUANTUM':0xcffff,
                               'DM_SOURCE':0xcffff,'DM_QUANTUM':0xfffff},
             'completion': 'new composite revision/profile and counts; physical fields parent semantics; last ROOT_SEAL after VIS readiness',
             'publication': 'single native mutex publishes physical+visual bank/generation; no GL/wait under lock',
             'glyphLease': 'single work glyph buffer; block next writer until scene fence retires',
             'FAULT': 'no partial publication; explicit FAULT or labelled STALE complete old shader frame',
             'unchanged': {'deadlineNs':250000000,'gapNs':33333333,'quotaHz':120,'displayFps':30}},
    'dispatch': dispatch_stats, 'resources': resources,
    'scene': {'pipeline': 'XComponent+shader only', 'vertexSSBOBlocks':0,
              'fragmentSSBOBlocks':1,'attributesRequired':6,
              'glyph': 'loc0/1 uvec4 IPointer offsets0/16 stride32 divisor1 from same WORK',
              'packet': 'loc0/1/2 parent mass/x/y, loc3 uvec4 Pstyle, loc4/5 uint id/generation; separate VAO',
              'barriers':['SHADER_STORAGE','VERTEX_ATTRIB_ARRAY'],
              'drawBounds': 'at most V glyph and K physical packet instances; 6*(V+K) vertices',
              'mask': 'current S/F/B and foreground same CURRENT token and view; no invented thickness',
              'cost': 'two-anchor bounding quad can overdraw large area; measure real fragment cost',
              'material': 'premultiplied linear alpha<=0.5 contrast +/-0.08 research constants not approved thresholds'},
    'cold': {'capability':'G11_VIS_COLD_V1','page':'G11_VIS_SEGMENT_PAGE_V1',
             'slots':2,'readerMax':1,'builderMax':1,'ttlNs':'60000000000',
             'pageWords':1024,'pageUtf8Bytes':16384,
             'payload': 'parent selected ROOT/FULL plus all visual segments with exact profile/offset dictionary',
             'wholeRows': 'marker32/glyph8/descriptor8/cohort4 never split',
             'credit': 'global actual native capacity; pinned old route prevents third slot',
             'witness': 'one prearmed (root,micro,phase,role), complete plan/maps and old/new marker, header4096; extra GPU+native each capacity listed',
             'witnessCopies': 'old marker before update, new marker and plan/maps after; no extra dispatch, count actual segmented copies',
             'historyLimit': 'last plan only, batch digest is locator not full intermediate material proof'},
    'unmeasured': ['shader compile/link', 'actual capabilities/attributes', 'GPU runtime/250ms feasibility',
                   'scene/device image correctness', 'perceptual thresholds', 'fragment overdraw', 'actual file export'],
    'handoff': {'developer': 'all implementation/compile/commit scope or concrete blocker; no tests/device',
                'tester':'independent V01-V14, exact source plans and replay, no new CPU physics stepper',
                'effector':'same accepted interval, no visible layer/phase rebase/overdraw, no fabricated animation'}
}
emit('architecture-T563-g11-manifest-v1.json', manifest)

# Small integer exhaustive specification algebra; this is NOT a physical stepper.
rank_cases = 0
for m in range(1, 25):
    for a in range(m+1):
        for b in range(m-a+1):
            groups = [[], [], []]
            for rank in range(m):
                side, newrank = ((0,rank) if rank<a else
                                 (2,rank-(m-b)) if rank>=m-b else (1,rank-a))
                groups[side].append(newrank)
                mirror = m-1-rank
                mside, mrank = ((0,mirror) if mirror<b else
                                (2,mirror-(m-a)) if mirror>=m-a else (1,mirror-b))
                length = [a,m-a-b,b][side]
                assert mside == 2-side and mrank == length-1-newrank
                rank_cases += 1
            assert groups == [list(range(a)),list(range(m-a-b)),list(range(b))]
            if a == b == 0:
                assert groups[1] == list(range(m))
quantization_cases = 0
for mass in range(1,129):
    for num in range(1,128,2):
        xi = Fraction(num,128)
        r = (num*mass)//128
        assert abs(Fraction(2*r+1,2*mass)-xi) <= Fraction(1,2*mass)
        quantization_cases += 1
global_cases = 0
choices = [(m,a,b) for m in range(5) for a in range(m+1) for b in range(m-a+1)]
for old in product([x for x in choices if x[1]==0], choices,
                   [x for x in choices if x[2]==0]):
    newmass = [old[c][0]-old[c][1]-old[c][2]
               +(old[c-1][2] if c>0 else 0)+(old[c+1][1] if c<2 else 0)
               for c in range(3)]
    output = []
    for c,(mass,left,right) in enumerate(old):
        for rank in range(mass):
            if rank<left:
                d=c-1
                newrank=(old[d-1][2] if d>0 else 0)+old[d][0]-old[d][1]-old[d][2]+rank
            elif rank>=mass-right:
                d,newrank=c+1,rank-(mass-right)
            else:
                d,newrank=c,(old[c-1][2] if c>0 else 0)+rank-left
            assert 0<=d<3 and 0<=newrank<newmass[d]
            output.append((d,newrank))
    assert sorted(output)==[(c,r) for c,m in enumerate(newmass) for r in range(m)]
    global_cases += 1
uniform_cases=0
for mass in range(1,25):
    for right in range(mass+1):
        for rank in range(mass):
            # Local periodic uniform stream; unwrapped destination preserves crossing.
            d,newrank=((1,rank-(mass-right)) if rank>=mass-right else (0,rank+right))
            assert Fraction(d)+Fraction(2*newrank+1,2*mass)-Fraction(2*rank+1,2*mass)==Fraction(right,mass)
            uniform_cases+=1
for V in [2048,8192]:
    M = V//128
    for j in range(M):
        jj = M-1-j
        assert Fraction(2*j+1,2*M)+Fraction(2*jj+1,2*M) == 1
        assert min(j,M-1-j) == min(jj,M-1-jj)
maxrow = next(x for x in resources if x['parent']=='DM' and x['K']==512 and x['V']==8192)
assert maxrow['twoFULLSlotsBytes'] == 388308992
assert maxrow['knownPayloadWithGPUFullTwoColdInputAndPage'] == 632474752
assert maxrow['knownPayloadWithWitnessGPUAndNative'] == 641920128
assert maxrow['optionalSinglePhaseWitnessBytes'] == 4722688
emit('analysis-T563-engineering-checks-v1.json', {
    'schema':'T563_FINITE_ENGINEERING_AUDIT_V1', 'result':'FINITE_ENGINEERING_CHECKS_PASS_NOT_PRODUCT_PASS',
    'inputs':inputs, 'resourceCombinations':len(resources), 'rankPartitionMirrorCases':rank_cases,
    'threeColumnGlobalBijectionCases':global_cases,'uniformFluxDisplacementCases':uniform_cases,
    'requantizationBoundCases':quantization_cases, 'configWords':len(config_names),
    'markerWords':len(marker_names), 'controlBytes':control[-1]['byteEnd'],
    'maxPlanRecords':49664, 'R':65536, 'dispatch':dispatch_stats,
    'checks':['parent pass order preserved exactly after stripping visual hooks',
              'FLOW_BEGIN precedes origin reserve scratch reuse', 'no hooks in discovery/rejected prefix',
              'root/source seal last', '4 bindings every pass', 'all 8 resource combinations contiguous',
              'q0 exact rank identity', 'rank interval partition bijection and mirror',
              'three-column boundary/interior exact rank bijection', 'uniform displacement q/m including crossing',
              'quantization <=1/(2m)', 'seed pair mirror', 'full cold/witness cost arithmetic'],
    'limitations':['finite algebra is not independent GPU validation',
                   'no Hamilton physics recomputation or CPU physical stepper',
                   'no device/GL tests or performance claim',
                   'mirror proof assumes corresponding actual left/right shares; does not fix parent left-first tie'],
    'deviceExecuted':False, 'productPASS':False})
print(json.dumps({'T560':'source/layout audit complete', 'T563':'finite engineering audit complete',
                  'rankCases':rank_cases,'quantizationCases':quantization_cases,
                  'resources':len(resources),'dispatch':dispatch_stats},ensure_ascii=False))
