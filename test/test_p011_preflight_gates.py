"""Host-only regressions against frozen real green evidence and boundary gates."""
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'test'))
import p011_v28_preflight_check as checker


class RealEvidenceTests(unittest.TestCase):
    def run_fixture(self, relative, missing_roi=False):
        evidence = ROOT / relative
        with tempfile.TemporaryDirectory() as temporary:
            command = [sys.executable, str(ROOT / 'test/p011_v28_preflight_check.py')]
            for key, name in [('baseline', 'baseline.jpeg'), ('positive', 'positive.jpeg'),
                              ('negative', 'negative.jpeg'), ('log', 'hilog.txt'),
                              ('manifest', 'manifest.json')]:
                path = evidence / name
                if missing_roi and key == 'baseline':
                    path = Path(temporary) / 'no-green.png'
                    Image.new('RGB', (20, 20), (0, 0, 0)).save(path)
                command += ['--' + key, str(path)]
            output = Path(temporary) / 'result.json'
            process = subprocess.run(command + ['--output', str(output)], capture_output=True, text=True)
            self.assertTrue(output.exists(), process.stderr)
            return process.returncode, json.loads(output.read_text())

    def test_real_default_geometry_shape_failure_exits_nonzero(self):
        code, result = self.run_fixture('test/artifacts/p011-v28-canonical-20261018/green-new-hap')
        self.assertNotEqual(code, 0, 'ROI presence must not hide physical gate failure')
        self.assertTrue(result['roiReady'])
        self.assertFalse(result['physicalGatePassed'])
        self.assertEqual(result['status'], 'FAIL(heavyObservation)')
        for direction in ('positive', 'negative'):
            self.assertLess(result[direction]['heavyShapeRms'], 0.5)

    def test_historical_real_green_fixture_remains_pass(self):
        code, result = self.run_fixture('demo/effect/P011-v28-heavy-interface/preflight')
        self.assertEqual(code, 0)
        self.assertTrue(result['roiReady'])
        self.assertTrue(result['physicalGatePassed'])
        self.assertEqual(result['status'], 'OBSERVATION_READY')

    def test_roi_missing_blocks_even_when_physical_metrics_pass(self):
        code, result = self.run_fixture('demo/effect/P011-v28-heavy-interface/preflight', missing_roi=True)
        self.assertNotEqual(code, 0)
        self.assertFalse(result['roiReady'])
        self.assertTrue(result['physicalGatePassed'])
        self.assertEqual(result['status'], 'BLOCKED(roiObservation)')


class ThresholdTests(unittest.TestCase):
    def summary(self, **changes):
        result = dict(heavyFluxNonZeroSamples=30, heavyShapeRms=0.5, heavyShapeMaxDelta=1.0)
        result.update(changes)
        return result

    def test_exact_boundaries_pass(self):
        self.assertTrue(all(checker.physical_gates(self.summary()).values()))

    def test_each_threshold_is_mandatory(self):
        for field, value in [('heavyFluxNonZeroSamples', 29), ('heavyShapeRms', 0.499999),
                             ('heavyShapeMaxDelta', 0.999999), ('heavyShapeRms', float('nan')),
                             ('heavyShapeMaxDelta', float('inf'))]:
            with self.subTest(field=field, value=value):
                self.assertFalse(all(checker.physical_gates(self.summary(**{field: value})).values()))


if __name__ == '__main__':
    unittest.main()
