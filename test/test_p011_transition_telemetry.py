#!/usr/bin/env python3
import unittest
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from p011_v210_natural_observation_check import (
    validate,
    validate_observation_contract,
)


def base_record(transition_available: bool, slip: dict):
    return {
        'context': {
            'observationRouteId': 'natural-relax-hysteresis-v1',
            'ledgerSchemaVersion': 3,
            'sequence': 1,
            'runRole': 'calibration',
            'thresholdCalibrationId': 'threshold-calibration-1',
            'thetaStartDeg': 30.0,
            'thetaStopDeg': 20.0,
            'effectiveTilt': 0.0,
            'transitionTelemetryAvailable': transition_available,
        },
        'stateBefore': {name: [0.0, 0.0] for name in
                        ('bh', 'bl', 'mh', 'ml', 'suspH', 'suspL', 'S', 'waterTop')},
        'stateAfter': {name: [0.0, 0.0] for name in
                       ('bh', 'bl', 'mh', 'ml', 'suspH', 'suspL', 'S', 'waterTop')},
        'settle': {name: [0.0, 0.0] for name in ('requested', 'allowed', 'heavy', 'light')},
        'lift': {name: [0.0, 0.0] for name in ('requested', 'allowed', 'heavy', 'light')},
        'percolate': {
            name: [0.0, 0.0] for name in
            ('requested', 'allowed', 'q', 'r', 'heavyDown', 'lightDown')
        },
        'suspensionDiffusion': [],
        'slip': [slip],
        'relaxHysteresis': {
            'effectiveTilt': 0.0,
            'thresholds': {
                'thetaStartDeg': 30.0,
                'thetaStopDeg': 20.0,
                'slopeStart': math.tan(math.radians(30.0)),
                'slopeStop': math.tan(math.radians(20.0)),
                'slopeHardMax': math.tan(math.radians(30.0)),
            },
            'edges': [{
                'interfaceIndex': 0,
                'd': 0.0,
                'r': 0.0,
                'dAfter': 0.0,
                'rAfter': 0.0,
                'activeBefore': slip.get('activeBefore') is True,
                'activeAfter': slip.get('activeAfter') is True,
                'transitionType': slip.get('transitionType', 'holdInactive'),
                'requested': slip.get('requested', 0.0),
                'granted': slip.get('allowed', 0.0),
                'budgetScale': 1.0 if slip.get('requested', 0.0) else None,
                'hardGuardRejected': slip.get('hardGuardRejected'),
                'hardGuardClipped': slip.get('hardGuardClipped'),
                'hardGuardAvailable': slip.get('hardGuardAvailable', False) is True,
            }],
            'hardGuardRejected': slip.get('hardGuardRejected'),
            'hardGuardClipped': slip.get('hardGuardClipped'),
            'hardGuardAvailable': slip.get('hardGuardAvailable', False) is True,
            'maxResidualBefore': 0.0,
            'maxResidualAfter': 0.0,
        },
        'bottomCreep': [],
        'splits': [],
        'budgets': {
            'localLimit': [0.0, 0.0],
            'localUsed': [0.0, 0.0],
            'localRemaining': [0.0, 0.0],
        },
        'audit': {
            'finite': True,
            'range': True,
            'capacity': True,
            'stock': True,
            'conservation': True,
            'bottomBudgetExcess': 0.0,
            'totalBudgetExcess': 0.0,
            'maxLocalBudgetExcess': 0.0,
            'errorCode': None,
        },
    }


def edge(**overrides):
    result = {
        'interfaceIndex': 0,
        'donor': 0,
        'receiver': 1,
        'directionSign': 1,
        'requested': 1.0,
        'allowed': 1.0,
        'heavy': 1.0,
        'light': 0.0,
        'activeBefore': None,
        'activeAfter': None,
        'transitionType': 'unavailable',
        'effectiveTilt': 0.0,
        'd': 0.0,
        'r': 0.0,
        'dAfter': 0.0,
        'rAfter': 0.0,
        'hardGuardRejected': None,
        'hardGuardClipped': None,
        'hardGuardAvailable': False,
    }
    result.update(overrides)
    return result


def manifest_for(record):
    context = record['context']
    return {
        name: context[name]
        for name in (
            'observationRouteId',
            'runRole',
            'thresholdCalibrationId',
            'thetaStartDeg',
            'thetaStopDeg',
            'transitionTelemetryAvailable',
        )
    }


