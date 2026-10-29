"""AFTER V2 authority is isolated from the T250 BEFORE inventory; never rewrite V1."""
from pathlib import Path
import json,hashlib,gzip,sys
root=Path(__file__).resolve().parents[2];out=root/'test/reports/t278';project=Path(sys.argv[1]);run=json.loads((out/'execution-v8.json').read_text(encoding='utf-8-sig'))
assert run['failed']==0 and run['passed']==99
sha=lambda b:hashlib.sha256(b).hexdigest()
v1=root/'test/reports/t250/cut-inventory.json';legacy=json.loads(v1.read_bytes());(out/'cut-inventory-v1.json').write_bytes(v1.read_bytes())
# consumer file, function, predicate binding substring, fixture/readset, original return, original first-code owner
defs={
1:('WaterSandField.ets','injectGestureSand','if (!anyWet)','root1 legal positive request; candidate requested-column water equal sand+susp+particle; undo in finally','receipt NO_WATER, admitted=0, txCode=0','source route receipt, no new business code'),
2:('WaterSandField.ets','injectGestureSand',"this.gestureReceipt(request, 0, 0, 'CAPACITY_BLOCKED')",'root1 legal finite offeredH=1e9 in cold clone; unchanged lambda/global capacity/eta solve','receipt CAPACITY_BLOCKED, admitted=0, txCode=0','source lambda/eta/audit receipt, no new business code'),
3:('P011ParticleHot.ets','beginStepHotBody','txTrace.enter(3)','successful begin: active/epoch/backup/pool raw snapshot','0','begin; AFTER OBS only'),
4:('P011ParticleHot.ets','collectParticlePlanIntoBody','if (work.active !== 1 || expectedStepEpoch !== work.stepEpoch)','root2 work.active=0 only before original collect; finally restore=1','P011_HOT_STALE_HANDLE=4; wrapper335 propagates','collect predicate scope315, propagated335; successful header not re-coded'),
5:('P011ParticleM2Field.ets','beginCollectBody','txTrace.enter(5)','original A/B return after collect, read-only twice','0 / original nonzero','actual A/B collect; AFTER OBS only'),
6:('P011ParticleM2Sink.ets','applyInto','if (target < 0 || target >= work.cols)','root2 validated request p.column=cols before target consumer; finally restore column','2','sink target check scope7; request validator success unchanged'),
7:('P011ParticleM2Sink.ets','applyInto','if (work.events.count + eventCount > work.events.capacity)','root2 after successful prepare: events.count=capacity; finally undo; same consumer as cut8','failHot(9)','event capacity in sink scope7, not preceding reserve/guard'),
8:('P011ParticleM2Sink.ets','applyInto','if (work.events.count + eventCount > work.events.capacity)','root2 successful prepare token then events.count=capacity; finally undo','failHot(9)','same event-capacity consumer as cut7; deduplicate EVENT_CAPACITY_9'),
9:('P011MergeExactIndex.ets','find','find(p: P011ParticleSnapshot,radius: number)','original found / noPair / invalidInput cold snapshots; input bytes unchanged; root2 found uses original LEGACY_SCAN fixture adapter','true / false+invalid=false / false+invalid=true','find outcome only; noPair is not an invented business error'),
10:('P011ParticleM2Sink.ets','applyInto','if (this.failAfterPublish !== 0)','root2 existing failAfterPublish=1 after real publish and SINK_ROW audit; finally restore','failHot(13)','existing sink failAfterPublish branch scope7; publish remains successful'),
11:('P011ParticleM2Sink.ets','applyInto',"auditParticleHot(work,'SINK_ROW')",'exact SINK_ROW audit return before failAfterPublish; original dynamic scope11','0 / original audit code','audit scope11; after successful audit OBS only'),
12:('P011ParticleM2Sink.ets','applyInto','this.budget.closeInto(ticketToken, granted, this.refund)','positive real ticket in cold SHAKE1/TILT20 fixture; read after successful close, before event seal','0','original close; no mutation of successful close'),
13:('P011ParticleHot.ets','applyParticlePlanIntoBody','txTrace.enter(13)','original motion/apply output and snapshot after return','original status','motion/apply; OBS only'),
14:('P011ParticleHot.ets','applyParticlePlanIntoBody',"auditParticleHot(work, 'APPLY_TAIL')",'exact APPLY_TAIL original audit return, dynamic scope14','0 / original audit code','audit scope14; successful return OBS only'),
15:('P011ParticleHot.ets','finishStepHot','export function finishStepHot(','read only after wrapper336 returns; not inner FINISH audit marker15','original finish status','finish336; OBS only'),
16:('WaterSandField.ets','previewChannelPlansAt','private previewChannelPlansAt(','original preview/restore result and authority raw bytes','plan / null, original return retained','parent planner; OBS only'),
17:('WaterSandField.ets','stepWithTiltTrace','stepWithTiltTrace(','original root substep prefix and tail receipt; no post-return corruption','original step receipt','actual root/substep; OBS only'),
18:('WaterSandField.ets','audit','private audit(checkDrift: boolean)','audit successful original return with immutable raw inventory/capacity/conservation','null / original WaterSandErrorCode','audit scope18; OBS only'),
19:('WaterSandField.ets','injectGestureSand','this.sourceEpoch += 1','already successful source authority and receipt; no AFTER mutation','original accepted source receipt','NONE: authority already committed'),
20:('WaterSandField.ets','finishNaturalObservation','private finishNaturalObservation(','terminal observation record/error/missing status','original observation record','NONE: terminal boundary'),
}
nexts={1,2,4,6,7,8,10};nas={19,20};rows=[]
for old in legacy['cuts']:
 n=old['cutId'];file,fn,predicate,fixture,ret,owner=defs[n];src=root/'entry/src/main/ets/physics'/file;s=src.read_text(encoding='utf-8-sig');start=s.find(predicate)
 if start<0:raise RuntimeError(f'UNBOUND:{n}:{predicate}')
 disposition='NEXT' if n in nexts else 'NA' if n in nas else 'OBS'
 r={**old,'beforeStateIsHistorical':True,'afterDisposition':disposition,'consumerFile':str(src.relative_to(root)).replace('\\','/'),'consumerFunction':fn,'consumerPredicate':predicate,'consumerBinding':{'line':s.count('\n',0,start)+1,'sourceSha256':sha(src.read_bytes())},'fixtureInput':fixture,'expectedOriginalReturn':ret,'firstCodeOwner':owner,'rollbackBoundary':'original candidate rollback only; successful root1 prefix retained' if n in nexts else 'read-only, no failure or rollback fabricated','reason':'real next consumer; original return and cold rollback/withdrawal evidence' if n in nexts else 'authority committed / terminal; no same-root next rejecting consumer' if n in nas else 'successful original return; immutable AFTER observation','AFTER':'AFTER_NEXT_EXERCISED' if n in nexts else 'AFTER_NA' if n in nas else 'AFTER_OBSERVED','faultInjectionPassEligible':n in nexts,'uniqueConsumer':'EVENT_CAPACITY_9' if n in (7,8) else str(n) if n in nexts else None}
 if n in (11,14):r['originalSites']=[{'file':r['consumerFile'],'line':r['consumerBinding']['line'],'context':predicate,'bindingRevision':'T278_EXACT_CALLSITE_V2','dynamicAuditScope':n}];r['bindingReason']='V1 regex missed dynamic source===APPLY_TAIL?14:source===FINISH?15:11; exact original call now bound; no new checkpoint'
 rows.append(r)
