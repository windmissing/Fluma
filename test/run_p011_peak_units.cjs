const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P011PeakSampler}=require('../entry/src/main/ets/physics/P011PeakMetrics.ets');
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {P011NaturalObservationProbe}=require('../entry/src/main/ets/physics/P011NaturalObservation.ets');
let t=100;const clock={clockId:'test-clock',nowElapsedMs:()=>t};
const key=n=>({runId:'run',sequence:n,frameId:n,substepId:n,attemptId:'a'+n,committedOrdinal:n});
const mass=(u,p=0)=>({DH:1000-u-p,DL:0,UH:u,UL:0,PH:p,PL:0,EH:1000,EL:0});
const s=new P011PeakSampler('run',clock);s.accept(1,1,'case');const input=s.consume();
s.commit(key(1),100,input,mass(320,200));s.commit(key(2),200,input,mass(520));
t=3100;s.commit(key(3),t,input,mass(1000));s.accept(0,0,'case');
const r=s.summary();assert.equal(r.peakNonDepositedFraction,.52);assert.equal(r.peakCommitKey.sequence,1);
assert.equal(r.peakWindowSampleCount,2);assert.equal(r.peakStatus,'PASS');assert.equal(r.peakNonDepositedLightFraction,null);
const events=s.inputEvents();assert.equal(events[1].peakInputCommitOrder,5);assert.equal(events[0].shakeOnEventId,events[0].eventId);
events[0].value=0;assert.equal(s.inputEvents()[0].value,1);
const f=new WaterSandField(128,512);f.setNaturalObservationRouteId('dual-field-particle-v1');
f.setParticleRoute({massPackage:.1,nmax:32,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:3,dragLight:2,liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0});
t=0;f.setEndpointSampling(clock,{occurrenceId:'case',commandSequence:f.p011InputSequence,commandPhaseId:f.commandPhaseId,segmentId:null});
const rows=[];f.setNaturalObservationProbe(new P011NaturalObservationProbe(128,row=>rows.push(row)));f.setShakeInput(1);
for(let i=0;i<5;i++){t+=20;assert.ok(f.step(1/60).ok);}
assert.equal(rows.length,5);for(const row of rows){const c=row.context;assert.equal(c.peakConsumedShakeInputValue,1);assert.equal(c.peakFieldRevision,'p011-peak-fields-v1');assert.equal(c.peakConsumedShakeOnEventId,f.getPeakShakeInputEvents()[0].eventId);assert.ok(c.peakInputCommitOrder>1);}
assert.equal(f.getPeakSummary().peakWindowSampleCount,5);
console.log(JSON.stringify({passed:8,failed:0,summary:r}));