class TransitionTelemetryTests(unittest.TestCase):
    def test_unavailable_state_uses_null_guard_and_state(self):
        record = base_record(False, edge())
        self.assertEqual(validate([record], 2), [])

    def test_available_state_validates_transition(self):
        record = base_record(True, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        self.assertEqual(validate([record], 2), [])

    def test_available_state_rejects_missing_active_state(self):
        record = base_record(True, edge(activeBefore=None, activeAfter=None))
        errors = validate([record], 2)
        self.assertTrue(any(error.endswith('.activeState') for error in errors))

    def test_available_state_rejects_missing_transition_type(self):
        slip = edge(activeBefore=False, activeAfter=True, transitionType='start')
        slip.pop('transitionType')
        record = base_record(True, slip)
        errors = validate([record], 2)
        self.assertTrue(any(error.endswith('.transitionType') for error in errors))

    def test_available_telemetry_requires_schema_v3(self):
        record = base_record(True, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        record['context']['ledgerSchemaVersion'] = 2
        errors = validate([record], 2)
        self.assertTrue(any(error.endswith(': transitionSchema') for error in errors))

    def test_unavailable_state_rejects_fabricated_state(self):
        record = base_record(False, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        errors = validate([record], 2)
        self.assertTrue(any(error.endswith('.telemetryUnavailable') for error in errors))

    def test_guard_zero_is_rejected_without_availability(self):
        record = base_record(True, edge(
            activeBefore=True,
            activeAfter=True,
            transitionType='holdActive',
            hardGuardRejected=0.0,
            hardGuardClipped=0.0,
        ))
        errors = validate([record], 2)
        self.assertTrue(any(error.endswith('.guardValueWithoutAvailability') for error in errors))

    def test_schema_v3_contract_matches_manifest(self):
        record = base_record(True, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        self.assertEqual(validate_observation_contract([record], 2, manifest_for(record)), [])

    def test_schema_v3_contract_rejects_manifest_mismatch(self):
        record = base_record(True, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        manifest = manifest_for(record)
        manifest['runRole'] = 'holdout'
        errors = validate_observation_contract([record], 2, manifest)
        self.assertTrue(any(error.endswith(':manifest.runRole') for error in errors))

    def test_schema_v3_contract_requires_top_level_edges(self):
        record = base_record(True, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        record.pop('relaxHysteresis')
        errors = validate_observation_contract([record], 2, manifest_for(record))
        self.assertTrue(any(error.endswith(':relaxHysteresis') for error in errors))

    def test_schema_v3_observation_run_skips_calibration_threshold_identity(self):
        record = base_record(True, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        record['context']['runRole'] = None
        record['context']['thresholdCalibrationId'] = None
        record['context']['thetaStartDeg'] = None
        record['context']['thetaStopDeg'] = None
        manifest = {
            'observationRouteId': record['context']['observationRouteId'],
            'runRole': None,
            'transitionTelemetryAvailable': True,
        }
        self.assertEqual(validate_observation_contract([record], 2, manifest), [])

    def test_schema_v3_observation_run_still_requires_transition_telemetry(self):
        record = base_record(False, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        record['context']['runRole'] = None
        record['context']['thresholdCalibrationId'] = None
        record['context']['thetaStartDeg'] = None
        record['context']['thetaStopDeg'] = None
        errors = validate_observation_contract(
            [record], 2,
            {'observationRouteId': 'natural-observation-v2.10',
             'runRole': None, 'transitionTelemetryAvailable': False},
        )
        self.assertTrue(any(error.endswith(':transitionTelemetryAvailable') for error in errors))

    def test_schema_v3_unknown_run_role_is_rejected(self):
        record = base_record(True, edge(
            activeBefore=False,
            activeAfter=True,
            transitionType='start',
        ))
        record['context']['runRole'] = 'production'
        errors = validate_observation_contract([record], 2, manifest_for(record))
        self.assertTrue(any(error.endswith(':runRole') for error in errors))

    def test_guard_available_positive_values_are_accepted(self):
        slip = edge(
            activeBefore=True,
            activeAfter=True,
            transitionType='holdActive',
            hardGuardRejected=0.2,
            hardGuardClipped=0.1,
            hardGuardAvailable=True,
        )
        record = base_record(True, slip)
        self.assertEqual(validate_observation_contract([record], 2, manifest_for(record)), [])


if __name__ == '__main__':
    unittest.main()
