"""Architect-only static contract audit; no product clock/GL execution or asset regeneration."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / 'docs'


def read(name):
    return json.loads((DOCS / name).read_text(encoding='utf-8-sig'))


def fingerprint(name):
    p = ROOT / name
    b = p.read_bytes()
    return dict(path=name, bytes=len(b), sha256=hashlib.sha256(b).hexdigest())


def main():
    m = read('architecture-T498-m3-engineering-manifest-v1.json')
    p = read('design-T508-blocked-quota-manifest-v1.json')
    d = read('design-T497-gpu-M3-manifest-v1.json')
    f = read('design-T508-blocked-quota-fixtures-v1.json')
    assert m['schema'] == 'T498_M3A_ENGINEERING_MANIFEST_V2'
    assert m['executionReady'] and m['validation']['fullEngineeringGate'] == 'ENGINEERING_READY'
    assert m['validation']['product'] == 'NOT_IMPLEMENTED'
    assert m['versions']['wire'] == 4 and m['versions']['time'] == 1
    assert m['deadlineNs'] == 250000000 and m['gapNs'] == 33333333
    assert m['design']['sha256'] == fingerprint('docs/' + m['design']['path'])['sha256']
    assert d['blockedTimePolicy'] == p
    e = m['blockedControlEngineering']
    for k, v in p.items():
        if k != 'status':
            assert e[k] == v, k
    snapshot = p['snapshot']
    assert snapshot['words'] == 256 and snapshot['bytes'] == 1024
    assert set(snapshot['fields']) == set(map(str, range(256)))
    assert len(set(snapshot['fields'].values())) == 256
    assert set(snapshot['flags'].values()) == set(range(8))
    assert snapshot['magic'] == 860963661 and snapshot['revision'] == 3
    assert p['resourceDelta'] == dict(nativeStateReservedBytes=1024, snapshotBytes=1024,
                                    extraGPUDispatch=0, extraGPUBank=0,
                                    perRootCompletionBytes=1024, extraHotGPUReadback=0)
    cases = {c['id']: c for c in f['cases']}
    assert len(cases) == 21
    for i in range(1, 22):
        assert any(k.startswith(f'BQ{i:02d}_') for k in cases)
    count = 0
    for c in cases.values():
        if c['type'] != 'QUOTA_TRACE':
            continue
        for row in c['steps']:
            s = row['expected']
            assert s['blockedDiscarded'] == sum(s[k] for k in (
                'blockedDiscardedActive', 'blockedDiscardedPending', 'blockedDiscardedWait'))
            assert s['generated'] == sum(s[k] for k in (
                'accepted', 'activeRemaining', 'pending', 'overload', 'revoked', 'blockedDiscarded'))
            assert 120 * s['creditedElapsedNs'] == 10**9 * s['generated'] + sum(s[k] for k in (
                'carry', 'clearedCarry', 'blockedClearedCarry'))
            assert 0 <= s['carry'] < 10**9
            assert all(type(v) is not int or 0 <= v <= 2**64-1 for v in s.values())
            if s['planningMode'] in ('BLOCKED_WAIT', 'REARM_PENDING'):
                assert s['pending'] == s['activeRemaining'] == 0
                assert s['open'] and s['ready'] and s['baseline']
            count += 1
    assert count == 20
    local = cases['BQ04_LOCAL_CARRY_123456789']
    assert local['before']['carry'] == local['expected']['blockedClearedCarry'] == 123456789
    assert 123456789 % 40 != 0 and 123456800 % 40 == 0
    a = cases['BQ01_ACTIVE4']['steps']
    assert a[1]['expected'] == a[2]['expected']
    assert a[1]['expected']['blockedDiscardedActive'] == 4
    b = cases['BQ02_ACTIVE2_PENDING2_CARRY']['steps'][1]['expected']
    assert (b['blockedDiscardedActive'], b['blockedDiscardedPending'], b['blockedClearedCarry']) == (2, 2, 123456800)
    c = cases['BQ03_BLOCK_PULSE_INPUT_REARM']['steps']
    assert c[6]['expected']['planningMode'] == 'RUNNABLE'
    assert c[6]['expected']['pending'] == 0
    assert c[-1]['expected']['accepted'] == 1
    # Local arithmetic identity only, not an alternate product clock implementation.
    arithmetic = 0
    for x in range(1, 5):
        for pending in range(5):
            for carry in (0, 40, 80, 123456800, 999999960):
                before = (x + pending) * 10**9 + carry
                cleared = dict(Bactive=x, Bpending=pending, Bwait=0, X=0, P=0, C=0, BC=carry)
                assert before == sum(cleared[k] for k in ('Bactive', 'Bpending', 'Bwait', 'X', 'P'))*10**9 + cleared['C'] + cleared['BC']
                for gap in (1, 8333333, 10000000, 16666667, 33333333):
                    fresh, remainder = divmod(120 * gap, 10**9)
                    assert before + 120 * gap == (x + pending + fresh)*10**9 + remainder + carry
                    arithmetic += 1
    counts, payloads = [], []
    for q in m['profiles']:
        assert q['sourceDispatch'] == 72
        assert len(q['quantumPasses']) == q['quantumDispatch']
        assert [r['index'] for r in q['quantumPasses']] == list(range(q['quantumDispatch']))
        assert max(r['bindingCount'] for r in q['quantumPasses']) <= 4
        assert q['quantumDispatch'] == 5*(q['nativeM2Dispatches']-5)+9
        assert q['payloadBytes'] == q['m2BaselinePayloadBytes'] + q['extraPayloadBytes']
        counts.append(q['quantumDispatch']); payloads.append(q['payloadBytes'])
    assert counts == [549, 619, 594, 664]
    assert payloads == [61257552, 65229392, 129767504, 133739344]
    names = ['docs/algorithm-T508-m3-blocked-quota-v1.md',
             'docs/design-T508-blocked-quota-manifest-v1.json',
             'docs/design-T508-blocked-quota-fixtures-v1.json',
             'docs/design-T508-blocked-quota-evidence-v1.json',
             'docs/design-T497-gpu-M3-manifest-v1.json',
             'docs/design-T497-gpu-M3-fixtures-v1.json',
             'docs/architecture-T498-gpu-M3-engineering-v1.md',
             'docs/architecture-T498-m3-blocked-control-v2.md',
             'docs/architecture-T498-m3-engineering-manifest-v1.json',
             'docs/analysis-T498-control-gate-v2.py',
             'entry/src/main/cpp/gpu_resident/m1/progress_clock_v2.h']
    out = dict(schema='T498_CONTROL_ENGINEERING_AUDIT_V2', date='2026-11-16',
               status='ENGINEERING_CONTRACT_CHECKS_PASS', product='NOT_IMPLEMENTED',
               device='NOT_RUN', GLExecuted=False, physicsSimulatorAdded=False,
               expectedSnapshots=count, localArithmeticCases=arithmetic,
               fixtureSchemasChecked=21, snapshotWords=256,
               sourceDispatch=72, quantumDispatch=counts, payloadBytes=payloads,
               newHostReservedBytes=2048, newGPUBytes=0,
               limitations=['static identities and contracts only; no native control execution',
                            'negative fixture declarations are not product test results',
                            'current source clock is V2; V3 remains to be implemented'],
               inputs=[fingerprint(n) for n in names])
    (DOCS/'analysis-T498-control-gate-v2.json').write_text(
        json.dumps(out, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: out[k] for k in ['status', 'expectedSnapshots', 'localArithmeticCases', 'quantumDispatch']}))


if __name__ == '__main__':
    main()
