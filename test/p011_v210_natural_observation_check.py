#!/usr/bin/env python3
#!/usr/bin/env python3
import argparse, hashlib, json, math
from pathlib import Path
from typing import Any
from PIL import Image

ROUTE = 'natural-observation-v2.10'
OBSERVATION_ROUTES = {
    'natural-observation-v2.10',
    'natural-relax-hysteresis-v1',
}
LEDGER_SCHEMA_VERSIONS = {1, 2, 3}
PHASES = [
    'baseline',
    'positive',
    'positiveReturn',
    'positiveOvershoot',
    'positiveOvershootReturn',
    'negativeOvershoot',
    'negativeOvershootReturn',
]
GREEN = (36,87,31)

def load_json(path):
    with path.open('r',encoding='utf-8-sig') as f: return json.load(f)

def log_json(line):
    s=line.find('{')
    if s<0: return None
    try: v=json.loads(line[s:])
    except Exception: return None
    return v if isinstance(v,dict) else None

def parse_hilog(path):
    out={'store':None,'layout':None,'metric':0,'errors':[]}
    with path.open('r',encoding='utf-8',errors='replace') as f:
        for line in f:
            if 'P011_OBS_STORE ' in line: out['store']=log_json(line)
            elif 'P011_RASTER_LAYOUT ' in line: out['layout']=log_json(line)
            elif 'P011_OBS_METRIC ' in line and log_json(line): out['metric']+=1
            elif 'P011_OBS_ERROR ' in line: out['errors'].append(line.strip())
    return out

def vec(v,n):
    return isinstance(v,list) and len(v)==n and all(isinstance(x,(int,float)) and math.isfinite(x) for x in v)

def validate(records,cols):
    e=[]
    names=['bh','bl','mh','ml','suspH','suspL','S','waterTop']
    for i,r in enumerate(records):
        p=f'record[{i}]'
        if not isinstance(r,dict): e.append(p+': object'); continue
        c=r.get('context',{})
        if c.get('observationRouteId') not in OBSERVATION_ROUTES:
            e.append(p+': route')
        if c.get('ledgerSchemaVersion') not in LEDGER_SCHEMA_VERSIONS:
            e.append(p+': schema')
        if c.get('transitionTelemetryAvailable') is True and c.get('ledgerSchemaVersion') != 3:
            e.append(p+': transitionSchema')
        if c.get('sequence')!=i+1: e.append(p+': sequence')
        for sn in ['stateBefore','stateAfter']:
            s=r.get(sn,{})
            for n in names:
                if not vec(s.get(n),cols): e.append(f'{p}:{sn}.{n}')
        for gn in ['settle','lift']:
            g=r.get(gn,{})
            for n in ['requested','allowed','heavy','light']:
                if not vec(g.get(n),cols): e.append(f'{p}:{gn}.{n}')
        g=r.get('percolate',{})
        for n in ['requested','allowed','q','r','heavyDown','lightDown']:
            if not vec(g.get(n),cols): e.append(f'{p}:percolate.{n}')
        for n in ['suspensionDiffusion','slip','bottomCreep','splits']:
            if not isinstance(r.get(n),list): e.append(f'{p}:{n}')
        for gn in ['suspensionDiffusion','slip','bottomCreep']:
            for j,g in enumerate(r.get(gn,[])):
                ep=f'{p}:{gn}[{j}]'
                if not isinstance(g,dict): e.append(ep+': object'); continue
                telemetry_available=c.get('transitionTelemetryAvailable') is True
                if 'transitionType' not in g:
                    if telemetry_available:
                        e.append(ep+'.transitionType')
                    continue
                for n in ['effectiveTilt','d','r','dAfter','rAfter']:
                    if not math.isfinite(float(g.get(n,math.nan))):
                        e.append(ep+'.'+n)
                if abs(float(g.get('r',math.nan))-abs(float(g.get('d',math.nan))))>1e-9:
                    e.append(ep+'.r')
                if abs(float(g.get('rAfter',math.nan))-abs(float(g.get('dAfter',math.nan))))>1e-9:
                    e.append(ep+'.rAfter')
                guard_available=g.get('hardGuardAvailable')
                if guard_available is not True:
                    if g.get('hardGuardRejected') is not None or g.get('hardGuardClipped') is not None:
                        e.append(ep+'.guardValueWithoutAvailability')
                else:
                    for n in ['hardGuardRejected','hardGuardClipped']:
                        value=g.get(n)
                        if not isinstance(value,(int,float)) or not math.isfinite(value) or value<0:
                            e.append(ep+'.'+n)
                if gn!='slip':
                    continue
                active_before=g.get('activeBefore')
                active_after=g.get('activeAfter')
                transition=g.get('transitionType')
                if telemetry_available:
                    if not isinstance(active_before,bool) or not isinstance(active_after,bool):
                        e.append(ep+'.activeState')
                    if transition not in {'start','stop','holdActive','holdInactive'}:
                        e.append(ep+'.transitionType')
                    if isinstance(active_before,bool) and isinstance(active_after,bool):
                        expected=('start' if not active_before and active_after else
                                  'stop' if active_before and not active_after else
                                  'holdActive' if active_after else 'holdInactive')
                        if transition!=expected: e.append(ep+'.transitionMismatch')
                else:
                    if active_before is not None or active_after is not None or transition!='unavailable':
                        e.append(ep+'.telemetryUnavailable')
        b=r.get('budgets',{})
        for n in ['localLimit','localUsed','localRemaining']:
            if not vec(b.get(n),cols): e.append(f'{p}:budgets.{n}')
        a=r.get('audit',{})
        for n in ['finite','range','capacity','stock','conservation']:
            if a.get(n) is not True: e.append(f'{p}:audit.{n}')
        for n in ['bottomBudgetExcess','totalBudgetExcess','maxLocalBudgetExcess']:
            if float(a.get(n,1))>1e-9: e.append(f'{p}:audit.{n}')
        if a.get('errorCode') is not None: e.append(f'{p}:errorCode')
    return e

