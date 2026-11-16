"""Read existing source/artifacts only. No device/GL run, cache writes, or test repair."""
from pathlib import Path
import hashlib
import json
import re
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / 'docs'


def fp(p):
    p = Path(p)
    b = p.read_bytes()
    return dict(path=p.relative_to(ROOT).as_posix(), bytes=len(b), sha256=hashlib.sha256(b).hexdigest())


def read(p):
    return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def write(name, data):
    (DOCS/name).write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


def t518():
    local = ROOT/'.codex/local-test/t496'
    raw = {n: (local/n).read_text(encoding='utf-8-sig', errors='replace') for n in (
        't496_log.ndjson', 't496-stages.ndjson', 't496-script-manifest.json')}
    logs = [json.loads(l) for l in raw['t496_log.ndjson'].splitlines() if l.strip()]
    stages = [json.loads(l) for l in raw['t496-stages.ndjson'].splitlines() if l.strip()]
    runs = {}
    for row in logs:
        r = runs.setdefault(row['runId'], dict(rows=0, phases={}, last=None))
        r['rows'] += 1
        phase = row.get('phase', 'ASSERT')
        r['phases'][phase] = r['phases'].get(phase, 0)+1
        r['last'] = row
    latest = logs[-1]['runId']
    lr = [r for r in logs if r['runId'] == latest]
    assert {r['scriptNs'] for r in lr} == {'0', '1000000'}
    assert len(logs) == 29 and len(runs) == 8 and len(stages) == 10
    layouts = {}
    for stem in ('driver', 'impl'):
        data = (local/f'layout-{stem}.txt').read_bytes().decode('gb18030', errors='replace')
        start = data.index('class grs::m1::FullM1Transport')
        canonical = data[start:].strip()
        size = int(re.search(r'size\((\d+)\)', canonical).group(1))
        assert size == 182640 and re.search(r'128\s*\| SourceCoreTransport source', canonical)
        layouts[stem] = dict(size=size, canonical=canonical,
                             original=fp(local/f'layout-{stem}.txt'),
                             emptyGateOutput=fp(local/f'sizeof-{stem}.txt'))
    assert layouts['driver']['canonical'] == layouts['impl']['canonical']
    assert all(v['emptyGateOutput']['bytes'] == 0 for v in layouts.values())
    host = (ROOT/'test/t496/external_source_diff.cpp').read_text(encoding='utf-8-sig')
    assert 'programcache::Store().Configure' not in host
    assert 'RecoverySessionLocked(' not in host
    seed = ROOT/'.codex/local-test/t485-app-m1-exact/cache/gpu-program-cache/v1'
    entries = []
    for receipt in sorted(seed.glob('*.jsonl')):
        rows = [json.loads(x) for x in receipt.read_text(encoding='utf-8-sig').splitlines() if x.strip()]
        blob = receipt.with_suffix('.bin')
        b = blob.read_bytes()
        assert b[:8] == b'T475B001'
        fmt, n = struct.unpack_from('<II', b, 8)
        key, payloadsha = b[16:80].decode('ascii'), b[80:144].decode('ascii')
        assert n+144 == len(b) and key == receipt.stem
        assert payloadsha == hashlib.sha256(b[144:]).hexdigest()
        assert any(r['route'] == 'BINARY_LOAD' for r in rows)
        entries.append(dict(role=rows[0]['role'], key=key, format=fmt, payloadBytes=n,
                            payloadSha256=payloadsha, binary=fp(blob), receipt=fp(receipt),
                            environmentSHA=rows[-1]['environmentSHA'],
                            lastReceipt=rows[-1], currentGLLoad='NOT_RUN'))
    assert {r['role'] for r in entries} == {'SOURCE', 'INTERMEDIATE_QUANTUM', 'FINAL_QUANTUM',
                                          'SCENE_FULL', 'SCENE_PARTICLES', 'SCENE_WATER'}
    files = [ROOT/'test/t496/build.cmd', ROOT/'test/t496/external_source_diff.cpp',
             ROOT/'test/t496/source_gate.md', local/'t496.exe',
             ROOT/'test/t492/build_app.cjs', ROOT/'test/t485/build_app_check.cjs',
             ROOT/'test/t475/build_app.cjs', ROOT/'docs/test-report-T504-resource-lifecycle-v1.md',
             ROOT/'docs/architecture-T494-external-source-reservation-triage-v1.md',
             ROOT/'docs/architecture-T518-host-create-triage-v1.md', Path(__file__)]
    base = ROOT/'.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident'
    files += [base/p for p in ('app_bridge.cpp', 'm1/full_transport.cpp', 'm1/source_transport.cpp',
                              'm1/quantum_candidate.cpp', 'm1/scene.cpp',
                              'program_cache/program_factory.h', 'program_cache/graphics_program_factory.h',
                              'program_cache/artifact_store.h', 'execution_diagnostics.h')]
    files += [local/n for n in raw]
    write('analysis-T518-host-create-evidence-v1.json', dict(
        schema='T518_ARCHITECT_STATIC_TRIAGE_V1', date='2026-11-16',
        status='TRIAGE_COMPLETE_PRECISE_TESTER_EVIDENCE_REQUIRED',
        device='NOT_RUN', GLExecuted=False, logs=logs, stages=stages,
        rawOriginals=raw, runCount=len(runs), runs=runs, latestRun=latest,
        firstUnmatchedStage=dict(runId=latest, stage='D1_OFF', event='ENTER'),
        concreteGLUnmatchedCall='UNKNOWN_NOT_RECORDED',
        reported52Minutes='TESTER_OBSERVATION_NOT_DERIVABLE_FROM_CURRENT_JSONL',
        layouts=layouts, historicalExeSourceBinding='NO_INPUT_HASH_RECEIPT',
        warmSeedEntries=entries, warmCurrentEnvironment='UNKNOWN_NOT_PROBED',
        confirmedStaticIssues=['empty layout comparison', 'unconfigured seed store',
                               'missing recovery session/pulse handoff', 'recursive scheduler mutex in drain',
                               'quit can leave hook awaiting permit', 'missing true milestone and D2 control',
                               'empty ledgers can pass parity; unconditional EXIT_OK and exit0'],
        inputFiles=[fp(p) for p in files]))
    print('T518: 29 log rows / 8 runs; layout 182640 both; 6 seed envelopes valid; no GL execution')


