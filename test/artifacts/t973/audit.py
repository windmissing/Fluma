"""Recompute T968 first-difference evidence from immutable logs and archives."""

from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
RUNS = ROOT / 'test/reports/t968/runs'
ASSETS = ROOT / '.codex/local-test/t963-g13/g13_assets'
sys.path[:0] = [str(ROOT / 'test/t964'), str(ROOT / 'pc/vulkan')]
from build_roles import DECLARATION, MODES, split_group, substitute
from projection import read_state, reference, rms, rhs_words, gradient, divergence, evaluate


def sha(data):
    return hashlib.sha256(data).hexdigest()


def rows(path):
    return [json.loads(line) for line in path.read_bytes().splitlines() if line.startswith(b'{')]


def raw_words(data, start, count):
    return struct.unpack_from(f'<{count}I', data, start * 4)


def sand_arm(path):
    events = rows(path)
    roots, timing = [], None
    for event in events:
        if event['stage'] == 'ROOT':
            roots.append({k: event[k] for k in ('kind', 'run', 'nonce', 'fence', 'fenceGeneration',
                                                'vkResult', 'dispatches', 'tailRead', 'accepted')})
        if event['stage'] == 'ROOT_GPU_TIMESTAMPS' and event['kind'] == 'QUANTUM':
            gpu, timing = event, {}
            root = next(x for x in roots if x['kind'] == 'QUANTUM')
            assert all(root[k] == gpu[k] for k in ('run', 'nonce', 'fence', 'fenceGeneration'))
            assert gpu['fenceGeneration'] == gpu['queryResetSerial'] and gpu['acceptedRoot'] is None
            assert all(p['available'] and p['vkResult'] == 0 for p in gpu['points'])
            points = gpu['points']
            tick_mask = (1 << gpu['validBits']) - 1
            ms = lambda a, b: ((int(b['tick'])-int(a['tick'])) & tick_mask) * gpu['periodNs']/1e6
            coarse = {span['name']: ms(points[i], points[i+1]) for i, span in enumerate(gpu['spans'][:14])}
            assert all(abs(v - span['ns']/1e6) < 0.001 for v, span in zip(coarse.values(), gpu['spans']))
            fine = []
            for index in range(15, len(points), 2):
                lo, hi = points[index:index+2]
                assert lo['edge'] == 'INPUT_READY' and hi['edge'] == 'DISPATCH_DONE'
                assert lo['slot'] == hi['slot'] and lo['opcode'] == hi['opcode']
                fine.append(dict(slot=lo['slot'], opcode=lo['opcode'], role=lo['role'],
                                 jSlot=lo['jSlot'], micro=lo['micro'], ms=ms(lo,hi)))
            timing = dict(points=len(points), coarseMs=coarse, totalMs=ms(points[0],points[14]), fine=fine,
                          inventorySingleScan=gpu['inventorySingleScan'])
    host = [{k: event[k] for k in ('kind','stop','grantNs','submitExitNs','stopNs') if k in event}
            for event in events if event['stage'] == 'QUANTUM_TIMELINE']
    for event in events:
        if event['stage'] == 'ROOT_TIMELINE' and event['rootKind'] == 'SOURCE':
            host.append({k: event[k] for k in ('rootKind','firstStop','grantNs','submitExitNs','stopNs','gateResult')})
    return dict(sha256=sha(path.read_bytes()), roots=roots, timing=timing, host=host)


