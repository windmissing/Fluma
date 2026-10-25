"""Independent peak protocol v1 checker. Never grants formal release."""
import hashlib
import json
import math
from pathlib import Path
EPS = 1e-9
def finite(v): return isinstance(v,(int,float)) and not isinstance(v,bool) and math.isfinite(v)
def check_peak(records, manifest):
    missing=[]; hard=[]; incomplete=[]; baseline=[]; denominator=[]; candidates=[]
    identity_fields = ['buildSha256','paramsHash','inputHash','ledgerHash','endpointClockId']
    for name in identity_fields:
        if not manifest.get(name):missing.append('manifest.'+name)
    events=manifest.get('peakShakeInputEvents')
    if not isinstance(events,list): missing.append('manifest.peakShakeInputEvents'); events=[]
    on_id=manifest.get('shakeOnEventId'); off_id=manifest.get('shakeOffEventId')
    start=manifest.get('shakeOnEffectiveElapsedMs'); off=manifest.get('shakeOffEffectiveElapsedMs')
    for n,v in [('shakeOnEventId',on_id),('shakeOffEventId',off_id),('shakeOnEffectiveElapsedMs',start),('shakeOffEffectiveElapsedMs',off)]:
        if v is None:missing.append('manifest.'+n)
    end=start+3000 if finite(start) else None
    event_map={}; orders={}; last_event=-1; last_event_time=-1
    for e in events:
        try:
            if e['eventId'] in event_map or e['peakInputCommitOrder'] in orders:raise ValueError('DUPLICATE_INPUT_EVENT')
            if not isinstance(e['peakInputCommitOrder'],int) or isinstance(e['peakInputCommitOrder'],bool) or e['peakInputCommitOrder']<0:raise ValueError('INPUT_ORDER_INVALID')
            if not e.get('endpointClockId') or not e.get('caseOccurrenceId'):missing.append('peakShakeInputEvents/endpointClockId or caseOccurrenceId')
            elif e['endpointClockId']!=manifest.get('endpointClockId'):hard.append('INPUT_CLOCK_MISMATCH')
            if e['runId']!=manifest.get('runId') or not finite(e['effectiveElapsedMs']) or e['effectiveElapsedMs']<last_event_time or e['peakInputCommitOrder']<=last_event:raise ValueError('INPUT_IDENTITY_OR_ORDER')
            if not finite(e['value']) or not 0<=e['value']<=1:raise ValueError('INPUT_VALUE')
            event_map[e['eventId']]=e;orders[e['peakInputCommitOrder']]='event';last_event=e['peakInputCommitOrder'];last_event_time=e['effectiveElapsedMs']
        except (KeyError,TypeError,ValueError) as error:hard.append(str(error))
    if events:
        for eid,t,kind in [(on_id,start,'SHAKE_ON'),(off_id,off,'SHAKE_OFF')]:
            e=event_map.get(eid)
            if e is None:incomplete.append('BOUNDARY_EVENT_MISSING')
            elif e.get('effectiveElapsedMs')!=t or e.get('kind')!=kind or e.get('shakeOnEventId')!=on_id:hard.append('BOUNDARY_EVENT_MISMATCH')
    first=records[0] if records else {}; audit=first.get('audit',{})
    vh=manifest.get('initialHeavy',audit.get('heavyTotalBefore'));vl=manifest.get('initialLight',audit.get('lightTotalBefore'))
    if not finite(vh) or not finite(vl):baseline.append('BASELINE_MISSING')
    elif min(vh,vl)<0:baseline.append('BASELINE_NEGATIVE')
    seen=set();last_time=-1;last_order=-1;last_ordinal=None;identity=None;before=[];tau=0
    for i,r in enumerate(records):
        c=r.get('context',{});t=c.get('endpointSampleElapsedMs');order=c.get('peakInputCommitOrder')
        if c.get('commitStatus')!='committed':continue
        if c.get('peakFieldRevision')!='p011-peak-fields-v1':missing.append(f'records/{i}/context/peakFieldRevision')
        for n in ['peakInputCommitOrder','peakConsumedShakeInputEventId','peakConsumedShakeOnEventId','peakConsumedShakeInputValue']:
            if n not in c:missing.append(f'records/{i}/context/'+n)
        k=tuple(c.get(n) for n in ['runId','sequence','frameId','substepId'])
        if k in seen or None in k or c.get('runId')!=manifest.get('runId'):hard.append('COMMIT_KEY_INVALID')
        seen.add(k)
        if not finite(t) or t<last_time:hard.append('ELAPSED_INVALID');continue
        last_time=t
        if not isinstance(order,int) or isinstance(order,bool):continue
        if order in orders or order<=last_order:hard.append('COMMIT_ORDER_INVALID')
        orders[order]='commit';last_order=order
        try:
            state=r['stateAfter'];names=['bh','bl','mh','ml','suspH','suspL']
            arrays=[state[n] for n in names]
            if any(not isinstance(a,list) or any(not finite(v) or v<0 for v in a) for a in arrays) or len({len(a) for a in arrays})!=1:raise ValueError('STATE_INVALID')
            dh=sum(state['bh'])+sum(state['mh']);dl=sum(state['bl'])+sum(state['ml']);uh=sum(state['suspH']);ul=sum(state['suspL'])
            p=r.get('particles')
            if not isinstance(p,dict):missing.append(f'records/{i}/particles');continue
            ph=p.get('massH');pl=p.get('massL')
            if not finite(ph) or not finite(pl) or min(ph,pl)<0:raise ValueError('PARTICLE_INVALID')
            raw=r.get('particleStateSnapshot');pool=json.loads(raw) if isinstance(raw,str) else raw
            pool=pool if isinstance(pool,dict) else p
            if all(n in pool for n in ['pMass','pPhase','pState']):
                def arr(x):return list(x.values()) if isinstance(x,dict) else x
                masses,phases,states=[arr(pool[n]) for n in ['pMass','pPhase','pState']];calc=[0,0];count=0
                if len({len(masses),len(phases),len(states)})!=1:raise ValueError('POOL_LENGTH')
                for mass,phase,st in zip(masses,phases,states):
                    if st==0:continue
                    if st not in (1,2) or phase not in (0,1) or not finite(mass) or mass<=0:raise ValueError('POOL_INVALID')
                    calc[phase]+=mass;count+=1
                if abs(calc[0]-ph)>EPS or abs(calc[1]-pl)>EPS or count!=p.get('count'):raise ValueError('PARTICLE_SUMMARY_MISMATCH')
            eh=vh+c['sourceInjectedH'];el=vl+c['sourceInjectedL'];e=eh+el;u=uh+ul;pm=ph+pl;d=dh+dl
            if e<=0:denominator.append('E_NONPOSITIVE');continue
            if abs(dh+uh+ph-eh)>EPS or abs(dl+ul+pl-el)>EPS:hard.append('PHASE_CONSERVATION')
            for flag in ['finite','stock','capacity','conservation','range']:
                if r.get('audit',{}).get(flag) is False:hard.append('AUDIT_'+flag.upper())
            for n in ['bottomBudgetExcess','totalBudgetExcess','maxLocalBudgetExcess']:
                if finite(r.get('audit',{}).get(n)) and r['audit'][n]>EPS:hard.append('AUDIT_'+n)
            if finite(start) and start<=t<end and (c.get('particleRouteActive') is not True or c.get('particleRouteId')!='dual-field-particle-v1'):incomplete.append('PARTICLE_ROUTE_MISMATCH')
            q=dict(record=r,key=dict(zip(['runId','sequence','frameId','substepId'],k)),elapsed=t,order=order,D=d,U=u,P=pm,M=u+pm,E=e,UH=uh,UL=ul,PH=ph,PL=pl,EH=eh,EL=el,f=(u+pm)/e)
            if start is not None and t<start:before.append(q)
            if start is None or not start<=t<end:continue
            if c.get('endpointSampleStage')!='POST_COMMIT':missing.append(f'records/{i}/context/endpointSampleStage')
            if events:
                on=event_map.get(on_id)
                if on and order<=on['peakInputCommitOrder']:continue
                consumed=event_map.get(c.get('peakConsumedShakeInputEventId'))
                if consumed is None:incomplete.append('CONSUMED_EVENT_MISSING');continue
                if consumed['peakInputCommitOrder']>=order or consumed['effectiveElapsedMs']>t:hard.append('INPUT_AFTER_COMMIT');continue
                latest=max((x for x in events if x['peakInputCommitOrder']<order),key=lambda x:x['peakInputCommitOrder'],default=None)
                if latest is None or latest['eventId']!=consumed['eventId']:hard.append('INPUT_CONSUMPTION_MISMATCH');continue
                if c.get('endpointClockId') not in (None, consumed.get('endpointClockId')):hard.append('COMMIT_CLOCK_MISMATCH')
                if c.get('endpointPhaseOccurrenceId') not in (None, consumed.get('caseOccurrenceId')):incomplete.append('CASE_OCCURRENCE_CHANGED')
                if on and order<=on['peakInputCommitOrder']:continue
            if c.get('peakConsumedShakeOnEventId')!=on_id or c.get('peakConsumedShakeInputValue')!=1:incomplete.append('INPUT_NOT_ONE');continue
            ident=tuple(c.get(n) for n in ['sourceEpoch','resetCount','invertCount','routeChangeCount','modeChangeCount'])
            if identity is None:identity=ident
            elif ident!=identity:incomplete.append('RUN_CONTINUITY_CHANGED')
            ordinal=c.get('endpointCommittedOrdinal')
            if isinstance(ordinal,int):
                if last_ordinal is not None and ordinal!=last_ordinal+1:incomplete.append('COMMIT_COVERAGE_GAP')
                last_ordinal=ordinal
            dt=c.get('physicsDt')
            if finite(dt) and dt>0:tau+=dt
            else:missing.append(f'records/{i}/context/physicsDt')
            candidates.append(q)
        except (KeyError,TypeError,ValueError,OverflowError) as error:hard.append('INVALID_RECORD:'+str(error))
    if not candidates:incomplete.append('NO_ELIGIBLE_SAMPLES')
    if finite(start) and finite(off) and off<end:incomplete.append('EARLY_OFF')
    if end is not None and last_time<end:incomplete.append('WINDOW_NOT_COVERED')
    peak=max(candidates,key=lambda q:(q['f'],-q['order']),default=None)
    f=peak['f'] if peak else None
    status='PASS' if peak and peak['M']>=.5*peak['E'] else 'FAIL' if peak else 'INCOMPLETE'
    if incomplete:status='INCOMPLETE'
    if denominator:status='INVALID_DENOMINATOR'
    if baseline:status='INVALID_BASELINE'
    if missing:status='NOT_IMPLEMENTED'
    if hard:status='INVALID_LEDGER'
    def mx(expr):return max((expr(q) for q in candidates),default=None)
    historical=[q['U']/(q['D']+q['U']) for q in candidates if q['D']+q['U']>0]
    sampled_keys={tuple(x.get(n) for n in ['runId','sequence','frameId','substepId']) for x in manifest.get('legacy250msCommitKeys',[])}
    sampled=[q for q in candidates if tuple(q['key'].values()) in sampled_keys]
    reported=manifest.get('legacyPeakSuspReported')
    legacy_samples=manifest.get('legacy250msSamples',[])
    if reported is None and legacy_samples:
        values=[x.get('suspensionFraction') for x in legacy_samples if finite(x.get('suspensionFraction'))]
        reported=max(values,default=None)
    m0=before[-1]['M'] if before else None
    protocol=Path(__file__).resolve().parents[1]/'docs/peak-protocol-P011-ws08-peak-nondeposited-v1.md'
    sampled_f=max((q['f'] for q in sampled),default=None)
    sampled_h=max((q['U']/(q['D']+q['U']) for q in sampled if q['D']+q['U']>0),default=None)
    attribution=dict(windowEffect=reported-sampled_h if finite(reported) and sampled_h is not None else None,samplingDensityEffect=f-sampled_f if f is not None and sampled_f is not None else None,numeratorDenominatorEffect=sampled_f-sampled_h if sampled_f is not None and sampled_h is not None else None)
    return dict(runIdentity={n:manifest.get(n) for n in ['runId']+identity_fields},differenceAttribution=attribution,peakFieldRevision='p011-peak-fields-v1',peakMethodId='peakNonDepositedFraction',peakMetricRevision='shake-peak-nondeposited-v1',peakProtocolId='p011-ws08-peak-nondeposited',peakProtocolRevision='ws08-peak-nondeposited-v1',peakAuthority='APPROVED',peakProtocolEffective=False,peakFieldAlignment='ALIGNED',peakProtocolSourceHash=hashlib.sha256(protocol.read_bytes()).hexdigest() if protocol.exists() else None,evidenceClassification='CANDIDATE_DIAGNOSTIC',releaseStatus='FORMAL_RELEASE_NOT_AUTHORIZED',peakWindowStartElapsedMs=start,peakWindowEndElapsedMs=end,peakWindowSampleCount=len(candidates),peakStatus=status,peakScore=(10 if f>=.9 else 8 if f>=.75 else 6 if f>=.5 else 4 if f>=.35 else 2 if f>=.2 else 0) if f is not None and status in ('PASS','FAIL') else None,peakNonDepositedFraction=f,peakStockMass=peak['M'] if peak else None,peakReferenceMass=peak['E'] if peak else None,peakCommitKey=peak['key'] if peak else None,peakInputCommitOrder=peak['order'] if peak else None,peakSampleElapsedMs=peak['elapsed'] if peak else None,peakMasses={n:peak[n] for n in ['D','U','P','M','E','UH','UL','PH','PL','EH','EL']} if peak else None,peakNonDepositedHeavyFraction=mx(lambda q:(q['UH']+q['PH'])/q['EH']) if candidates and candidates[0]['EH']>0 else None,peakNonDepositedLightFraction=mx(lambda q:(q['UL']+q['PL'])/q['EL']) if candidates and candidates[0]['EL']>0 else None,heavyPhaseStatus='N/A' if candidates and candidates[0]['EH']==0 else 'OBSERVED',lightPhaseStatus='N/A' if candidates and candidates[0]['EL']==0 else 'OBSERVED',legacyPeakSuspReported=reported,legacyPeakEvidenceKind='legacy-susp-only',legacyPeakSuspHeavyReported=manifest.get('legacyPeakSuspHeavyReported'),legacyPeakSuspLightReported=manifest.get('legacyPeakSuspLightReported'),legacyPeakHistoricalReference=max(historical,default=None),legacyHistoricalReferenceStatus='VALID' if len(historical)==len(candidates) and candidates else 'INVALID_DENOMINATOR',legacyPeakLiveFormula=mx(lambda q:q['U']/max(q['D']+q['U']+q['P'],EPS)),sameReferenceSuspOnlyPeak=mx(lambda q:q['U']/q['E']),OBSERVED_SAMPLED_MAX=max((q['f'] for q in sampled),default=None),sampledEvidence='LOWER_BOUND_ONLY',common250ms=dict(sampleCount=len(sampled),newPeak=max((q['f'] for q in sampled),default=None),historicalPeak=max((q['U']/(q['D']+q['U']) for q in sampled if q['D']+q['U']>0),default=None)),baselineNonDepositedFraction=m0/candidates[0]['E'] if m0 is not None and candidates else None,integratedPhysicsSecondsInWindow=tau,reachabilityNecessaryCondition=None if m0 is None or not candidates else m0+300*tau>=.5*candidates[0]['E'],missingFields=sorted(set(missing)),hardGateFailures=sorted(set(hard)),peakInputEligibilityReasons=sorted(set(incomplete)),baselineErrors=baseline,denominatorErrors=denominator,hardGateStatus='FAIL' if hard else 'PASS',endpointResultReference=manifest.get('endpointResultReference'),naturalnessStatus='NOT_OBSERVABLE',lifecycleStatus='NOT_OBSERVABLE')


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ledger', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    records = [json.loads(line) for line in args.ledger.read_text(encoding='utf8').splitlines() if line.strip()]
    manifest = json.loads(args.manifest.read_text(encoding='utf8'))
    actual_hash = hashlib.sha256(args.ledger.read_bytes()).hexdigest()
    if manifest.get('ledgerHash') not in (None, actual_hash): raise ValueError('LEDGER_HASH_MISMATCH')
    manifest['ledgerHash'] = actual_hash
    report = check_peak(records, manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf8')
    print(json.dumps(report, ensure_ascii=False))
    return 0 if report['peakStatus'] in ('PASS', 'FAIL') else 1
if __name__ == '__main__':
    raise SystemExit(main())
