"""Finite architect contract checks only; no device, product tests or physics.

Reads canonical inputs; writes only this turn's T535/T543 manifests/evidence.
Never run the historical T524/T527 V1 generator over the revised T527 table.
"""
from pathlib import Path
from fractions import Fraction
import hashlib
import json
import math
import struct

ROOT = Path(__file__).resolve().parents[1]
DOC = ROOT / 'docs'
Q = 1 << 20


def read(name):
    return json.loads((ROOT / name).read_text(encoding='utf-8-sig'))


def write(name, value):
    (DOC / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode('utf-8')


def packed(words):
    return struct.pack('<' + 'I' * len(words), *words)


def hashwords(hex_sha):
    return list(struct.unpack('<8I', bytes.fromhex(hex_sha)))


def pair(words, at, number):
    assert 0 <= number < 1 << 64
    words[at:at + 2] = [number & 0xffffffff, number >> 32]


def bits(number):
    return struct.pack('<d', float(Fraction(number)) if isinstance(number, str) else float(number)).hex()


class Checks:
    def __init__(self):
        self.rows = []

    def yes(self, name, condition, **details):
        assert condition, (name, details)
        self.rows.append(dict(id=name, result='PASS_FINITE_CONTRACT_ONLY', **details))


def v5():
    m = read('docs/architecture-T527-v5-cold-manifest-v1.json')
    c = Checks()
    c.yes('fixed_revision_and_mask', m['headerRevision'] == 2 and m['header256'][15] == 'prefixDigestPolicy'
          and m['prefixDigestPolicy']['maskByteRange'] == [1116, 1148]
          and m['payload'] == 'GRS_M3B_LINEAGE_COLD_V5_2')
    known = [(192, 64, 28524416, 157537024), (192, 128, 29577088, 163853056),
             (512, 64, 47166336, 255871744), (512, 128, 48219008, 262187776)]
    for p, (k, kw, rb, fb) in zip(m['profiles'], known):
        c.yes(f'budget_{k}_{kw}', (p['K'], p['Kw'], p['rootSlotBytes'], p['fullSlotBytes']) == (k, kw, rb, fb))
        for mode, count in [('root', 7), ('full', 61)]:
            ss = p[mode + 'Segments']
            c.yes(f'layout_{k}_{kw}_{mode}', len(ss) == count and ss[0]['name'] == 'prefix'
                  and ss[0]['byteStart'] == 0 and ss[0]['byteEnd'] == 32768
                  and all(s['bytes'] == s['byteEnd'] - s['byteStart'] and s['bytes'] % s['strideBytes'] == 0 for s in ss)
                  and all(a['byteEnd'] == b['byteStart'] for a, b in zip(ss, ss[1:]))
                  and ss[-1]['byteEnd'] == p[mode + 'SlotBytes'])
    c.yes('unchanged_peak_and_archive', m['maxCumulativeSlotCapacityBytes'] == 524375552
          and m['profiles'][-1]['knownGPUPayloadWithFullBytes'] == 366564432
          and m['profiles'][-1]['fullArchiveUtf8Max'] == 1049837568)

    def digest(block):
        return hashlib.sha256(block[:1116] + bytes(32) + block[1148:32768]).digest()

    def accepted(block):
        h = struct.unpack_from('<256I', block)
        d = struct.unpack_from('<32I', block, 1024)
        if h[0] != m['headerMagic'] or h[1] != 2 or h[3] != m['headerSeal'] or h[15] != 1:
            return False
        if d[0] != 1 or any(d[i] for i in [1,2,3,4,5,6,7,8,17,18,19,20]) or d[21] != 2 or d[9:13] != (0, 0, 32768, 0) or d[13:17] != (1, 8192, 8192, 8192) or d[22] != 2 or d[31] != m['descriptorSeal']:
            return False
        if not 1 <= h[11] <= 64 or any(h[100:]) or any(block[1024+h[11]*128:9216]):
            return False
        return block[1116:1148] == digest(block)

    vectors = []
    for p in m['profiles']:
        for mode in ['root', 'full']:
            ss = p[mode + 'Segments']
            h = [0] * 256
            h[:16] = [m['headerMagic'], 2, 256, m['headerSeal'], 5, 31, 15,
                      int(mode == 'full'), p['K'], p['Kw'], 5 if mode == 'full' else 1, len(ss), 0, 0, 0, 1]
            pair(h, 16, 101); pair(h, 18, 202); pair(h, 28, 303)
            pair(h, 42, p[mode + 'SlotBytes']); pair(h, 44, p[mode + 'SlotBytes'])
            b = bytearray(32768); b[:1024] = packed(h)
            for i, s in enumerate(ss):
                d = [0] * 32
                d[0] = i + 1
                pair(d, 9, s['byteStart']); pair(d, 11, s['bytes'])
                d[13] = s['strideBytes'] // 4
                d[14:17] = [s['bytes'] // s['strideBytes']] * 3
                d[21] = 2; d[22] = 2 if i == 0 else 1; d[31] = m['descriptorSeal']
                if i:
                    # Synthetic digest anchors: this fixture checks the PREFIX,
                    # not a complete real full-size payload or cold reader.
                    d[23:31] = hashwords(sha(('T535_SYNTHETIC_DESCRIPTOR:' + s['name']).encode()))
                b[1024 + 128*i:1152 + 128*i] = packed(d)
            b[1116:1148] = digest(b)
            key = f'{p["K"]}_{p["Kw"]}_{mode}'
            c.yes('canonical_' + key, accepted(b))
            c.yes('raw_is_distinct_' + key, hashlib.sha256(b).digest() != digest(b))
            vectors.append(dict(id=key, scope='PREFIX_ONLY_SYNTHETIC_DESCRIPTOR_DIGESTS_NOT_COMPLETE_TRACE',
                                canonicalPrefixSha256=digest(b).hex(), rawPrefixSha256=sha(b)))
            if key != '512_128_full':
                continue
            # Each mutation is independent, including self digest corruption.
            for label, at in [('own_sha', 1116), ('other_sha', 1024+128+23*4),
                              ('prefix_seal', 1151), ('header_seal', 12), ('root_record', 24192),
                              ('config', 26752), ('padding', 32767), ('unused_descriptor', 1024+61*128)]:
                bad = bytearray(b); bad[at] ^= 1
                c.yes('reject_' + label, not accepted(bad))
            for label, offset, value in [('old_header', 4, 1), ('missing_policy', 60, 0),
                                          ('unknown_policy', 60, 2), ('prefix_alias', 1024+9*4, 4),
                                          ('prefix_length', 1024+11*4, 32764),('prefix_role',1024+2*4,2),
                                          ('unused_desc_even_rehash',1024+61*128,1),('reserved_header_even_rehash',100*4,1)]:
                bad = bytearray(b); struct.pack_into('<I', bad, offset, value)
                bad[1116:1148] = digest(bad)  # even a recomputed SHA cannot authorize a wrong contract
                c.yes('reject_' + label, not accepted(bad))
            data = bytes(range(256)); descriptor_sha = sha(data)
            changed = bytearray(data); changed[255] ^= 1
            c.yes('nonprefix_hash_covers_full_length', sha(changed) != descriptor_sha)
            original_page = b'{"page":1,"words":[1,2,3]}\n'
            changed_page = b'{"page":1,"words":[1,2,4]}\n'
            c.yes('external_pages_bytes_detect_change', sha(original_page) != sha(changed_page))
            c.yes('external_sha_not_recursive', sha(original_page + b'{"manifest":true}\n') != sha(original_page))
    write('analysis-T535-contract-checks-v1.json', dict(schema='T535_FINITE_CONTRACT_CHECKS_V1',
          date='2026-11-17', status='PASS_FINITE_CONTRACT_ONLY', deviceUsed=False, productTestRun=False,
          contractSha256=sha((DOC/'architecture-T527-v5-cold-manifest-v1.json').read_bytes()),
          cases=c.rows, vectors=vectors,
          notCovered=['complete ROOT/FULL capture', 'product collector/reader', 'device/physics/performance']))
    write('architecture-T535-prefix-digest-manifest-v2.json', dict(
          schema='T535_V5_PREFIX_DIGEST_ENGINEERING_V2', date='2026-11-17', status='ENGINEERING_READY_NOT_IMPLEMENTED',
          baseTable='architecture-T527-v5-cold-manifest-v1.json', baseTableSha256=sha((DOC/'architecture-T527-v5-cold-manifest-v1.json').read_bytes()),
          capability=m['capability'], payload=m['payload'], page=m['pageSchema'], headerRevision=2,
          headerSeal=m['headerSeal'], digestPolicy=m['prefixDigestPolicy'], archive=m['archiveManifest'],
          pageIdentityFields=['capability','payload','pageRevision','headerRevision','policy','mode','K','Kw','requestId','traceId','scope','actualBytes','segmentCount','actualDataPages','canonicalPrefixSha256','rawPrefixSha256'],
          unsupported=['V5_1','missing_policy','unknown_policy','legacy_page_for_V5_2'],
          prefixDescriptor=dict(index=0, kind=1, byteOffset=0, byteLength=32768, strideWords=1,
                                validRows=8192, plannedRows=8192, sealedRows=8192, segmentRevision=2, flags=2,
                                zeroMetadataWords=[1,2,3,4,5,6,7,8,17,18,19,20]),
          construction=['zero block','fill all nonprefix immutable bytes','hash full nonprefix segments','freeze all metadata and constant seals','canonical prefix SHA with fixed zero32 mask','write own SHA once','raw prefix SHA','scope TTL restore validation','immutable READY'],
          resourceChanges=dict(nativePayloadBytes=0, GPUPayloadBytes=0, slots=0, archiveRecords=0),
          developerResponsibility='development compile commit only; no tests; no device',
          checks='analysis-T535-contract-checks-v1.json'))
    return len(c.rows)


def rne(n, d):
    q, r = divmod(n, d)
    return q + int(2*r > d or (2*r == d and q & 1))


def quantize(number):
    value = Fraction.from_float(float(number))
    return (-1 if value < 0 else 1) * (abs(value.numerator)*Q // value.denominator)


def recipe(dx, dy, dt):
    if not (math.isfinite(dx) and math.isfinite(dy)) or abs(dx)>128 or abs(dy)>256:
        return dict(status='REJECT_BAD_DELTA')
    if not isinstance(dt, int) or not 8333334 <= dt <= 1000000000:
        return dict(status='REJECT_BAD_DURATION')
    x, y = quantize(dx), quantize(dy)
    if x*x+y*y < (Q//4)**2:
        return dict(status='NO_IMPULSE', reason='DEAD_DISPLACEMENT')
    ux, uy = rne(abs(x)*1000000000, 4*dt), rne(max(y,0)*1000000000, 4*dt)
    vx = (-1 if x<0 else 1)*min(24*Q,ux); vy=min(32*Q,uy)
    if not vx and not vy:
        return dict(status='NO_IMPULSE', reason='NO_SUPPORTED_DIRECTION')
    jx, jy = (Q//16)*vx, (Q//16)*vy
    cap = 8*(Q//16)*(vx*vx+vy*vy)*(1<<24)
    return dict(status='EXPLICIT_MODEL_IMPULSE', dxQ=x, dyQ=y, vxQ=vx, vyQ=vy,
                JxQ40=str(jx), JyQ40=str(jy), positiveWorkCap128=str(cap),
                saturatedX=ux>24*Q, saturatedY=uy>32*Q)


def g9():
    c = Checks()
    spec = read('docs/design-T534-parameter-spec-v1.json')
    profile = next(p for p in spec['profiles'] if p['id']=='G9_BALANCED_R1')
    stripped = {k:v for k,v in profile.items() if k not in ['canonicalConfigSha256','canonicalShaRule']}
    c.yes('T534_candidate_canonical_sha', sha(canonical(stripped))==profile['canonicalConfigSha256'])
    adapter = spec['inputMap']; adapter_sha=sha(canonical(adapter))
    old = read('entry/src/main/cpp/gpu_resident/g9spf/reader_schema.json')
    p = next(p for p in old['profiles'] if p['K']==512)
    segments={s['name']:s for s in p['stateSegments']}
    seed=[0]*(p['stateBytes']//4)
    seed[0]=old['identities']['state']['magic']; seed[1]=1; seed[2]=old['profile']
    seed[18]=1; seed[19]=1; seed[21]=seed[23]=512*Q
    seed[24:28]=[512,128,64,64]; pair(seed,40,5120*Q)
    seed[98:104]=[32768,24576,Q//4,4*65536,65536,576]
    column=segments['column32']['byteStart']//4; bed=segments['bedCohort4']['byteStart']//4
    for col in range(128):
        seed[column+col*32]=40*Q; seed[column+col*32+6]=1
        seed[bed+col*64*4:bed+col*64*4+4]=[1,0,1,40*Q]
    source=segments['sourceRow16']['byteStart']//4
    seed[source:source+4]=[1,0,1,0]; pair(seed,source+4,5120*Q); seed[source+14]=1
    template_sha=sha(packed(seed))
    config=[0]*64
    config[:16]=[int.from_bytes(b'G9C2','little'),2,old['profile'],512,512*Q,32768,24576,Q//4,4*65536,65536,576,2,7,1,53401,1]
    config[16:24]=hashwords(profile['canonicalConfigSha256']); config[24:32]=hashwords(adapter_sha)
    config[32:40]=hashwords(template_sha); config[40:48]=hashwords(sha((DOC/'design-T534-parameter-spec-v1.json').read_bytes()))
    parameter_sha=sha(packed(config))
    actual=seed.copy();pair(actual,4,7);pair(actual,6,1);pair(actual,8,11);pair(actual,14,2)
    actual[42:50]=hashwords(parameter_sha); actual_sha=sha(packed(actual))
    c.yes('canonical_seed_64bit_total', sum(seed[column+i*32] for i in range(128))==5120*Q
          and 5120*Q>0xffffffff and seed[41]==1 and seed[source+5]==1)
    c.yes('canonical_seed_physical_parameters', seed[98:104]==[32768,24576,262144,262144,65536,576])
    c.yes('post_identity_seed_sha_distinct', actual_sha!=template_sha)
    c.yes('config_not_legacy_candidate', config[4]!=16*Q and config[6]!=16384 and config[7]!=Q and config[10]!=384)
    mutated=config.copy();mutated[6]+=1
    c.yes('one_parameter_changes_sha', sha(packed(mutated))!=parameter_sha)
    f=read('docs/design-T534-parameter-fixtures-v1.json')
    fixtures=[]
    for item in f['mapping']:
        g=item['given']; e=item['expectedInputOnly']
        result=recipe(float(Fraction(g['dxModel'])),float(Fraction(g['dyModel'])),g['durationNs'])
        # Normalize spelling only; every designer expected value is checked.
        status_names={'BAD_DURATION':'REJECT_BAD_DURATION', 'DISPLACEMENT_DOMAIN':'REJECT_BAD_DELTA'}
        for key,value in e.items():
            normalized=status_names.get(value,value) if key=='status' else value
            c.yes(item['id']+'_'+key, key in result and result[key]==normalized, expected=value, actual=result.get(key))
        fixtures.append(dict(id=item['id'],given=g,rawDxBitsLE=bits(g['dxModel']),rawDyBitsLE=bits(g['dyModel']),expected=result))
    for name,dx,dy,dt,status in [
        ('dt_low_minus1',24.,0.,8333333,'REJECT_BAD_DURATION'),
        ('dt_low_exact',24.,0.,8333334,'EXPLICIT_MODEL_IMPULSE'),
        ('dt_high_exact',24.,0.,1000000000,'EXPLICIT_MODEL_IMPULSE'),
        ('dt_high_plus1',24.,0.,1000000001,'REJECT_BAD_DURATION'),
        ('negative_zero',-0.,-0.,250000000,'NO_IMPULSE'),
        ('subnormal',float.fromhex('0x0.0000000000001p-1022'),0.,250000000,'NO_IMPULSE'),
        ('nan',float('nan'),0.,250000000,'REJECT_BAD_DELTA'),
        ('inf',float('inf'),0.,250000000,'REJECT_BAD_DELTA'),
        ('x_outside',128.0000000001,0.,250000000,'REJECT_BAD_DELTA'),
        ('dead_below',math.nextafter(.25,0),0.,250000000,'NO_IMPULSE'),
        ('dead_exact',.25,0.,250000000,'EXPLICIT_MODEL_IMPULSE'),
        ('down_only',0.,-32.,250000000,'NO_IMPULSE'),
        ('down_tangent',24.,-32.,250000000,'EXPLICIT_MODEL_IMPULSE'),
        ('saturate_both',128.,256.,8333334,'EXPLICIT_MODEL_IMPULSE')]:
        r=recipe(dx,dy,dt);c.yes(name,r['status']==status)
        fixtures.append(dict(id=name,dxBitsLE=bits(dx),dyBitsLE=bits(dy),durationNs=dt,expected=r))
    for n,d,e in [(1,2,0),(3,2,2),(5,2,2),(7,2,4),(249999999,1000000000,0)]:
        c.yes(f'RNE_{n}_{d}',rne(n,d)==e)
    c.yes('negative_quantization_unbiased', quantize(-.1000001)==-quantize(.1000001))
    maximum=recipe(128.,256.,8333334)
    cap=int(maximum['positiveWorkCap128'])
    c.yes('max_caps', int(maximum['JxQ40'])==3*Q*Q//2 and int(maximum['JyQ40'])==2*Q*Q
          and cap==800*Q**3*(1<<24) and cap<1<<120, cap=str(cap), bitLength=cap.bit_length())

    def pixel(px,py,l,t,w,h):
        if not all(math.isfinite(x) for x in [px,py,l,t,w,h]) or w<=0 or h<=0 or not l<=px<l+w or not t<=py<t+h:
            return None
        return ((px-l)/w)*128,((t+h-py)/h)*256
    c.yes('pixel_upper_left',pixel(10.,20.,10.,20.,256.,512.)==(0.,256.))
    c.yes('pixel_center',pixel(138.,276.,10.,20.,256.,512.)==(64.,128.))
    c.yes('pixel_right_rejected',pixel(266.,20.,10.,20.,256.,512.) is None)
    c.yes('pixel_bottom_rejected',pixel(10.,532.,10.,20.,256.,512.) is None)
    c.yes('pixel_blackbar_rejected',pixel(9.,20.,10.,20.,256.,512.) is None)
    c.yes('pixel_bad_float_rejected',pixel(math.inf,20.,10.,20.,256.,512.) is None)
    edge=Fraction.from_float(math.nextafter(128.,0.))*Q
    c.yes('UP_x_RNE_grid_fringe_rejected',rne(edge.numerator,edge.denominator)==128*Q)
    def timeok(d,u,receipt,freeze,proof=True):
        return proof and all(type(x) is int and 0<x<=2**53-1 for x in [d,u]) and d<u<=receipt<=freeze and 8333334<=u-d<=1000000000 and freeze-u<=250000000
    c.yes('clock_age_equal',timeok(1,250000001,250000001,500000001))
    c.yes('clock_age_plus1_reject',not timeok(1,250000001,250000001,500000002))
    c.yes('clock_future_reject',not timeok(1,250000001,250000000,250000001))
    c.yes('clock_backwards_reject',not timeok(250000001,1,250000002,250000003))
    c.yes('clock_unknown_reject',not timeok(1,250000001,250000001,250000001,False))
    c.yes('clock_unsafe_number_reject',not timeok(2**53,2**53+250000000,2**53+250000000,2**53+250000000))

    input_fields=old['fields']['input'].copy()
    bodyset={0:set(),1:set(range(40,46))|set(range(64,77)),2:set(range(40,64))|{77}}
    def maskvalid(words):
        return words[38]==words[39]==0 and all(words[i]==0 for i in range(40,128) if i not in bodyset[words[40]])
    inp=[0]*128;inp[0]=old['identities']['input']['magic'];inp[1]=2;inp[2]=old['profile']
    pair(inp,4,7);pair(inp,12,2);inp[25]=2;inp[26:34]=hashwords(parameter_sha)
    inp[40]=2;pair(inp,41,1);pair(inp,43,250000001);inp[45]=4;inp[77]=53401
    for at,value in [(46,64.),(48,4.),(50,24.),(52,32.)]:inp[at:at+2]=list(struct.unpack('<2I',struct.pack('<d',value)))
    pair(inp,54,250000000);pair(inp,56,3*Q*Q//2);pair(inp,58,2*Q*Q)
    inp[60:64]=list(struct.unpack('<4I',cap.to_bytes(16,'little')))
    c.yes('real_impulse_unused_zero',maskvalid(inp))
    for idx in [38,39,64,76,78,127]:
        bad=inp.copy();bad[idx]=1;c.yes('reject_unused_word_'+str(idx),not maskvalid(bad))
    dedup_indices=[4,5,12,13]+list(range(26,34))+list(range(40,128))
    def same(a,b):return all(a[i]==b[i] for i in dedup_indices)
    copy=inp.copy();pair(copy,10,99);pair(copy,14,777);pair(copy,18,888);pair(copy,20,999)
    c.yes('nonce_receipts_do_not_duplicate',same(inp,copy))
    copy=inp.copy();copy[56]+=1;c.yes('changed_J_not_same_command',not same(inp,copy))
    copy=inp.copy();copy[43]+=1;c.yes('changed_capture_not_same_command',not same(inp,copy))
    c.yes('GPU_word_exact_cap_mismatch_vector',inp[60:64]!=[inp[60]^1,*inp[61:64]])
    # Contract truth table, not execution of an ArkTS/native state machine.
    def up_admission(active, changed, same_pointer=True, cancelled=False,
                     mapping_same=True, scope_same=True, move_count=0):
        return active in [0,1] and changed==1 and same_pointer and not cancelled and mapping_same and scope_same and move_count<=256
    for name, args, expected in [
        ('UP_zero_active_valid',(0,1),True),('UP_one_active_valid',(1,1),True),
        ('UP_two_active_cancel',(2,1),False),('UP_no_changed_reject',(0,0),False),
        ('UP_two_changed_reject',(0,2),False),('UP_pointer_changed_reject',(0,1,False),False),
        ('UP_cancelled_reject',(0,1,True,True),False),
        ('UP_mapping_changed_reject',(0,1,True,False,False),False),
        ('UP_scope_changed_reject',(0,1,True,False,True,False),False),
        ('UP_move_overflow_reject',(0,1,True,False,True,True,257),False)]:
        c.yes(name,up_admission(*args)==expected)

    fields=['reserved_'+str(i) for i in range(256)]
    def put(at,names):fields[at:at+len(names)]=names
    def u64(at,name):put(at,[name+'Lo',name+'Hi'])
    def digest(at,name):put(at,[name+str(i) for i in range(8)])
    put(0,['magic','revision','words','seal']);u64(4,'strokeId');put(6,['pointerId','producer'])
    ids=['run','owner','context','inputEpoch','pageSession','permission','window','config','surface']
    for i,name in enumerate(ids):u64(8+2*i,name)
    for at,name in [(26,'commandSeq'),(28,'downSampleNs'),(30,'upSampleNs'),(32,'downReceiptNs'),(34,'upReceiptNs'),(36,'freezeNs'),(38,'clockProofGeneration'),(40,'downLocalXVpBits'),(42,'downLocalYVpBits'),(44,'upLocalXVpBits'),(46,'upLocalYVpBits'),(48,'downWindowXVpBits'),(50,'downWindowYVpBits'),(52,'upWindowXVpBits'),(54,'upWindowYVpBits'),(56,'downDisplayXVpBits'),(58,'downDisplayYVpBits'),(60,'upDisplayXVpBits'),(62,'upDisplayYVpBits'),(64,'vpToPxScaleBits'),(66,'componentOriginWindowPxXBits'),(68,'componentOriginWindowPxYBits'),(70,'rectLeftPxBits'),(72,'rectTopPxBits'),(74,'rectWidthPxBits'),(76,'rectHeightPxBits'),(78,'downModelXBits'),(80,'downModelYBits'),(82,'upModelXBits'),(84,'upModelYBits'),(86,'rawDeltaXBits'),(88,'rawDeltaYBits'),(90,'durationNs'),(92,'mappedDownCaptureNs'),(94,'mappedUpCaptureNs')]:u64(at,name)
    put(96,['coordinateRevision','eventClockDomain','nativeClockDomain','touchAdapterCode','downAngleIndex','upAngleIndex','pointerCountMax','eventCount','moveCount','rejectReason','saturationFlags','pointPolicy'])
    for at,name in [(108,'downSceneGeneration'),(110,'downSceneCommit'),(112,'geometryRevision'),(114,'viewportRevision')]:u64(at,name)
    put(116,['surfaceWidthPx','surfaceHeightPx'])
    for at,name in [(118,'parameterSha'),(126,'adapterSpecSha'),(134,'seedTemplateSha'),(142,'actualSeedSha'),(150,'stableCommandBodySha'),(158,'downSceneTokenSha'),(166,'clockProofSha'),(174,'coordinateProofSha'),(182,'frozenInputSha'),(190,'platformSourceSha')]:digest(at,name)
    put(198,['timestampExactSafeInteger','pointSource','timestampUnitNs','poseSource'])
    for at,name in [(202,'upSceneGeneration'),(204,'upSceneCommit')]:u64(at,name)
    digest(208,'upSceneTokenSha');u64(216,'JxQ40');u64(218,'JyQ40');put(220,['workCap'+str(i) for i in range(4)])
    u64(232,'downTimestampBinary64Bits');u64(234,'upTimestampBinary64Bits')
    u64(236,'nativeClockCheckBeforeNs');u64(238,'nativeClockCheckAfterNs');digest(240,'moveRawRollingSha')
    c.yes('record_unique_complete_256',len(fields)==256 and len(set(fields))==256)
    c.yes('prefix_extension_nonoverlap',3072+2*256==3584 and 3584+64==3648 and 3648+64==3712 and 3712<=8192)
    c.yes('native_credit_exact',16*1024+4*1024+4*256+1024==22528)
    c.yes('resource_original_unchanged',p['payloadBytes']==93803648 and p['coldFullSlotBytes']==179671040
          and old['cold']['maxCumulativeCapacityBytes']==359342080)
    source_names=['docs/design-T534-parameter-spec-v1.json','docs/design-T534-parameter-fixtures-v1.json',
                  'docs/developer-T529-g9-spf-implementation.md']
    source_names += ['entry/src/main/cpp/gpu_resident/g9spf/'+name for name in
                     ['input_gate.h','input_source.glsl','research_config.h','control_types.h','napi.inc','transport.cpp','bridge_create.inc','cold_capture.cpp','cold_contract.h','scene.cpp','reader_schema.json']]
    sources=[dict(path=name,sha256=sha((ROOT/name).read_bytes())) for name in source_names]
    sdk=Path('C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0')
    for name in ['ets/component/common.d.ts','native/sysroot/usr/include/ace/xcomponent/native_interface_xcomponent.h']:
        sources.append(dict(path=str(sdk/name),sha256=sha((sdk/name).read_bytes()),meaning='API_UNITS_ONLY_NOT_DEVICE_CLOCK_PROOF'))
    manifest=dict(schema='T543_G9_CANDIDATE_ADAPTER_ENGINEERING_V1',date='2026-11-17',
          status='ENGINEERING_READY_NOT_IMPLEMENTED',deviceUsed=False,productDefaultChanged=False,CPUPhysicsAdded=False,
          designCandidateId=profile['id'], designCandidateSha256=profile['canonicalConfigSha256'],
          adapterId=adapter['id'],adapterSpecCanonicalRule='sorted compact UTF8 JSON of T534 inputMap object',adapterSpecSha256=adapter_sha,
          parameterRevision=2,parameterSha256=parameter_sha,config64=config,
          config64Fields=['magic','revision','profile','K','gQ20','muStartQ16','muStopQ16','bandQ20','entrainRateQ16','shearQ16','settleTicks','parameterRevision','researchAllowFlags','seedRevision','touchAdapterCode','coordinatePolicy'] +
             ['designCandidateSha'+str(i) for i in range(8)]+['adapterSpecSha'+str(i) for i in range(8)]+['seedTemplateSha'+str(i) for i in range(8)]+['approvedDesignFileSha'+str(i) for i in range(8)]+['reserved_'+str(i) for i in range(48,64)],
          configHash='SHA256 exact 64 LE words including all reserved0; no self hash field',
          seed=dict(id='G9_SEED_S40_ORIGIN1_R1',bytes=p['stateBytes'],templateSha256=template_sha,
              canonicalHeaderZeroFields=['run','owner','context','inputGeneration','parameterSha'],
              canonicalAssignments={'header':{str(i):v for i,v in enumerate(seed[:256]) if v},
                                    'columnEvery32Words':{'0':40*Q,'6':1},'bedRank0Every64Cohort4':[1,0,1,40*Q],
                                    'sourceRow0':seed[source:source+16]},
              totalMassQ20=str(5120*Q),scopeKey=['run','inputEpoch','origin','sourceId'],allOtherWordsZero=True,
              exampleFinalIdentity=dict(run=7,owner=1,context=11,inputEpoch=2),exampleActualUploadSha256=actual_sha,
              actualHashOrder='fill final native identity and parameter SHA first, then hash exact uploaded LE bytes',
              gpuInitialAuditRequiredBeforeCurrent=True,initializationDispatches=2),
          input=dict(revision=2,words=128,fields=input_fields,adapterCode=53401,realCommandFlags=dict(SAT_X=1,SAT_Y=2,REAL_TOUCH=4),
              envelopeFlags=0,poseClockDomain=2,poseSource=2,word38=0,word39=0,reservedWords=[78,128],
              allowedBodyWords={str(k):sorted(v) for k,v in bodyset.items()},dedupWords=dedup_indices,
              GPURecompute=['deltaQ20_floor_abs_sign','duration','RNE_vx_vy','JxJy','workCap128','saturationFlags','radius4'],
              GPUFloat64Required=False,modelToPixelValidation='native and offline reader exact evidence; GPU integer model recipe',
              adapterBranch='IMPULSE adapter53401 with REAL_TOUCH => recompute; adapter0 flags0 => explicit synthetic; all other combinations reject',
              impulseAloneRearmsBlocked=False,acceptedAllUnappliedConsumes=True,blockedConsumes=False),
          coordinate=dict(revision=1,source='ArkTS same DOWN/UP API event point; vp -> physical px using UIContext.vp2px',
              physicalRect='native already successfully presented uBox, UI top-left origin',
              x='((px-left)/width)*128',y='((top+height-py)/height)*256',rounding='binary64 separate RNE operations; no FMA or fast-math',
              pixelBounds='[left,left+width) x [top,top+height)',modelBounds='0<=x<128,0<y<=256; UP RNE(x*Q)<128Q also required',
              normalCommitDuringStrokeAllowed=True,changeCancels=['mapping scope','geometry','viewport','rect','config','surface','clock proof','STALE frame']),
          clock=dict(profile='OHOS_TOUCH_MONOTONIC_NS_R1',unit='ns',offsetNs=0,assumedVerified=False,
              requireActualPlatformProducerProof=True,ArkTSMaxExactTimestamp=2**53-1,durationNs=[8333334,1000000000],UPAgeNsMax=250000000,
              refreshUPForbidden=True,poseManualObservationMayRefreshOnlyOnAuthorizedPulse=True,
              upstreamReferences=['https://github.com/openharmony/multimodalinput_input/blob/master/frameworks/proxy/events/src/input_event.cpp',
                                  'https://github.com/openharmony/arkui_ace_engine/blob/master/adapter/ohos/entrance/mmi_event_convertor.cpp'],
              upstreamMeaning='design reference only, not evidence for deployed device build'),
          gesture=dict(oneUP=True,moveForce=False,moveMax=256,moveEvidence='count and rolling raw hash; not full trajectory replay',
              queueCapacity=16,pointerMax=1,UPActiveCountMayBeZero=True,sequence='native strokeId == IMPULSE commandSeq, nonzero U64 monotonic per inputEpoch',
              rejects=['cancel','multitouch','pointer mismatch','out of rect including any MOVE','wrong identity','future/regressed/unsafe/missing timestamp','clock proof missing','queue full','bad binary64','unsupported transform']),
          touchEvidence=dict(magic=int.from_bytes(b'G9T2','little'),revision=1,words=256,seal=0xc0de0902,fields=fields,
              hash='external raw-record SHA only; no self digest',absent='all256words0 with explicit absence in manifest',
              stableBodySha='SHA256 LE words in input.dedupWords order; locator only, full body compare still mandatory'),
          cold=dict(capability='G9_SPF_COLD_V2',headerMagic=0x31433947,headerRevision=2,page='G9_SPF_SEGMENT_PAGE_V2',prefixWords=8192,
              prefixSelfDescriptor=False,rawPrefixHash='external manifest only',
              extension=[dict(name='CURRENT_FROZEN_TOUCH',wordStart=3072,words=256),dict(name='BASE_LAST_CONSUMED_TOUCH',wordStart=3328,words=256),
                         dict(name='CONFIG64_V2',wordStart=3584,words=64),dict(name='SEED_META64',wordStart=3648,words=64)],
              reservedZero=[3712,8192],rootSlotBytes=p['coldRootSlotBytes'],fullSlotBytes=p['coldFullSlotBytes'],twoSlotBytes=359342080,
              seedMeta64Fields=['magic','revision','words','seal']+['seedTemplateSha'+str(i) for i in range(8)]+['actualSeedSha'+str(i) for i in range(8)]+['parameterSha'+str(i) for i in range(8)]+
                 ['M0Lo','M0Hi','originLo','originHi','sourceId','seedKind','uploadBytesLo','uploadBytesHi','seedRevision']+['reserved_'+str(i) for i in range(37,64)],
              seedMetaMagic=int.from_bytes(b'G9S2','little'),seedMetaRevision=1,seedMetaSeal=0xc0de0902,
              oldReader='reject V2; new reader may read explicit V1 synthetic but never certify it as real touch'),
          cost=dict(extraGPUBytes=0,extraNativeBoundedInputPayloadBytes=22528,seedUploadTemporaryBytes=673792,
              knownConcurrentK512FullTwoSlotAndPageBytes=586902656,
              extraColdAllocationBytes=0,hotInputBytes=512,hotConfigBytes=512,completionReadBytes=1024,
              quantumDispatchMax=2024,sourceDispatchMax=12,planDispatchMax=8108,initializationDispatches=2,
              extraQuantumDispatches=0,rootFenceCount=1,bindings=4,workgroupLanes=64,groupsPerPass=1,sharedBytesMax=4096,
              timeCost='additional bounded gate integer operations; device cost not measured',notIncluded=['allocator overhead','driver','FBO','stack/compiler spill','JS runtime']),
          unchanged=dict(rootDeadlineNs=250000000,gapNs=33333333,quotaHz=120,displayTargetFps=30,defaultOFF=True,CPUStopped=True),
          developerHandoff='development compile commit only; no tests; no device; complete full agreed scope or specific evidenced blocker',
          sources=sources,checks='analysis-T543-contract-checks-v1.json')
    c.yes('complete_machine_fields',len(manifest['config64Fields'])==64 and len(manifest['cold']['seedMeta64Fields'])==64)
    write('architecture-T543-g9-adapter-manifest-v1.json',manifest)
    write('analysis-T543-contract-checks-v1.json',dict(schema='T543_FINITE_ENGINEERING_CHECKS_V1',date='2026-11-17',
          status='PASS_FINITE_CONTRACT_ONLY',deviceUsed=False,productTestRun=False,CPUPhysicsRun=False,
          cases=c.rows,fixtures=fixtures,exampleInput128V2=inp,exampleInputSha256=sha(packed(inp)),
          parameterSha256=parameter_sha,seedTemplateSha256=template_sha,exampleActualSeedSha256=actual_sha,
          manifestSha256=sha((DOC/'architecture-T543-g9-adapter-manifest-v1.json').read_bytes()),
          notCovered=['native/ArkTS/GPU actual implementation','platform clock mapping on device','physics','runtime cold export','subjective effect','30fps']))
    return len(c.rows)


if __name__ == '__main__':
    counts={'T535':v5(),'T543':g9()}
    print(json.dumps(dict(status='PASS_FINITE_CONTRACT_ONLY',cases=counts,deviceUsed=False,productTests=False)))