def f02(stem):
    path = RUNS / 'goal13' / (stem + '.quantum')
    data = path.read_bytes()
    trace = rows(path.with_suffix('.jsonl'))
    final = next(r for r in trace if 't833QuantumArchive' in r)
    archive = final['t833QuantumArchive']
    assert archive['schema'] == 'PC_G13_T833_RAW_V2' and archive['sha256'] == sha(data)
    assert archive['slotCount'] == 14 and archive['slotBytes'] == 168
    assert archive['fieldOffset'] == 1024 + 14*168 and archive['fileBytes'] == len(data)
    assert archive['completionSHA'] == sha(data[:1024]) and list(raw_words(data, 0, 256)) == archive['completion']
    assert len(set(archive['descriptorBuffers'])) == 7
    offset, bounds, segments = archive['fieldOffset'], {}, {}
    for name, size, digest in zip(archive['segments'],archive['segmentsBytes'],archive['segmentSHA'],strict=True):
        contents = data[offset:offset+size]
        assert len(contents) == size and sha(contents) == digest
        bounds[name], segments[name] = [offset, size, digest], contents
        offset += size
    assert offset == len(data)
    assert struct.pack('<8I',*archive['completion'][96:104]).hex() == sha(segments['CANDIDATE'])
    layout = archive['layoutWords']
    assert layout[:6] == [64,64,4096,8320,49536,149408]
    work = segments['WORK']
    mac, rhs, residual = (4*layout[16+i] for i in (6,12,13))
    mac_bytes, rhs_bytes = work[mac:mac+8320*4], work[rhs:rhs+4096*8]
    initial, initial_p = read_state(segments['BASE'])
    velocities, pressure = read_state(segments['CANDIDATE'])
    replay_v, replay_p = read_state(segments['REPLAY'])
    actual_rhs = struct.unpack('<4096q',rhs_bytes)
    actual_residual = struct.unpack_from('<4096q', work, residual)
    assert mac_bytes == segments['BASE'][1024:1024+8320*4]
    assert actual_rhs == tuple(rhs_words(initial,1)) and velocities == replay_v and pressure == replay_p
    div = divergence(velocities)
    p_values = [x/2**32 for x in pressure]
    calc_residual = [x/2**32+y for x,y in zip(actual_rhs,divergence(gradient(p_values)),strict=True)]
    expected_origin = final['t833QuantumPlan']['nonce']
    settled = next(r for r in trace if r.get('stage') == 'T955_ORIGINAL_RETIRE')
    cold = next(r for r in trace if r.get('stage') == 'T877_COLD_COPY' and 'copyCount' in r)
    assert expected_origin == settled['nonce'] == cold['nonce']
    assert int(expected_origin) == (archive['completion'][9] << 32) | archive['completion'][8]
    assert settled['fence'] == final['t833QuantumFenceObject'] and settled['vkResult'] == 0
    assert archive['nativeError'] == 9 and archive['acceptedRoot'] is None and not archive['privateProjectComplete']
    scope_traces = {scope: raw_words(segments['CTRL'],start,16) for scope,start in [('actual',2112),('replay',10304)]}
    cycles = {}
    for scope, start in [('actual',2048),('replay',10240)]:
        cycles[scope] = [raw_words(segments['CTRL'],start+1024+128+16*i,16) for i in range(16)]
        assert all(w[0] == i and w[3] == 0 and w[15] == 0xc0de1301 for i,w in enumerate(cycles[scope]))
        projection = raw_words(segments['CTRL'],start+1024+512+8*(39-16),8)
        assert projection[:4] == (39,33,0,8320) and projection[7] == 0xc0de1301
    active_cycles = {scope: [w[0] for w in records if w[1] == 33 and w[14] == 68]
                     for scope, records in cycles.items()}
    assert all(w == list(range(8 if 'nonzero' in stem else 1)) for w in active_cycles.values())
    diagnostic = None
    if 'nonzero' in stem:
        checker_receipt = dict(mode='PRIVATE_PROJECT_DIAGNOSTIC', acceptedRoot=None,
                               qpcSubmit=int(final['t833QuantumSubmitQpc']), qpcFence=int(final['t833QuantumFenceQpc']),
                               queueSubmit=final['t833QuantumSubmit'], fence=settled['vkResult'],
                               pressureSolveObserved=True, analyticWrite=False)
        normalized_rhs = b''.join(x.to_bytes(16,'little',signed=True) for x in actual_rhs)
        diagnostic = evaluate(segments['BASE'],segments['CANDIDATE'],normalized_rhs,checker_receipt,1)
    return dict(archiveSha256=sha(data), logSha256=sha(path.with_suffix('.jsonl').read_bytes()),
                bounds=bounds, archiveSlotCount=archive['slotCount'], logicalPlanSlotCount=archive['completion'][39],
                formerWorkOffsets=[mac,rhs,residual], rawMacSHA=sha(mac_bytes), rawRhsSHA=sha(rhs_bytes),
                inputFacesNonzero=sum(x!=0 for x in initial), inputPressureNonzero=sum(x!=0 for x in initial_p),
                outputFacesNonzero=sum(x!=0 for x in velocities), outputPressureNonzero=sum(x!=0 for x in pressure),
                rhsAllMatch=True, rhsRms=rms([x/2**32 for x in actual_rhs]), residualRms=rms(calc_residual),
                gpuResidualRms=rms([x/2**32 for x in actual_residual]), divRms=rms(div), divMax=max(map(abs,div)),
                velocityRms=rms(velocities), gauge=sum(pressure), pressureRms=rms([x-y for x,y in zip(p_values,reference())]),
                scopeTraces=scope_traces, pressureCycles=cycles, activeCycles=active_cycles, t964Numerics=diagnostic,
                originalRetire=settled, coldCopy=cold,
                outcome=archive['completion'][5:8], nativeError=archive['nativeError'], acceptedRoot=None)


