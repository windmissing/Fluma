import copy, sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from p011_peak_check import check_peak

def fixture(points):
    records=[]
    for i,(t,u,p) in enumerate(points):
        c=dict(runId='r',sequence=i+1,frameId=i+1,substepId=i+1,attemptId='a'+str(i),commitStatus='committed',endpointSampleStage='POST_COMMIT',endpointSampleElapsedMs=t,endpointCommittedOrdinal=i+1,physicsDt=.1,sourceInjectedH=0,sourceInjectedL=0,sourceEpoch=0,resetCount=0,invertCount=0,routeChangeCount=0,modeChangeCount=0,particleRouteActive=True,particleRouteId='dual-field-particle-v1',peakFieldRevision='p011-peak-fields-v1',peakInputCommitOrder=2+i,peakConsumedShakeInputEventId='on',peakConsumedShakeOnEventId='on',peakConsumedShakeInputValue=1)
        if t<100:c.update(peakInputCommitOrder=0,peakConsumedShakeInputEventId=None,peakConsumedShakeOnEventId=None,peakConsumedShakeInputValue=0)
        if t>=3100:c.update(peakInputCommitOrder=100+i,peakConsumedShakeInputEventId='off',peakConsumedShakeOnEventId=None,peakConsumedShakeInputValue=0)
        records.append(dict(context=c,stateAfter=dict(bh=[1000-u-p],bl=[0],mh=[0],ml=[0],suspH=[u],suspL=[0]),particles=dict(count=int(p>0),massH=p,massL=0,pMass=[p] if p else [],pPhase=[0] if p else [],pState=[2] if p else []),audit=dict(heavyTotalBefore=1000,lightTotalBefore=0,finite=True,stock=True,capacity=True,conservation=True)))
    events=[dict(eventId='on',runId='r',peakInputCommitOrder=1,effectiveElapsedMs=100,endpointClockId='clock',value=1,kind='SHAKE_ON',shakeOnEventId='on',caseOccurrenceId='case'),dict(eventId='off',runId='r',peakInputCommitOrder=100,effectiveElapsedMs=3100,endpointClockId='clock',value=0,kind='SHAKE_OFF',shakeOnEventId='on',caseOccurrenceId='case')]
    return records,dict(buildSha256='a'*64,paramsHash='b'*64,inputHash='c'*64,ledgerHash='d'*64,endpointClockId='clock',runId='r',shakeOnEventId='on',shakeOffEventId='off',shakeOnEffectiveElapsedMs=100,shakeOffEffectiveElapsedMs=3100,peakShakeInputEvents=events)
class PeakTests(unittest.TestCase):
    def check(self,points):return check_peak(*fixture(points))
    def test_P1_representation(self):
        r=self.check([(0,0,0),(100,320,200),(3100,0,0)]);self.assertEqual(r['peakNonDepositedFraction'],.52);self.assertEqual(r['legacyPeakHistoricalReference'],.4);self.assertEqual(r['peakStatus'],'PASS')
    def test_P2_degrade(self):
        r=self.check([(0,0,0),(100,320,200),(200,520,0),(3100,0,0)]);self.assertEqual(r['peakNonDepositedFraction'],.52);self.assertEqual(r['peakCommitKey']['sequence'],2)
    def test_P3_sum_before_max(self):self.assertEqual(self.check([(0,0,0),(100,400,0),(200,0,400),(3100,0,0)])['peakNonDepositedFraction'],.4)
    def test_P4_response_lag(self):self.assertEqual(self.check([(0,0,0),(100,100,0),(2500,350,200),(3100,0,0)])['peakSampleElapsedMs'],2500)
    def test_P5_rest_pollution(self):self.assertEqual(self.check([(0,800,0),(100,400,0),(3100,0,0)])['peakStatus'],'FAIL')
    def test_P6_contact_wait(self):self.assertEqual(self.check([(0,0,0),(100,20,500),(3100,0,0)])['peakNonDepositedFraction'],.52)
    def test_P7_common_samples(self):
        records,m=fixture([(0,0,0),(100,300,0),(200,520,0),(350,480,0),(3100,0,0)]);m['legacy250msCommitKeys']=[{n:records[i]['context'][n] for n in ['runId','sequence','frameId','substepId']} for i in [1,3]]
        r=check_peak(records,m);self.assertEqual(r['OBSERVED_SAMPLED_MAX'],.48);self.assertEqual(r['peakNonDepositedFraction'],.52);self.assertTrue(r['reachabilityNecessaryCondition'] is False)
    def test_P8_right_boundary(self):self.assertEqual(self.check([(0,0,0),(100,480,0),(3100,350,200)])['peakNonDepositedFraction'],.48)
    def test_missing_events(self):
        records,m=fixture([(0,0,0),(100,500,0),(3100,0,0)]);m.pop('peakShakeInputEvents');self.assertEqual(check_peak(records,m)['peakStatus'],'NOT_IMPLEMENTED')
    def test_fraction_no_tolerance_and_phase_na(self):
        r=self.check([(0,0,0),(100,500-1e-10,0),(3100,0,0)]);self.assertEqual(r['peakStatus'],'FAIL');self.assertEqual(r['lightPhaseStatus'],'N/A')
    def test_pool_mismatch(self):
        records,m=fixture([(0,0,0),(100,100,400),(3100,0,0)]);records[1]['particles']['pMass']=[399];self.assertEqual(check_peak(records,m)['peakStatus'],'INVALID_LEDGER')
    def test_early_off(self):
        records,m=fixture([(0,0,0),(100,500,0),(3100,0,0)]);m['shakeOffEffectiveElapsedMs']=200;m['peakShakeInputEvents'][1]['effectiveElapsedMs']=200;self.assertEqual(check_peak(records,m)['peakStatus'],'INCOMPLETE')
    def test_legacy_report_keeps_full_sequence_and_raw_formula(self):
        records,m=fixture([(0,800,0),(100,400,0),(3100,0,0)])
        m['legacy250msSamples']=[dict(suspensionFraction=.8),dict(suspensionFraction=.4)]
        m['legacy250msCommitKeys']=[{n:records[1]['context'][n] for n in ['runId','sequence','frameId','substepId']}]
        r=check_peak(records,m)
        self.assertEqual(r['legacyPeakSuspReported'],.8)
        self.assertEqual(r['peakNonDepositedFraction'],.4)
        self.assertEqual(r['common250ms']['newPeak'],.4)
        self.assertEqual(r['differenceAttribution']['windowEffect'],.4)
if __name__=='__main__':unittest.main()
