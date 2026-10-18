"""P-011 v2.8 regressions for canonical frozen TC02 evidence."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import p011_tc02_schedule as tc02
import p011_check as checker


class CaptureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.log = self.root / 'p011-tc02-formal.log'
        self.schedule = tc02.build_schedule(100)
        self.log.write_text('\n'.join('P011_EVENT input ' + json.dumps({
            'source': 'script', 'sequence': index, 'runId': 'one-run',
            'acceptedTime': index * 116, 'targetTilt': row['targetTilt'],
        }) for index, row in enumerate(self.schedule, 1)), encoding='utf-8')

    def freeze(self):
        identity = tc02.prepare_capture(self.root / 'schedule.csv', self.root / 'device.sh')
        context = {'injectionCompleted': True, 'collectorStopped': True,
                   'collectorPid': 123, 'residualCollectors': 0}
        return tc02.freeze_capture(self.log, identity, context)

    def validate(self):
        return tc02.validate_log(self.schedule, 100, self.log)

    def test_531_closed_events_without_capture_audit_fail(self):
        result = self.validate()
        self.assertFalse(result['passed'])
        self.assertIn('captureNotFrozen', ' '.join(result['errors']))

    def test_frozen_capture_passes_with_complete_before_after_audit(self):
        self.freeze()
        result = self.validate()
        self.assertTrue(result['passed'], result['errors'])
        audit = result['captureAudit']
        self.assertTrue(result['captureFrozen'])
        self.assertGreaterEqual(audit['stabilityIntervalMs'], 1000)
        for suffix in ('Size', 'Mtime', 'Sha256'):
            self.assertEqual(audit['preParse' + suffix], audit['postParse' + suffix])

    def test_99ms_accepted_gap_fails_existing_100ms_protocol_gate(self):
        lines = self.log.read_text().splitlines()
        payload = json.loads(lines[1].split('P011_EVENT input ', 1)[1])
        payload['acceptedTime'] = 116 + 99
        lines[1] = 'P011_EVENT input ' + json.dumps(payload)
        self.log.write_text('\n'.join(lines), encoding='utf-8')
        self.freeze()
        result = self.validate()
        self.assertFalse(result['passed'])
        self.assertEqual(result['acceptedTimeGapViolations'], 1)

    def test_changed_capture_after_freeze_fails(self):
        self.freeze()
        with self.log.open('a', encoding='utf-8') as handle:
            handle.write('\nnew data')
        self.assertFalse(self.validate()['passed'])

    def test_changed_during_parse_discards_result(self):
        self.freeze()
        original = tc02.parse_events
        snapshot = tc02.file_snapshot
        def mutating_parse(text):
            result = original(text)
            # Windows holds a read handle denying writers; simulate the second
            # snapshot detecting a change to exercise the fail-closed branch.
            changed = dict(snapshot(self.log), sha256='0' * 64)
            self.snapshot_mock.return_value = changed
            return result
        with patch.object(tc02, 'file_snapshot', return_value=snapshot(self.log)) as self.snapshot_mock, \
                patch.object(tc02, 'parse_events', side_effect=mutating_parse):
            result = self.validate()
        self.assertFalse(result['captureFrozen'])
        self.assertIn('captureNotFrozen', ' '.join(result['errors']))

    def test_capture_path_identity_and_audit_tampering_fail_closed(self):
        self.freeze()
        manifest_path = tc02.capture_manifest_path(self.log)
        original = json.loads(manifest_path.read_text(encoding='utf-8'))
        for section, field, value in (
            ('capturePath', 'shellIntervalSeconds', 0.1),
            ('capturePath', 'scheduleSha256', '0' * 64),
            ('capturePath', 'injectionScriptSha256', '0' * 64),
            ('capturePath', 'hilogParametersSha256', '0' * 64),
            ('captureAudit', 'logPath', str(self.root / 'other.log')),
            ('captureAudit', 'postParseSha256', '0' * 64),
            ('captureAudit', 'collectorStopped', False),
        ):
            with self.subTest(field=field):
                manifest = copy.deepcopy(original)
                manifest[section][field] = value
                manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
                self.assertFalse(self.validate()['passed'])

    def test_freeze_refuses_incomplete_injection_or_live_collector(self):
        identity = tc02.prepare_capture(self.root / 'schedule.csv', self.root / 'device.sh')
        for context in ({}, {'injectionCompleted': True, 'collectorStopped': False},
                        {'injectionCompleted': True, 'collectorStopped': True,
                         'collectorPid': 123, 'residualCollectors': 1}):
            with self.assertRaises(ValueError):
                tc02.freeze_capture(self.log, identity, context)

    def test_checker_blocks_physical_scoring_without_capture_audit(self):
        with patch.object(checker, 'check_suite') as score:
            result = checker.check_file_inputs([self.log], ['behavior'], checker.DEFAULT_ROUTE)
        score.assert_not_called()
        self.assertEqual(result['overall'], 'FAIL(captureNotFrozen)')
        self.assertNotIn('tcP01102', result)

    def test_v28_suite_without_behavior_cannot_bypass_schedule_gate(self):
        with patch.object(checker, 'check_suite') as score:
            result = checker.check_file_inputs([self.log], ['pressureClip'], checker.DEFAULT_ROUTE)
        score.assert_not_called()
        self.assertEqual(result['overall'], 'FAIL(capturePathMismatch)')

    def test_checker_reaudits_immutable_capture_after_physical_scoring(self):
        self.freeze()
        with patch.object(checker, 'check_suite', return_value={'overall': 'PASS'}) as score:
            result = checker.check_file_inputs([self.log], ['behavior'], checker.DEFAULT_ROUTE)
        score.assert_called_once()
        self.assertTrue(result['captureFrozen'])
        self.assertTrue(result['schedulePrecheck']['passed'])

    def test_windows_live_writer_is_rejected(self):
        import os
        if os.name != 'nt':
            self.skipTest('Windows writer sharing semantics')
        self.freeze()
        with self.log.open('a', encoding='utf-8'):
            result = self.validate()
        self.assertFalse(result['passed'])

    def test_historical_route_keeps_legacy_cli_input_contract(self):
        with patch.object(checker, 'check_suite', return_value={'overall': 'PASS'}) as score:
            result = checker.check_file_inputs([self.log], ['behavior'], 'bottom-creep-v2.7.3')
        score.assert_called_once()
        self.assertEqual(result['overall'], 'PASS')

    def test_runner_reuses_frozen_tc02_without_rewriting_evidence(self):
        import os
        if os.name != 'nt':
            self.skipTest('PowerShell runner integration')
        self.freeze()
        before = {path: path.read_bytes() for path in (self.log, tc02.capture_manifest_path(self.log))}
        new_root = self.root / 'reused-suite'
        completed = subprocess.run([
            'powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
            str(Path(tc02.__file__).with_name('p011_tc02_formal_suite.ps1')),
            '-Mode', 'Tc02', '-ReuseTc02Log', str(self.log), '-OutputRoot', str(new_root),
        ], capture_output=True, text=True)
        self.assertEqual(completed.returncode, 0, completed.stderr + completed.stdout)
        validation = json.loads((new_root / 'p011-tc02-validation.json').read_text())
        self.assertTrue(validation['passed'])
        self.assertEqual(validation['capturePath'], json.loads(
            (new_root / 'p011-tc02-capture-path.json').read_text()))
        for path, content in before.items():
            self.assertEqual(path.read_bytes(), content)

    def test_closed_historical_log_cli_missing_audit_has_no_physical_scores(self):
        historical = Path(__file__).parent / 'artifacts/p011-v28-heavy-interface/p011-tc02-formal.log'
        if not historical.exists():
            self.skipTest('Historical closed capture unavailable')
        output = self.root / 'blocked.json'
        completed = subprocess.run([
            sys.executable, str(Path(checker.__file__)), '--input', str(historical),
            '--json', str(output), '--markdown', str(self.root / 'blocked.md'),
        ], capture_output=True, text=True)
        self.assertEqual(completed.returncode, 1, completed.stderr)
        result = json.loads(output.read_text())
        self.assertEqual(result['overall'], 'FAIL(captureNotFrozen)')
        self.assertNotIn('tcP01102', result)


class CompactOnlyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.log = checker.self_check_log()
        cls.baseline = checker.check(cls.log, checker.DEFAULT_ROUTE)

    def test_compact_only_preserves_response_and_substep_scores(self):
        compact = '\n'.join(line for line in self.log.splitlines() if 'P011_BOTTOM' not in line)
        result = checker.check(compact, checker.DEFAULT_ROUTE)
        self.assertEqual(result['overall'], 'PASS', result['dataCompleteness'])
        for field in ('tcP01101', 'tcP01102', 'tcP01103'):
            self.assertEqual(result[field], self.baseline[field])

    def test_truncated_diagnostic_bottom_does_not_block_compact(self):
        truncated = '\n'.join(line[:line.index('P011_BOTTOM') + 30]
                              if 'P011_BOTTOM' in line else line for line in self.log.splitlines())
        result = checker.check(truncated, checker.DEFAULT_ROUTE)
        self.assertEqual(result['overall'], 'PASS', result['dataCompleteness'])
        self.assertGreater(result['bottomDiagnostics']['malformedCount'], 0)

    def test_missing_diagnostic_array_does_not_crash_or_block(self):
        lines = []
        for line in self.log.splitlines():
            if line.startswith('P011_BOTTOM '):
                payload = json.loads(line[len('P011_BOTTOM '):])
                del payload['bh']
                line = 'P011_BOTTOM ' + json.dumps(payload)
            lines.append(line)
        result = checker.check('\n'.join(lines), checker.DEFAULT_ROUTE)
        self.assertEqual(result['overall'], 'PASS', result['dataCompleteness'])
        self.assertGreater(result['bottomDiagnostics']['incompleteCount'], 0)

    def test_missing_compact_identity_still_blocks(self):
        result = checker.check(self.log.replace('"runId":', '"missingRunId":'), checker.DEFAULT_ROUTE)
        self.assertEqual(result['overall'], 'BLOCKED(missing_hook)')

    def test_signed_actual_reverse_motion_is_not_hidden_by_target_or_direction_flag(self):
        lines = self.log.splitlines()
        for index, line in enumerate(lines):
            if line.startswith('P011_METRIC '):
                metric = json.loads(line[len('P011_METRIC '):])
                if metric.get('heavyActualDisplacement', 0) > 0.1:
                    metric['heavyActualDisplacement'] -= 10
                    self.assertTrue(metric['directionCorrect'])
                    lines[index] = 'P011_METRIC ' + json.dumps(metric)
                    break
        result = checker.check('\n'.join(lines), checker.DEFAULT_ROUTE)
        self.assertGreater(result['tcP01101']['bottomDirectionError'], 0)
        self.assertFalse(result['mustPass']['bottomDirectionCorrectAll'])


if __name__ == '__main__':
    unittest.main()