def finite_number(value):
    return isinstance(value,(int,float)) and not isinstance(value,bool) and math.isfinite(value)

def expected_transition(active_before,active_after):
    if not active_before and active_after: return 'start'
    if active_before and not active_after: return 'stop'
    return 'holdActive' if active_after else 'holdInactive'

def validate_observation_contract(records,cols,manifest=None):
    errors=[]
    for index,record in enumerate(records):
        prefix=f'record[{index}]'
        context=record.get('context',{})
        if context.get('ledgerSchemaVersion')!=3:
            continue
        if context.get('runRole') not in {'calibration','holdout'}:
            errors.append(prefix+':runRole')
        threshold_id=context.get('thresholdCalibrationId')
        if not isinstance(threshold_id,str) or not threshold_id:
            errors.append(prefix+':thresholdCalibrationId')
        for name in ('thetaStartDeg','thetaStopDeg'):
            if not finite_number(context.get(name)):
                errors.append(prefix+':'+name)
        if context.get('transitionTelemetryAvailable') is not True:
            errors.append(prefix+':transitionTelemetryAvailable')
        observation=record.get('relaxHysteresis')
        if not isinstance(observation,dict):
            errors.append(prefix+':relaxHysteresis')
            continue
        if not finite_number(observation.get('effectiveTilt')) or (
            abs(float(observation.get('effectiveTilt',math.nan))-
                float(context.get('effectiveTilt',math.nan)))>1e-9):
            errors.append(prefix+':relaxHysteresis.effectiveTilt')
        thresholds=observation.get('thresholds',{})
        if not isinstance(thresholds,dict):
            errors.append(prefix+':relaxHysteresis.thresholds')
        else:
            for angle_name,slope_name in (
                ('thetaStartDeg','slopeStart'),('thetaStopDeg','slopeStop')):
                angle=context.get(angle_name)
                slope=thresholds.get(slope_name)
                if thresholds.get(angle_name)!=angle:
                    errors.append(prefix+':relaxHysteresis.thresholds.'+angle_name)
                if not finite_number(angle) or not finite_number(slope) or (
                    abs(float(slope)-math.tan(math.radians(angle)))>1e-9):
                    errors.append(prefix+':relaxHysteresis.thresholds.'+slope_name)
            if abs(float(thresholds.get('slopeHardMax',math.nan))-math.tan(math.radians(30)))>1e-9:
                errors.append(prefix+':relaxHysteresis.thresholds.slopeHardMax')
        edges=observation.get('edges')
        if not isinstance(edges,list) or len(edges)!=cols-1 or any(
            not isinstance(edge,dict) or edge.get('interfaceIndex')!=i
            for i,edge in enumerate(edges if isinstance(edges,list) else [])
        ):
            errors.append(prefix+':relaxHysteresis.edges')
            edges=[]
        slip_by_edge={edge.get('interfaceIndex'):edge for edge in record.get('slip',[])
                      if isinstance(edge,dict)}
        max_before=0.0; max_after=0.0
        for edge_index,edge in enumerate(edges):
            edge_prefix=f'{prefix}:relaxHysteresis.edges[{edge_index}]'
            for name in ('d','r','dAfter','rAfter','requested','granted'):
                if not finite_number(edge.get(name)):
                    errors.append(edge_prefix+'.'+name)
            if finite_number(edge.get('d')) and finite_number(edge.get('r')):
                max_before=max(max_before,abs(float(edge['r'])))
                if abs(float(edge['r'])-abs(float(edge['d'])))>1e-9:
                    errors.append(edge_prefix+'.r')
            if finite_number(edge.get('dAfter')) and finite_number(edge.get('rAfter')):
                max_after=max(max_after,abs(float(edge['rAfter'])))
                if abs(float(edge['rAfter'])-abs(float(edge['dAfter'])))>1e-9:
                    errors.append(edge_prefix+'.rAfter')
            active_before=edge.get('activeBefore'); active_after=edge.get('activeAfter')
            if not isinstance(active_before,bool) or not isinstance(active_after,bool):
                errors.append(edge_prefix+'.activeState')
            transition=edge.get('transitionType')
            if transition != expected_transition(
                active_before if isinstance(active_before,bool) else False,
                active_after if isinstance(active_after,bool) else False):
                errors.append(edge_prefix+'.transitionType')
            slip=slip_by_edge.get(edge_index)
            if slip is not None:
                if abs(float(edge.get('requested',math.nan))-float(slip.get('requested',math.nan)))>1e-9 or (
                    abs(float(edge.get('granted',math.nan))-float(slip.get('allowed',math.nan)))>1e-9):
                    errors.append(edge_prefix+'.slipGrant')
            available=edge.get('hardGuardAvailable')
            if not isinstance(available,bool):
                errors.append(edge_prefix+'.hardGuardAvailable')
            elif not available and (
                edge.get('hardGuardRejected') is not None or
                edge.get('hardGuardClipped') is not None):
                errors.append(edge_prefix+'.guardValueWithoutAvailability')
            elif available:
                for name in ('hardGuardRejected','hardGuardClipped'):
                    if not finite_number(edge.get(name)) or float(edge[name])<0:
                        errors.append(edge_prefix+'.'+name)
        if abs(float(observation.get('maxResidualBefore',math.nan))-max_before)>1e-9:
            errors.append(prefix+':relaxHysteresis.maxResidualBefore')
        if abs(float(observation.get('maxResidualAfter',math.nan))-max_after)>1e-9:
            errors.append(prefix+':relaxHysteresis.maxResidualAfter')
        aggregate_available=observation.get('hardGuardAvailable')
        if not isinstance(aggregate_available,bool):
            errors.append(prefix+':relaxHysteresis.hardGuardAvailable')
        elif not aggregate_available and (
            observation.get('hardGuardRejected') is not None or
            observation.get('hardGuardClipped') is not None):
            errors.append(prefix+':relaxHysteresis.guardValueWithoutAvailability')
        elif aggregate_available:
            for name in ('hardGuardRejected','hardGuardClipped'):
                if not finite_number(observation.get(name)) or float(observation[name])<0:
                    errors.append(prefix+':relaxHysteresis.'+name)
            slip_availability=[edge.get('hardGuardAvailable') is True
                               for edge in record.get('slip',[]) if isinstance(edge,dict)]
            if not slip_availability or not all(slip_availability):
                errors.append(prefix+':relaxHysteresis.hardGuardCoverage')
            else:
                rejected=sum(float(edge.get('hardGuardRejected') or 0.0)
                             for edge in record.get('slip',[]) if isinstance(edge,dict))
                clipped=sum(float(edge.get('hardGuardClipped') or 0.0)
                            for edge in record.get('slip',[]) if isinstance(edge,dict))
                if abs(float(observation.get('hardGuardRejected',math.nan))-rejected)>1e-9 or (
                    abs(float(observation.get('hardGuardClipped',math.nan))-clipped)>1e-9):
                    errors.append(prefix+':relaxHysteresis.hardGuardAggregate')
    if manifest is not None:
        for name in ('observationRouteId','runRole','thresholdCalibrationId',
                     'thetaStartDeg','thetaStopDeg','transitionTelemetryAvailable'):
            if name not in manifest:
                errors.append('manifest:'+name)
        for index,record in enumerate(records):
            context=record.get('context',{})
            if context.get('ledgerSchemaVersion')!=3:
                continue
            for name in ('observationRouteId','runRole','thresholdCalibrationId',
                         'thetaStartDeg','thetaStopDeg','transitionTelemetryAvailable'):
                if name in manifest and context.get(name)!=manifest.get(name):
                    errors.append(f'record[{index}]:manifest.{name}')
    return errors

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1048576),b''): h.update(b)
    return h.hexdigest()