def qualify():
    root = RUNS / 'qual-g13'
    result = {}
    for folder in sorted(p for p in root.iterdir() if p.is_dir()):
        cases = {}
        for path in sorted(folder.glob('*.jsonl')):
            calls = [r for r in rows(path) if r.get('call') in ('vkCreateShaderModule','vkCreateComputePipelines')]
            cases[path.name] = dict(sha256=sha(path.read_bytes()), calls=calls)
        result[folder.name] = cases
    return result


def projection_timing(stem):
    rows_ = rows(RUNS/'goal13'/f'{stem}.jsonl')
    timing = next(r for r in rows_ if r.get('stage') == 'T877_GPU_TIMESTAMPS' and r.get('state') == 'LATE_RETIRED_DIAGNOSTIC')
    retired = next(r for r in rows_ if r.get('stage') == 'T955_ORIGINAL_RETIRE')
    assert all(timing[k] == retired[k] for k in ('nonce','fence'))
    assert timing['fenceGeneration'] == retired['generation'] and timing['businessRejected'] and timing['acceptedRoot'] is None
    points = {p['point']:int(p['tick']) for p in timing['points'] if p['available'] and p['vkResult'] == 0}
    tick_mask = (1 << timing['validBits']) - 1
    spans = [('total','ROOT_BEGIN','BUSINESS_COPIES_DONE'), ('actual','ACTUAL_BEGIN','ACTUAL_END'),
             ('replay','REPLAY_BEGIN','REPLAY_END'), ('sha819','SEAL_PREPARE_END','SEAL_SHA_END'),
             ('actualCycle0','ACTUAL_J1_MICRO0_PRESSURE_CYCLE0_BEGIN','ACTUAL_J1_MICRO0_PRESSURE_CYCLE0_END'),
             ('replayCycle0','REPLAY_J1_MICRO0_PRESSURE_CYCLE0_BEGIN','REPLAY_J1_MICRO0_PRESSURE_CYCLE0_END')]
    modules = next(r['candidateModules'] for r in rows_ if 't833QuantumArchive' in r)
    return dict(nonce=timing['nonce'],fence=timing['fence'],generation=retired['generation'],
                spansMs={name:((points[end]-points[begin]) & tick_mask)*timing['periodNs']/1e6 for name,begin,end in spans},
                sealModules=[m for m in modules if m['role'].startswith('OP11_')])