def t521():
    receipt = ROOT/'test/reports/t257/T482-final.json'
    r = read(receipt)
    packages = {}
    for key in ('signed', 'input'):
        p = Path(r[key]['path'])
        f = fp(p)
        assert f['sha256'] == r[key]['sha256']
        with zipfile.ZipFile(p) as z:
            name = 'libs/arm64-v8a/libgrsm1.so'
            b = z.read(name)
            markers = {s: s.encode() in b for s in ('T462_M2_CREATE_DIAGNOSTIC_V1', 'm2CreateDiagnostics',
                                                   'm2CreateLog', 'b7d0a027128811db429440dec0cb94c77c8c26167fc7b507c611ffb3158606a1')}
            assert all(markers.values())
            packages[key] = dict(package=f, elfEntry=name, elfBytes=len(b),
                                  elfSha256=hashlib.sha256(b).hexdigest(), elfMarkers=markers)
    assert packages['signed']['elfSha256'] == packages['input']['elfSha256']
    folder = ROOT/'test/reports/t491/t461'
    layouts = {}
    for p in sorted(folder.glob('t461_l*.json')):
        pairs = set()
        def walk(x):
            if isinstance(x, dict):
                if isinstance(x.get('text'), str) and x.get('text') and x.get('bounds'):
                    pairs.add((x['text'], str(x['bounds'])))
                for v in x.values():
                    if isinstance(v, (dict, list)):
                        walk(v)
            elif isinstance(x, list):
                for v in x:
                    walk(v)
        walk(read(p))
        layouts[p.name] = dict(file=fp(p), textBounds=[dict(text=t, bounds=b) for t, b in sorted(pairs)])
    assert layouts['t461_l4.json']['textBounds'] == layouts['t461_l5.json']['textBounds']
    assert (folder/'create-diag.json').stat().st_size == (folder/'m2log.txt').stat().st_size == 0
    base = ROOT/'.codex/local-test/t482-final/entry/src/main'
    files = [receipt, ROOT/'test/reports/t482/delivery.json', ROOT/'docs/test-report-T461-m2-v3-device-v1.md',
             ROOT/'docs/developer-T482-coherent-value-boundaries.md',
             ROOT/'docs/architecture-T521-m2-zero-ledger-triage-v1.md', Path(__file__),
             base/'cpp/gpu_resident/app_bridge.cpp', base/'cpp/gpu_resident/m2/create_diagnostics.h',
             base/'ets/pages/GpuSandWaterExperience.ets', base/'ets/physics/M2CreateColdFiles.ets',
             base/'ets/physics/DiagnosticColdFiles.ets', base/'ets/physics/M2CreatePackage.ets']
    files += sorted(p for p in folder.iterdir() if p.is_file())
    write('analysis-T521-m2-zero-ledger-evidence-v1.json', dict(
        schema='T521_ARCHITECT_STATIC_TRIAGE_V1', date='2026-11-16',
        status='TRIAGE_COMPLETE_PRECISE_EXISTING_CAPABILITY_EVIDENCE_REQUIRED',
        functionalStatus='NOT_REACHED', device='NOT_RUN', GLExecuted=False,
        packages=packages, layouts=layouts, filesListing=(folder/'files.txt').read_text(encoding='utf-8-sig'),
        nativeDiagnosticLocalFile='ZERO_BYTES_WITHOUT_DEVICE_NATIVE_RECEIPT',
        zeroRawLogMeaning=['not attempted', 'queried legal empty', 'binary path with no original log query',
                           'export partial or transport incomplete remains possible'],
        screenshotObservations={'t461_m2run.jpeg':'LOCK_SCREEN', 't461_real.jpeg':'STATE_LOST WAIT_RETIRED run0:0',
                                't461_run2.jpeg':'PAUSED_CLOCK CLOCK_GAP run0:1, four Q ledgers zero'},
        missing=['actual click command/coordinates/time', 'WS_EVENT t468_create',
                 'device ls-l/stat and fetch exit codes', 'three raw logs + native + manifest per prefix',
                 'native creation code/scope and boot/recovery/pulse root attribution'],
        inputFiles=[fp(p) for p in files]))
    print('T521: signed/unsigned ELF match; local empty files confirmed; native receipt/scope absent')


if __name__ == '__main__':
    t518()
    t521()
