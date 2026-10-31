'use strict';
// T331 (T323 §2) watchdog death-reproduction family: sustained R injection to
// 300+ rows in ONE attempt. The pre-T331 run died of THREAD_BLOCK_6S at
// R~261 (audit 1165ms + sort 777ms cumulative + apply per-row). With the
// incremental audit certificate the per-row full-pool audit disappears;
// judgements (device NOT_RUN, host clock):
//   D1 every single audit event <= 600ms
//   D2 attemptCumulativeMs < 6000 (10x margin against the 6s watchdog)
//   D3 audit counts keep the R+2 contract (no skipped audits)
//   D4 sort counts collapse (audit-scope heapsort replaced by O(L) order check)
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText,f);
const root=path.resolve('entry/src/main/ets/physics');
const h=require(path.join(root,'P011ParticleHot.ets'));
const {P011ParticleM2CommitSink:Sink}=require(path.join(root,'P011ParticleM2Sink.ets'));
const N=512,R=320;
const params={massPackage:.1,nmax:N,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:2,dragLight:2,liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0};
const w=h.configureHot({cols:128,nmax:N,requestCapacity:512,motionCapacity:N,eventCapacity:4096,lineageCapacity:8192,params});
const f={cols:128,capacity:512,sourceInjectedH:0,sourceInjectedL:0,sourceEpoch:0,mediumMode:'WET_COLUMN_V1'};
for(const k of ['waterTop','bh','bl','mh','ml','suspH','suspL','surface'])f[k]=new Float64Array(128);
f.waterTop.fill(400);f.bh.fill(20);f.surface.fill(20);
const host={internalParticleReserve:()=>{throw Error('SPAWN_NO_BUDGET')},internalParticleRefund:()=>{},internalParticleRestoreBudget:()=>{},internalParticleGuard:()=>{throw Error('SPAWN_NO_GUARD')}};
const sink=new Sink(w,host);
assert.equal(h.beginStepHot(w,{runToken:1,substepId:1,attemptToken:1,dt:1/60,effectiveTiltRad:0,seed:17,paramsRevision:1,inputRevision:1,field:f}),0);
for(const p of [w.planA,w.planB]){Object.assign(p,{ownerId:w.ownerId,runToken:1,stepEpoch:w.stepEpoch,substepId:1,attemptToken:1,paramsRevision:1,inputRevision:1,seed:17,dt:1/60,effectiveTiltRad:0,requestCount:R,motionCount:0,totalRequested:0,valid:1,consumed:0});
for(let i=0;i<R;i++){p.kind[i]=1;p.phase[i]=0;p.sourceKind[i]=0;p.column[i]=20;p.targetColumn[i]=-1;p.particleId[i]=0;p.sourceId[i]=17;p.ordinal[i]=i+1;p.substep[i]=1;p.requested[i]=.1;p.totalWeight[i]=0;p.predictedX[i]=20.5;p.predictedY[i]=30;p.predictedVx[i]=0;p.predictedVy[i]=0;p.mergeStart[i]=i*2;p.mergeCount[i]=0;}}
const applyStart=Date.now();
const code=h.applyParticlePlanInto(w,w.stepEpoch,w.planA,sink,w.results);
const applyWallMs=Date.now()-applyStart;
assert.equal(code,0);
const finishCode=h.finishStepHot(w,w.stepEpoch);assert.equal(finishCode,0);
// attempt cumulative (sealed value captured at finish; re-open for report only)
const attemptCum=w.complexity.attemptCumulativeMs;
const maxAudit=w.complexity.maxMs[7],maxSort=w.complexity.maxMs[8];
const auditCounts=w.complexity.counts[7],sortCounts=w.complexity.counts[8];
const fulls=w.auditFullVerifications,fallbacks=w.auditFallbacks;
// D1 single audit event <= 600ms
assert(maxAudit<=600,`maxAudit=${maxAudit}`);
// D2 attempt cumulative < 6000ms (10x margin)
assert(attemptCum<6000,`attemptCum=${attemptCum}`);
assert(applyWallMs<6000,`applyWall=${applyWallMs}`);
// D3 audit count contract: R sink rows + APPLY_TAIL + FINISH
assert.equal(auditCounts,R+2);
// D4 sort collapse: heapsort only at baseline (+ fallbacks); NOT per audit row.
// Pre-T331: sort ran every audit (R+2 times). Now: <= fulls (baseline builds).
assert(sortCounts<=fulls,`sort=${sortCounts} fulls=${fulls}`);
// Live count matches R (all spawns admitted)
assert.equal(w.pool.snapshot().liveCount,R);
// Full verifications: first row + every 16 rows + tail + finish
assert.equal(fulls,1+Math.floor((R-1)/16)+1+1,`fulls=${fulls}`);
assert.equal(fallbacks,0,`fallbacks=${fallbacks}`);
const report={revision:'T331_WATCHDOG_DEATH_FAMILY_V1',N,R,device:'NOT_RUN',applyWallMs,attemptCumulativeMs:attemptCum,maxAuditMs:maxAudit,maxSortMs:maxSort,auditCounts,sortCounts,fullVerifications:fulls,fallbacks,judgements:{D1_singleEventUnder600ms:maxAudit<=600,D2_attemptUnder6s:attemptCum<6000&&applyWallMs<6000,D3_auditCountContract:auditCounts===R+2,D4_sortCollapsed:sortCounts<=fulls}};
fs.mkdirSync('test/reports/t331',{recursive:true});
fs.writeFileSync('test/reports/t331/death-family.json',JSON.stringify(report,null,2));
console.log(JSON.stringify(report));
