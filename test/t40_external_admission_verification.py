"""T40 independent verification of the T39 O-2 implementation (commit 2f864f0).

Tester-side driver. Consumes the node executor's real-writer contexts and feeds
them into the delivered endpoint checker (read-only) to confirm that the fault
counterexamples required by the architect §9.3 contract are actually detected,
and that the healthy legal-equality case is not a false failure. No protocol
edit, no checker edit, no judgement change, no device.

Outputs: test/reports/t40-external-admission-independent.json
"""
import copy
import hashlib
import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from test_p011_endpoint_check import fixture
from p011_endpoint_check import analyze_endpoint

NODE = ['node', 'test/run_t40_external_admission_independent.cjs']
PROTOCOL = ROOT / 'docs/endpoint-protocol-P011-terminal-nondeposited-v1.md'
IMPL_FILES = [
    'entry/src/main/ets/physics/P011ExternalAdmissions.ets',
    'entry/src/main/ets/physics/WaterSandField.ets',
    'entry/src/main/ets/physics/P011NaturalObservation.ets',
]
PROTOCOL_SHA256 = ('ffe337d73a4ee78b21a48543025468238cb3607d68b386de7b2097fba5148837')


def git(*args):
    return subprocess.run(['git', *args], cwd=ROOT, capture_output=True,
                          text=True, encoding='utf8', check=True).stdout.strip()


def report(context):
    """Run the delivered checker on the standard fixture with the writer's
    externalAdmissionCount/sourceEpoch carried on every committed row."""
    records, manifest, safety = fixture()
    for row in records:
        row['context']['externalAdmissionCount'] = context.get('externalAdmissionCount')
        row['context']['sourceEpoch'] = context.get('sourceEpoch')
    return analyze_endpoint(records, manifest, safety)['occurrences'][0]


class T40Verification(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        run = subprocess.run(NODE, cwd=ROOT, capture_output=True, text=True,
                             encoding='utf8', check=True)
        cls.node = json.loads(run.stdout)
        cls.writer = cls.node['writerContexts']
        cls.checks = []

    def record(self, name, ok, **detail):
        self.checks.append(dict(check=name, ok=bool(ok), **detail))

    # ---- healthy legal equality ----
    def test_healthy_context_passes_without_mismatch(self):
        row = report(self.writer['healthy'])
        self.record('healthy: count==epoch passes; no hard gate failure',
                    row['endpointStatus'] == 'PASS' and
                    not any('EXTERNAL_ADMISSION' in f for f in row['hardGateFailures']),
                    status=row['endpointStatus'], hardGateFailures=row['hardGateFailures'],
                    externalAdmissionCount=row['externalAdmissionCount'])
        self.assertEqual(row['endpointStatus'], 'PASS')

    # ---- fault counterexamples must be detected ----
    def test_fault_contexts_reach_the_hard_gate(self):
        for name in ['epochFault', 'lost', 'duplicate', 'transportCorrupt']:
            with self.subTest(name=name):
                row = report(self.writer[name])
                self.record('fault %s: EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH + INVALID_LEDGER' % name,
                            'EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH' in row['hardGateFailures'] and
                            row['endpointStatus'] == 'INVALID_LEDGER',
                            status=row['endpointStatus'],
                            hardGateFailures=row['hardGateFailures'],
                            count=self.writer[name]['externalAdmissionCount'],
                            epoch=self.writer[name]['sourceEpoch'])
                self.assertIn('EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH', row['hardGateFailures'])
                self.assertEqual(row['endpointStatus'], 'INVALID_LEDGER')

    # ---- invalid values (transport/type damage) ----
    def test_invalid_values_are_rejected_by_type(self):
        for name, value in [('bool', True), ('negative', -1), ('fractional', 1.5)]:
            with self.subTest(name=name):
                ctx = copy.deepcopy(self.writer['healthy'])
                ctx['externalAdmissionCount'] = value
                row = report(ctx)
                self.record('invalid value %s: INVALID_EXTERNAL_ADMISSION_COUNT + INVALID_LEDGER' % name,
                            'INVALID_EXTERNAL_ADMISSION_COUNT' in row['hardGateFailures'] and
                            row['endpointStatus'] == 'INVALID_LEDGER',
                            value=repr(value), status=row['endpointStatus'],
                            hardGateFailures=row['hardGateFailures'])
                self.assertIn('INVALID_EXTERNAL_ADMISSION_COUNT', row['hardGateFailures'])

    def test_absent_count_is_missing_not_defaulted(self):
        ctx = copy.deepcopy(self.writer['healthy'])
        del ctx['externalAdmissionCount']
        row = report(ctx)
        self.record('absent count: missingFields disclosure, never defaulted to epoch or zero',
                    'context.externalAdmissionCount' in row['missingFields'] and
                    row['endpointStatus'] != 'PASS',
                    missing=[m for m in row['missingFields'] if 'externalAdmission' in m],
                    status=row['endpointStatus'])
        self.assertIn('context.externalAdmissionCount', row['missingFields'])

    # ---- legacy missing history ----
    def test_legacy_null_is_disclosed_not_filled(self):
        row = report(self.writer['legacy'])
        self.record('legacy restore: null count disclosed as missing, status not PASS',
                    'context.externalAdmissionCount' in row['missingFields'] and
                    row['endpointStatus'] != 'PASS',
                    missing=[m for m in row['missingFields'] if 'externalAdmission' in m],
                    status=row['endpointStatus'])
        self.assertIn('context.externalAdmissionCount', row['missingFields'])
        self.assertNotEqual(row['endpointStatus'], 'PASS')

    # ---- frozen protocol and implementation identity ----
    def test_protocol_frozen_and_implementation_identified(self):
        digest = hashlib.sha256(PROTOCOL.read_bytes()).hexdigest()
        self.record('endpoint protocol SHA-256 unchanged (ffe337d7...)',
                    digest == PROTOCOL_SHA256, sha256=digest)
        self.assertEqual(digest, PROTOCOL_SHA256)
        self.assertEqual(git('status', '--short', str(PROTOCOL.relative_to(ROOT))), '')
        impl = {}
        for rel in IMPL_FILES:
            impl[rel] = dict(
                headBlob=git('rev-parse', 'HEAD:%s' % rel),
                t39Blob=git('rev-parse', '2f864f0:%s' % rel),
                worktreeDirty=bool(git('status', '--short', rel)))
        self.record('implementation files identified at T39 commit and HEAD',
                    impl['entry/src/main/ets/physics/P011ExternalAdmissions.ets']['t39Blob'] ==
                    impl['entry/src/main/ets/physics/P011ExternalAdmissions.ets']['headBlob'],
                    files=impl)
        self.assertEqual(impl[IMPL_FILES[0]]['t39Blob'], impl[IMPL_FILES[0]]['headBlob'])

    @classmethod
    def tearDownClass(cls):
        payload = dict(
            suite='t40_external_admission_independent',
            taskId='6adda513e4b0e6ce82af399e',
            basis='architecture-P011-endpoint-fields-revision-v1 §9 + manager ruling in T40',
            nodeSuite=dict(suite=cls.node['suite'], passed=cls.node['passed'],
                           failed=cls.node['failed'], total=len(cls.node['results'])),
            writerContexts=cls.writer,
            checks=cls.checks,
            allMatch=all(c['ok'] for c in cls.checks) and cls.node['failed'] == 0,
        )
        out = ROOT / 'test/reports/t40-external-admission-independent.json'
        out.write_text(json.dumps(payload, ensure_ascii=False, indent=1), encoding='utf8')


if __name__ == '__main__':
    unittest.main(verbosity=2)