index=[]
for c in run['cases']:
 name=('obs' if c['kind']=='OBS' else 'next-'+str(c['cutId']))+'-'+c['freezeArm']+'.json.gz';raw=json.dumps(c,ensure_ascii=False,separators=(',',':')).encode();data=gzip.compress(raw,mtime=0);(out/name).write_bytes(data);index.append({'kind':c['kind'],'cutId':c.get('cutId'),'arm':c['freezeArm'],'file':name,'sha256':sha(data),'uncompressedSha256':sha(raw)})
manifest=(project/'t278-manifest.json').read_bytes();(out/'manifest.json').write_bytes(manifest)
summary={k:v for k,v in run.items() if k!='cases'};summary.update({'buildInputManifestSha256':sha(manifest),'archives':index,'hostRuntime':'NODE_TYPESCRIPT_TRANSPILE_ORIGINAL_GENERATED_ETS_NOT_OHOS_ABI','scopeArchiveNote':'legacy4096 scope rows may overflow; complete per-cut AFTER raw witnesses are separately bounded256 and asserted dropped=0; no claim of whole-stream scope completeness'})
(out/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(out/'cut-inventory.json').write_text(json.dumps({'revision':'T250_CUT_SITES_V2','owner':'T278_AFTER_ONLY','legacyV1Sha256':sha(v1.read_bytes()),'historicalBeforeUnchanged':True,'cuts':rows,'nextDescriptors':7,'uniqueFaultConsumersPerArm':6,'obsDescriptors':11,'naDescriptors':2,'faultPassDenominatorExcludes':['OBS','NA','duplicate predecessor7/8 of same capacity consumer','all T250 BEFORE'],'allRawWitnessArchives':index},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'passed':run['passed'],'failed':0,'NEXT':7,'uniqueConsumersPerArm':6,'OBS':11,'NA':2,'archives':len(index)}))
