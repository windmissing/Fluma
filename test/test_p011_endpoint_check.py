import copy
import hashlib
import unittest
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from p011_endpoint_check import analyze_endpoint, endpoint_state_hash, baseline_state_hash


def fixture(case='TC-WS-08', times=None, u=40, p=40):
    end = 8000 if case == 'TC-WS-08' else (10000 if case=='TC-WS-09' else 1000)
    times = times if times is not None else [end-1000, end-500, end]
    records=[]
    for i,t in enumerate(times):
        count=2 if p else 0
        zero=dict(count=0,massH=0,massL=0)
        particles=dict(count=count,massH=p/2,massL=p/2,
                       byState=dict(FLYING=dict(count=count,massH=p/2,massL=p/2),CONTACT_WAIT=zero.copy()),
                       bySourceKind=dict(GESTURE_EXTERNAL=zero.copy(),SHAKE_LIFT=dict(count=count,massH=p/2,massL=p/2)),
                       degraded=zero.copy(),pMass=[p/2,p/2] if p else [],pPhase=[0,1] if p else [],pState=[1,1] if p else [])
        records.append(dict(context=dict(runId='r',sequence=i+1,frameId=1,substepId=i+1,physicsDt=1/60,
            physicsTime=100000+i,commandSequence=1,commitStatus='committed',phaseD='same-name',
            particleRouteActive=True,particleRouteId='dual-field-particle-v1',sourceInjectedH=0,sourceInjectedL=0,sourceEpoch=0,
            externalAdmissionCount=0,
            endpointFieldRevision='p011-endpoint-fields-v1',endpointSampleStage='POST_COMMIT',endpointSampleElapsedMs=t,
            endpointClockId='r:startup-elapsed-v1',endpointPhaseOccurrenceId='occ-1',endpointCommittedOrdinal=i+1,
            integratedPhysicsSeconds=(i+1)/60,shakeInput=0,resetCount=0,invertCount=0,routeChangeCount=0,modeChangeCount=0),
            stateBefore=dict(bh=[600],bl=[600],mh=[400],ml=[400],suspH=[0],suspL=[0]),
            stateAfter=dict(bh=[(2000-u-p)/2],bl=[(2000-u-p)/2],mh=[0],ml=[0],suspH=[u/2],suspL=[u/2]),
            audit=dict(heavyTotalBefore=1000,lightTotalBefore=1000,finite=True,stock=True,capacity=True,conservation=True,errorCode=None),
            particles=particles,particleEvents=[],particleStateSnapshot='fixture-snapshot',particleCommittedSubstepId=i+1))
    key=dict(runId='r',sequence=1,frameId=1,substepId=1)
    occurrence=dict(caseId=case,endpointPhaseOccurrenceId='occ-1',phaseD='same-name',
                    endpointBoundaryElapsedMs=end,windowStartElapsedMs=end-1000,phaseStartElapsedMs=0,
                    phaseEndElapsedMs=end,shakeOffEffectiveElapsedMs=0,legacyWindowStartElapsedMs=end-5000)
    manifest=dict(runId='r',endpointClockId='r:startup-elapsed-v1',ledgerHash='fixture',
                  initialBaselineCommitKey=key,initialBaselineStateHash=baseline_state_hash(records[0]),occurrences=[occurrence])
    safety=dict(passed=True,ledgerHash='fixture')
    return records,manifest,safety