def green(px):
    return math.sqrt(sum((px[i]-GREEN[i])**2 for i in range(3)))<=70

def raster(path,layout):
    with Image.open(path) as im:
        im=im.convert('RGB'); w,h=im.size; px=im.load()
    scale_x=w/float(layout['canvasWidth']); scale_y=h/float(layout['canvasHeight'])
    bx,by,bw,bh=map(int,[layout['boxX']*scale_x,layout['boxY']*scale_y,
      layout['boxWidth']*scale_x,layout['boxHeight']*scale_y])
    cols,cap=int(layout['cols']),float(layout['capacity'])
    out=[]; count=0
    for c in range(cols):
        x0=bx+int(c*bw/cols); x1=max(x0+1,bx+int((c+1)*bw/cols))
        y=by+bh-1; found=False; interface=y
        while y>=by:
            ok=False
            for x in range(x0,x1):
                if green(px[x,y]): ok=True; count+=1; break
            if ok: interface=y; found=True
            elif found: break
            y-=1
        if not found: return None
        out.append(cap-(interface+.5-by)*cap/bh)
    return out,count

def reach(samples,f):
    if not samples: return None
    peak=max(abs(a) for _,a in samples)
    if peak<=0: return None
    return next((t for t,a in samples if abs(a)>=f*peak),None)

