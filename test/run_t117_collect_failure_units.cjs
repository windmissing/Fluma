'use strict';
const fs=require('fs'),a=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField,WaterSandErrorCode}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const {P011TiltPreviewTrace}=require('../entry/src/main/ets/physics/P011TiltPreviewTrace.ets');
let passed=0;
function field(){const f=new WaterSandField(128,512);a.equal(f.setParticleRoute(defaultP011ParticleParams()).ok,true);f.setNaturalObservationRouteId('dual-field-particle-v1');return f;}
{
 const f=field(),b=f.particleBinding;b.collectFailureEnabled=true;
 const x=b.beginCollect(1/60,0,1,1,0,0,0);a.equal(x,0);
 const y=b.beginCollect(1/60,0,1,1,0,0,0);a.equal(y,1);
 a.equal(b.collectFailure.phase,'ENTRY');a.equal(b.collectFailure.entryStepOpen,1);
 a.equal(b.collectFailure.entryActive,1);a.equal(b.collectFailure.m2Code,1);
 b.rollback();passed++;
}
{
 const f=field(),b=f.particleBinding;b.collectFailureEnabled=true;b.work.pageBacklog=1;
 const status=b.beginCollect(1/60,0,1,1,0,0,0);a.equal(status,3);
 a.equal(b.collectFailure.phase,'BEGIN_HOT');a.notEqual(b.collectFailure.hotCode,0);
 a.equal(b.collectFailure.predicate,'PAGE_BACKLOG');
 a.equal(b.collectFailure.entryPageBacklog,1);a.equal(b.collectFailure.exitActive,0);passed++;
}
{
 const f=field(),trace=new P011TiltPreviewTrace();f.tiltTracePending=[trace];f.planningPassService.trace=trace;
 f.sourceInjectedHeavy+=4;const drift=f.getDrift();a.ok(drift.heavy>.001);
 a.equal(f.audit(true),WaterSandErrorCode.UNKNOWN);a.equal(trace.firstFailure.origin,'AUDIT');
 a.equal(trace.firstFailure.driftHeavy,drift.heavy);a.equal(trace.firstFailure.sourceHeavy,4);
 a.equal(trace.firstFailure.driftLimit,.001);passed++;
}
{
 const x=field(),y=field(),traces=[];y.setTiltPreviewTraceSink(t=>traces.push(t));
 for(let i=0;i<3;i++){const a0=x.step(1/60),b0=y.step(1/60);delete a0.physicsTimeMs;delete b0.physicsTimeMs;
   a.deepEqual(a0,b0);a.deepEqual(x.snapshot(),y.snapshot());}
 a.ok(traces.every(t=>t.commitAfter>=t.commitBefore&&t.authoritativeBeforeDigest.length===8&&t.authoritativeAfterDigest.length===8));passed++;
}
console.log(JSON.stringify({suite:'T117 collect first failure',passed,failed:0}));
