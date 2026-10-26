'use strict';
const fs=require('fs'),a=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const f=new WaterSandField(128,512);f.setParticleRoute({massPackage:.1,nmax:16,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:2,dragLight:2,liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0});f.setNaturalObservationRouteId('dual-field-particle-v1');
f.initialize({waterRegions:[{startCol:0,endColInclusive:127,bottom:0,height:160}],heavyDepositedRegions:[{startCol:0,endColInclusive:127,bottom:0,height:24}],lightDepositedRegions:[],heavySuspendedRegions:[],lightSuspendedRegions:[]});
const h=new Float64Array(128);h[64]=1.6;const r=f.injectGestureSand({perColumnHeavy:h,perColumnLight:new Float64Array(128),sourceId:1,sequence:1,inputTime:10,x:64.5,y:100,seed:1});a.equal(r.admittedH,1.6);
let first=null, deliveries=0;f.setTiltPreviewTraceSink(t=>{if(first===null&&t.resultStatus==='blocked')first=t;deliveries++});
for(let i=0;i<=73;i++){const step=f.step(1/60);if(i===73){a.equal(step.ok,false);a.equal(step.errorCode,5)}}
a.ok(first);a.equal(first.status,'COMPLETE');a.equal(first.s0.length,128);a.ok(first.candidates.length<=3);
a.equal(first.runId,f.getP011RunId());const publication=f.createPublishedRenderStateSlot();a.equal(f.copyPublishedRenderStateInto(publication),'OK');a.equal(first.stateEpoch,publication.stateEpoch);
a.equal(f.getDryDiagnostics().sourceEpoch,1);a.ok(f.particleBinding.pool.totals().count>0);
f.setTiltPreviewTraceSink(null);const before=deliveries;f.step(1/60);a.equal(deliveries,before);
const page=fs.readFileSync('entry/src/main/ets/pages/EmptySandDemo.ets','utf8');
for(const marker of ['failureTraceExported','getLastError() !== 5','setTiltPreviewTraceSink(null)','first_code5_trace','payload.length<=size*limit','beforeCommit','afterCommit','actualSensor','beforePool'])a.ok(page.includes(marker),marker);
console.log(JSON.stringify({suite:'T102 first code5 offline',passed:1,failed:0,deliveries}));