def tau(samples):
    if len(samples)<3: return None
    peak=max(abs(a) for _,a in samples)
    if peak<=0: return None
    best=None; err=float('inf')
    for n in range(21):
        q=10**(n/2); s=0
        for t,a in samples: s+=(abs(a)-peak*(1-math.exp(-t/q)))**2
        if s<err: err=s; best=q
    return best

def diagnostics(records):
    p={}
    for r in records:
        d=r['context']['phaseD']; x=p.setdefault(d,{'records':0,'bottomRequested':0,'bottomUsed':0,'bottomHeavy':0,'bottomLight':0,'slip':0,'relaxRequested':0,'relaxApplied':0,'relaxOn':0,'limited':0})
        x['records']+=1; x['bottomRequested']+=r['budgets']['bottomRequested']; x['bottomUsed']+=r['budgets']['bottomUsed']
        for g in r['bottomCreep']: x['bottomHeavy']+=g['heavy']; x['bottomLight']+=g['light']
        for g in r['slip']: x['slip']+=g['allowed']
        x['relaxRequested']+=r['audit']['relaxRequestedTotal']; x['relaxApplied']+=r['audit']['relaxAppliedTotal']
        if r['audit']['relaxTriggered']: x['relaxOn']+=1
        if r['budgets']['bottomRequested']>r['budgets']['bottomLimit']+1e-9: x['limited']+=1
    return {'phases':p,
      'transportDirectionCorrect':p.get('positive',{}).get('bottomUsed',0)>0 and
        p.get('negativeOvershoot',{}).get('bottomUsed',0)>0,
      'rateLimitDominant':any(x['limited']>0 for x in p.values()),
      'nearNaturalEquilibrium':all(x['records']==0 or x['relaxOn']/x['records']<.05 for x in p.values())}