class EndpointTests(unittest.TestCase):
    def row(self,data): return analyze_endpoint(*data)['occurrences'][0]
    def test_ws_double_denominators_and_true_delta(self):
        r=self.row(fixture(u=40,p=40))
        self.assertEqual(r['endpointStatus'],'PASS')
        self.assertAlmostEqual(r['endpointFraction'],80/2000)
        self.assertAlmostEqual(r['legacyEndpointFraction'],40/1960)
        self.assertAlmostEqual(r['legacyMetricSuspensionFraction'],40/2000)
        self.assertAlmostEqual(r['endpointFractionDelta'],80/2000-40/1960)
        self.assertNotEqual(r['endpointFractionDelta'],r['sameReferenceParticleContribution'])
        self.assertFalse(r['formalReleaseAuthorized'])
    def test_slip_reference_not_display_expected_and_shake_is_allowed(self):
        data=fixture('TC-P011-SLIP-03',u=30,p=40)
        for r in data[0]:r['context']['shakeInput']=0.6
        r=self.row(data)
        self.assertEqual(r['endpointStatus'],'FAIL')
        self.assertAlmostEqual(r['endpointFraction'],70/1200)
        self.assertAlmostEqual(r['suspFraction']+r['airborneFraction'],70/2000)
    def test_window_spike_not_hidden_by_last_step(self):
        data=fixture(u=40,p=40)
        spike=fixture(u=80,p=40)[0][0]
        data[0][0]['stateAfter']=spike['stateAfter']
        r=self.row(data)
        self.assertEqual(r['endpointStatus'],'FAIL')
        self.assertGreater(r['windowPeakFraction'],r['endpointFraction'])
    def test_absolute_mass_epsilon_no_ratio_epsilon(self):
        for extra,want in [(0.5e-9,'PASS'),(2e-9,'FAIL')]:
            r=self.row(fixture(u=60+extra,p=40))
            self.assertEqual(r['endpointStatus'],want)
    def test_degrade_does_not_improve_M_and_cumulative_amount_ignored(self):
        before=fixture(u=40,p=40);after=fixture(u=60,p=20)
        for r in after[0]:r['particles']['degraded']=dict(count=1000,massH=10000,massL=10000)
        self.assertEqual(self.row(before)['endpointFraction'],self.row(after)['endpointFraction'])
    def test_state2_counts_and_no_lifetime_claim(self):
        data=fixture()
        for r in data[0]:
            p=r['particles'];p['byState']['CONTACT_WAIT']=p['byState']['FLYING'];p['byState']['FLYING']=dict(count=0,massH=0,massL=0);p['pState']=[2,2]
        row=self.row(data)
        self.assertEqual(row['endpointStatus'],'PASS')
        self.assertIn('PERSISTENT_PARTICLE_RESIDUE',row['particleResidueDiagnosticCodes'])
        self.assertEqual(row['longTermParticleResidueStatus'],'NOT_OBSERVABLE')
    def test_summary_mismatch_nonfinite_negative_and_phase_conservation_fail(self):
        for mutate in [lambda r:r['particles'].update(massH=99),
                       lambda r:r['stateAfter']['suspH'].__setitem__(0,float('nan')),
                       lambda r:r['particles']['pState'].__setitem__(0,9),
                       lambda r:r['stateAfter']['bh'].__setitem__(0,-1),
                       lambda r:r['stateAfter']['bh'].__setitem__(0,1000)]:
            data=fixture();mutate(data[0][-1]);self.assertEqual(self.row(data)['endpointStatus'],'INVALID_LEDGER')
    def test_missing_particles_keeps_legacy_no_zero_fabrication(self):
        data=fixture()
        for r in data[0]:r.pop('particles')
        row=self.row(data)
        self.assertEqual(row['endpointStatus'],'NOT_IMPLEMENTED')
        self.assertIsNone(row['endpointFraction']);self.assertIsNotNone(row['legacyEndpointFraction'])
    def test_missing_byState_is_diagnostic_only(self):
        data=fixture()
        for r in data[0]:r['particles'].pop('byState')
        row=self.row(data);self.assertEqual(row['endpointStatus'],'PASS')
        self.assertEqual(row['particleResidueDiagnosticStatus'],'NOT_OBSERVABLE')
    def test_missing_time_clock_phase_and_epoch_change(self):
        data=fixture();data[0][-1]['context'].pop('endpointSampleElapsedMs')
        self.assertEqual(self.row(data)['endpointStatus'],'NOT_IMPLEMENTED')
        for mutate in [lambda r:r['context'].update(endpointClockId='other'),
                       lambda r:r['context'].update(endpointPhaseOccurrenceId=None),
                       lambda r:r['context'].update(sourceEpoch=1,externalAdmissionCount=1),
                       lambda r:r['context'].update(invertCount=1)]:
            data=fixture();mutate(data[0][-1]);self.assertEqual(self.row(data)['endpointStatus'],'INCOMPLETE')
    def test_missing_ordinal_and_duplicate_key_and_failed_attempt(self):
        data=fixture();data[0][-1]['context']['endpointCommittedOrdinal']=4
        self.assertEqual(self.row(data)['endpointStatus'],'INCOMPLETE')
        data=fixture();data[0].append(copy.deepcopy(data[0][-1]))
        self.assertEqual(self.row(data)['endpointStatus'],'INVALID_LEDGER')
        data=fixture();data[0][1]['context']['commitStatus']='rolled_back'
        self.assertEqual(self.row(data)['endpointStatus'],'INVALID_LEDGER')
    def test_fixed_boundary_not_eof_and_window_not_integrated_clock(self):
        data=fixture(times=[7000,7500,7999]);self.assertEqual(self.row(data)['endpointStatus'],'INCOMPLETE')
        data=fixture()
        for r in data[0]:r['context']['integratedPhysicsSeconds']=0.001
        self.assertEqual(self.row(data)['endpointStatus'],'PASS')
    def test_repeated_phase_names_not_combined(self):
        data=fixture();data[0][-1]['context']['endpointPhaseOccurrenceId']='occ-2'
        self.assertEqual(self.row(data)['endpointStatus'],'INCOMPLETE')
    def test_baseline_and_zero_denominator_and_hard_gate_priority(self):
        data=fixture();data[0][0]['context']['sourceInjectedH']=1
        self.assertIn(self.row(data)['endpointStatus'],('INVALID_BASELINE','INVALID_LEDGER'))
        data=fixture('TC-P011-SLIP-03');data[0][0]['stateBefore'].update(bh=[0],bl=[0],mh=[1000],ml=[1000])
        data[1]['initialBaselineStateHash']=baseline_state_hash(data[0][0])
        self.assertEqual(self.row(data)['endpointStatus'],'INVALID_DENOMINATOR')
        data=fixture();data[2]['passed']=False;data[2]['failures']=['budget']
        self.assertEqual(self.row(data)['endpointStatus'],'INVALID_LEDGER')
    def test_dormant_holds_same_state_without_new_ordinal(self):
        data=fixture(times=[7000]);record=data[0][0];c=record['context']
        proof=dict(kind='DORMANT_STATE_RECHECK',observedElapsedMs=8000,runId='r',endpointClockId=c['endpointClockId'],
                   heldCommitKey=dict(runId='r',sequence=1,frameId=1,substepId=1),heldCommittedOrdinal=1,
                   heldStateHash=endpoint_state_hash(record),sourceEpoch=0,phaseOccurrenceId='occ-1',
                   modelDormant=True,modelFrozen=False,lastErrorCode=None,inputSequence=1,routeId='dual-field-particle-v1')
        data[0].append(proof);row=self.row(data)
        self.assertEqual(row['endpointStatus'],'PASS');self.assertEqual(row['windowCommittedOrdinals'],[1])
        proof['modelFrozen']=True;self.assertEqual(self.row(data)['endpointStatus'],'INCOMPLETE')
    def test_ws09_original_10s_5s_is_not_replaced(self):
        data=fixture('TC-WS-09');self.assertEqual(self.row(data)['endpointStatus'],'PASS')
        data[1]['occurrences'][0]['legacyWindowStartElapsedMs']=9000
        self.assertEqual(self.row(data)['endpointStatus'],'INCOMPLETE')
    def test_baseline_hash_and_nonfinite_clock_rejected(self):
        data=fixture();data[1]['initialBaselineStateHash']='wrong'
        self.assertEqual(self.row(data)['endpointStatus'],'INVALID_BASELINE')
        data=fixture();data[0][-1]['context']['endpointSampleElapsedMs']=float('nan')
        self.assertEqual(self.row(data)['endpointStatus'],'INVALID_LEDGER')

    def test_evidence_mode_enum_and_retrospective_identity(self):
        data=fixture();data[1]['evidenceMode']='CANDIDATE'
        report=analyze_endpoint(*data)
        self.assertEqual(report['status'],'CANDIDATE_ONLY')
        self.assertEqual(report['occurrences'][0]['evidenceClassification'],'CANDIDATE')
        data=fixture();data[1]['evidenceMode']='RETROSPECTIVE_DIAGNOSTIC'
        report=analyze_endpoint(*data)
        self.assertEqual(report['status'],'RETROSPECTIVE_DIAGNOSTIC')
        self.assertEqual(report['occurrences'][0]['evidenceClassification'],'RETROSPECTIVE_DIAGNOSTIC')
        self.assertNotEqual(report['status'],'CANDIDATE_ONLY')
        data=fixture();data[1]['evidenceMode']='CANDIDATE_WITH_RETRO'
        report=analyze_endpoint(*data)
        self.assertEqual(report['status'],'INCOMPLETE')
        self.assertEqual(report['occurrences'][0]['endpointStatus'],'INCOMPLETE')
        self.assertIn('INVALID_EVIDENCE_MODE',report['occurrences'][0]['endpointStatusDetail']['incomplete'])

    def test_protocol_hash_and_endpoint_sample_stage_are_reported(self):
        data=fixture();report=analyze_endpoint(*data);row=report['occurrences'][0]
        source=Path(__file__).resolve().parents[1]/'docs/endpoint-protocol-P011-terminal-nondeposited-v1.md'
        self.assertEqual(report['protocolSourceHash'],hashlib.sha256(source.read_bytes()).hexdigest())
        self.assertEqual(report['protocolSourceHashAlgorithm'],'SHA-256')
        self.assertTrue(row['protocolSourceHash'])
        self.assertEqual(row['endpointSampleStage'],'POST_COMMIT')

    def test_external_admission_count_matches_source_epoch_and_missing_is_not_fabricated(self):
        data=fixture();data[0][-1]['context']['sourceEpoch']=2
        data[0][-1]['context']['externalAdmissionCount']=2
        row=self.row(data)
        self.assertEqual(row['externalAdmissionCount'],2)
        self.assertNotIn('context.externalAdmissionCount',row['missingFields'])
        data=fixture();data[0][-1]['context']['externalAdmissionCount']=1
        self.assertEqual(self.row(data)['endpointStatus'],'INVALID_LEDGER')
        data=fixture()
        for record in data[0]: record['context'].pop('externalAdmissionCount')
        row=self.row(data)
        self.assertEqual(row['endpointStatus'],'NOT_IMPLEMENTED')
        self.assertIn('externalAdmissionCount',row['reportMissingFields'])
        self.assertIn('context.externalAdmissionCount',row['missingFields'])


if __name__=='__main__':unittest.main()