def trials():
    emit, opcodes = split_group((ASSETS / 'role_OP_48.comp').read_bytes())
    compiler = ROOT / '.codex/local-test/g13-glslang/bin/glslang.exe'
    result = {}
    with tempfile.TemporaryDirectory(prefix='t973-g13-') as temp:
        for selection in [(55,58),(56,59),(55,),(58,),(56,),(59,)]:
            assert set(selection) <= set(opcodes)
            name = 'OP' + '_'.join(map(str, selection))
            source, _ = emit(selection)
            src = source.decode()
            for original, replacement in (
                    ('#version 310 es', DECLARATION),
                    ('uniform uint uOp,uScope,uJ,uMicro,uBlock,uIndex;',
                     'layout(push_constant) uniform Dispatch {uint uOp,uScope,uJ,uMicro,uBlock,uIndex;};'),
                    ('uniform uint uL[80];','layout(std140,set=0,binding=4) uniform LayoutWords {uint uL[80];};'),
                    ('highp float ulp=','precise highp float ulp='),
                    ('highp float retained=','precise highp float retained='),
                    ('highp float produced=','precise highp float produced=')):
                src = substitute(src,original,replacement,name)
            comp = Path(temp)/(name+'.comp')
            mod = comp.with_suffix('.spv')
            encoded = src.encode()
            comp.write_bytes(encoded)
            result_text = subprocess.run([str(compiler), '-V', '--target-env','vulkan1.1','--spirv-val','--spirv-dis',
                                          '-o',str(mod),str(comp)],check=True,capture_output=True,text=True)
            asm = result_text.stdout
            assert not re.search(r'validation.*(fail|unavailable|not available)|\berror:', asm+result_text.stderr, re.I)
            entry, = re.findall(r'OpEntryPoint GLCompute (%\S+) "main"', asm)
            assert all(f'OpExecutionMode {entry} {mode} 32' in asm for mode in MODES)
            assert set(re.findall(r'OpDecorate %\S+ Binding (\d+)',asm)) == set('01234')
            assert f'OpExecutionMode {entry} LocalSize 64 1 1' in asm and 'RelaxedPrecision' not in asm
            pair = {(55,58): 'PC_OP55_58_INSTALL', (56,59): 'PC_OP56_59_AUDIT'}.get(selection)
            original_match = None if pair is None else (encoded == (ASSETS/(pair+'.comp')).read_bytes() and mod.read_bytes() == (ASSETS/(pair+'.spv')).read_bytes())
            assert original_match is not False
            result[name] = dict(opcodes=[*selection,66], sourceBytes=len(encoded), moduleBytes=mod.stat().st_size,
                                sourceSHA=sha(encoded), moduleSHA=sha(mod.read_bytes()), reproducesT963=original_match,
                                functionCalls=asm.count('OpFunctionCall'), noContraction=asm.count('NoContraction'))
    return result


def demo():
    result = {}
    for path in sorted(RUNS.glob('demo*/demo.jsonl')):
        data = path.read_bytes()
        counts = Counter(r.get('stage') for r in rows(path))
        result[path.parent.name] = dict(sha256=sha(data), rows=len(data.splitlines()), window=counts['WINDOW_AVAILABLE'],
                                       ready=counts['M1_POINTER_READY'], synthetic=counts['M1_DEMO_SYNTHETIC_INPUT'],
                                       pointer=counts['WIN32_POINTER'])
    return result


if __name__ == '__main__':
    output = dict(goal9={p.stem:sand_arm(p) for p in sorted((RUNS/'goal9').glob('*.jsonl'))},
                  g15={p.stem:sand_arm(p) for p in sorted((RUNS/'g15').glob('*.jsonl'))},
                  goal13={stem:f02(stem) for stem in ('f02-nonzero-j1','f02-zero')},
                  projectionTiming={stem:projection_timing(stem) for stem in ('f02-nonzero-j1','f02-zero','seal-A','seal-B')},
                  qualification=qualify(), roleTrials=trials(), demo=demo())
    print(json.dumps(output,indent=2,ensure_ascii=True))