def analyze(args):
    errors=[]; records=[]
    with args.ledger.open('r',encoding='utf-8') as f:
        for line in f:
            if line.strip(): records.append(json.loads(line))
    if not records: errors.append('ledger empty'); cols=128
    else: cols=len(records[0]['stateAfter']['bh'])
    manifest=load_json(args.raster_manifest)
    errors+=validate(records,cols)
    errors+=validate_observation_contract(records,cols,manifest)
    log=parse_hilog(args.hilog)
    errors+=log['errors']
    if not log['store']: errors.append('P011_OBS_STORE missing')
    if not log['layout']: errors.append('P011_RASTER_LAYOUT missing')
    if log['metric']<len(records): errors.append('heartbeat count below ledger')
    if len({r['context']['runId'] for r in records})!=1: errors.append('multiple runIds')
    phases={r['context']['phaseD'] for r in records}; complete=all(x in phases for x in PHASES)
    if not complete: errors.append('phase sequence incomplete')
    seq=sorted({r['context']['commandSequence'] for r in records})
    if seq!=list(range(1,8)): errors.append('command sequences invalid')

    layout=manifest.get('layout',log['layout']); entries=manifest.get('captures',[])
    capture_height=float(entries[0].get('height',layout['canvasHeight'])) if entries else float(layout['canvasHeight'])
    pixels_per_model_unit=float(layout.get('pixelsPerModelUnitY',1))*capture_height/float(layout['canvasHeight'])
    byphase={}
    for x in entries:
        path=args.raster_manifest.parent/x['file']
        if not path.is_file(): errors.append('raster missing:'+x['file']); continue
        if sha(path)!=x.get('sha256'): errors.append('raster hash mismatch:'+x['file'])
        result=raster(path,layout)
        if result is None: errors.append('heavy boundary missing:'+x['file']); continue
        byphase.setdefault(x['phase'],[]).append((float(x.get('relativePhaseTimeMs',0)),result[0],result[1]))
    raster_valid=all(x in byphase for x in PHASES)
    if not raster_valid: errors.append('raster phase incomplete')
    baseline=byphase.get('baseline',[None])[-1][1] if byphase.get('baseline') else None
    amp={}; maxamp={'positive':0.,'negativeOvershoot':0.}; green_change=0
    if baseline is not None:
        basegreen=byphase['baseline'][-1][2]; pmy=pixels_per_model_unit
        for phase in PHASES:
            samples=[]
            for t,iface,gc in byphase.get(phase,[]):
                a=max(abs((iface[i]-baseline[i])*pmy) for i in range(len(baseline)))
                samples.append((t,a)); green_change=max(green_change,abs(gc-basegreen))
                if phase in maxamp: maxamp[phase]=max(maxamp[phase],a)
            amp[phase]=samples
    else: raster_valid=False
    residual={x:(amp.get(x,[])[-1][1] if amp.get(x) else None)
              for x in ['positiveReturn','negativeOvershootReturn']}
    diag=diagnostics(records)
    audit_ok=not any('audit.' in x or x.endswith(':errorCode') for x in errors)
    status='invalid'
    if not errors and complete and audit_ok and raster_valid:
        if max(maxamp.values())>=2: status='visibleNaturalLimitObserved'
        elif diag['transportDirectionCorrect'] and diag['rateLimitDominant'] and not diag['nearNaturalEquilibrium']: status='ruleDriveTooWeak'
        elif diag['nearNaturalEquilibrium']: status='intrinsicallySlow'
        else: status='insufficientEvidence'
    base=records[0] if records else None; final=records[-1] if records else None
    delta=[final['stateAfter']['bh'][i]-base['stateBefore']['bh'][i] for i in range(cols)] if base and final else []
    return {'schemaVersion':1,
      'observationRouteId':records[0]['context'].get('observationRouteId',ROUTE) if records else ROUTE,
      'status':status,'phaseComplete':complete,'auditPassed':audit_ok,'rasterValid':raster_valid,
      'A_B_plus_px':maxamp['positive'],'A_B_minus_px':maxamp['negativeOvershoot'],'A_H_pixels':green_change,
      't50_plus_ms':reach(amp.get('positive',[]),.5),'t90_plus_ms':reach(amp.get('positive',[]),.9),
      't50_minus_ms':reach(amp.get('negativeOvershoot',[]),.5),
      't90_minus_ms':reach(amp.get('negativeOvershoot',[]),.9),
      'tau_ms':tau(amp.get('positive',[])),'residual_px':residual,'deltaHeavy':delta,
      'driveDiagnostics':diag,'errors':errors,'recordCount':len(records),'heartbeatCount':log['metric']}

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--ledger',type=Path,required=True); p.add_argument('--hilog',type=Path,required=True)
    p.add_argument('--raster-manifest',type=Path,required=True); p.add_argument('--output',type=Path,required=True)
    p.add_argument('--report',type=Path)
    a=p.parse_args(); s=analyze(a)
    a.output.write_text(json.dumps(s,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    if a.report: a.report.write_text('# P-011 natural-observation-v2.10\n\n'+json.dumps(s,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(json.dumps(s,indent=2,ensure_ascii=False))
    return 0 if s['status']!='invalid' else 1

if __name__=='__main__': raise SystemExit(main())
