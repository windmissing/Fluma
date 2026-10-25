"""T39 writer-to-existing-checker fault tests; no protocol changes or device."""
import copy
import json
import subprocess
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from test_p011_endpoint_check import fixture
from p011_endpoint_check import analyze_endpoint

class AdmissionWriterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[1]
        run = subprocess.run(['node', 'test/run_p011_external_admission_units.cjs'], cwd=root,
                             capture_output=True, text=True, encoding='utf8', check=True)
        cls.writer = json.loads(run.stdout)['writerContexts']

    def report(self, context):
        records, manifest, safety = fixture()
        for row in records:
            for name in ['externalAdmissionCount', 'sourceEpoch']:
                row['context'][name] = context[name]
        return analyze_endpoint(records, manifest, safety)['occurrences'][0]

    def test_healthy_writer_has_independent_agreeing_sources(self):
        context = self.writer['healthy']
        self.assertEqual(context['externalAdmissionCount'], context['sourceEpoch'])
        self.assertEqual(context['externalAdmissionEvidence']['observedTransactions'], 1)
        self.assertNotIn('EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH', self.report(context)['hardGateFailures'])

    def test_real_writer_epoch_loss_and_duplicate_faults_reach_existing_hard_gate(self):
        for name in ['epochFault', 'lost', 'duplicate']:
            with self.subTest(name=name):
                row = self.report(self.writer[name])
                self.assertIn('EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH', row['hardGateFailures'])
                self.assertEqual(row['endpointStatus'], 'INVALID_LEDGER')

    def test_transport_corruption_of_healthy_context_is_rejected(self):
        context = copy.deepcopy(self.writer['healthy'])
        context['externalAdmissionCount'] += 1
        row = self.report(context)
        self.assertIn('EXTERNAL_ADMISSION_COUNT_EPOCH_MISMATCH', row['hardGateFailures'])
        self.assertEqual(row['endpointStatus'], 'INVALID_LEDGER')

    def test_legacy_history_missing_is_not_filled_with_epoch_or_zero(self):
        row = self.report(self.writer['legacy'])
        self.assertIn('context.externalAdmissionCount', row['missingFields'])
        self.assertNotEqual(row['endpointStatus'], 'PASS')

if __name__ == '__main__':
    unittest.main()
