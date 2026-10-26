'use strict';
const fs=require('fs'),a=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
let passed=0;
const measurements=[];
for(const nmax of [16,512]){
  const f=new WaterSandField(128,512),params=defaultP011ParticleParams();params.nmax=nmax;
  a.equal(f.setParticleRoute(params).ok,true);f.setNaturalObservationRouteId('dual-field-particle-v1');
  const b=f.particleBinding;b.work.diagnosticEnabled=true;
  for(let i=0;i<nmax;i++)a.ok(b.pool.allocate({mass:.1,x:64.1+(i%4)*.1,y:100+(i%4)*.1,
    vx:i%3,vy:-i%3,phase:0,sourceKind:0,sourceId:8})>=0);
  const before=b.pool.snapshot();
  // The frozen pool's two smallest IDs are the lexicographic pair.
  const ids=[];for(let i=0;i<nmax;i++)if(before.pState[i])ids.push(before.pId[i]);ids.sort((x,y)=>x-y);
  const h=new Float64Array(128),l=new Float64Array(128);h[64]=.1;
  const receipt=f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:8,sequence:1,inputTime:0,x:64.5,y:100,seed:1});
  a.equal(receipt.admittedH,.1);a.equal(b.work.mergeCalls,1);a.equal(b.work.mergeSuccess,1);
  a.ok(b.work.mergeCandidateChecks<=nmax,`N=${nmax} checks=${b.work.mergeCandidateChecks}`);
  const merge=f.endpointParticleEvents.map(JSON.parse).find(e=>e.kind==='MERGE');
  a.ok(merge,`N=${nmax} MERGE missing`);a.deepEqual(merge.mergedIds,ids.slice(0,2));
  const after=b.pool.snapshot();let mergedSlot=-1;
  for(let i=0;i<after.nmax;i++)if(after.pState[i]&&after.pId[i]===ids[0])mergedSlot=i;
  a.ok(mergedSlot>=0);a.ok(Math.abs(after.pX[mergedSlot]-64.15)<1e-12);
  a.ok(Math.abs(after.pY[mergedSlot]-100.05)<1e-12);
  a.equal(b.pool.totals().count,nmax);a.ok(Math.abs(b.pool.totals().heavy-(nmax+1)*.1)<1e-9);
  a.equal(b.work.collectCalls,1,'birth-only route must not collect twice');passed++;
  measurements.push({nmax,case:'compatible-full',mergeCalls:b.work.mergeCalls,candidateChecks:b.work.mergeCandidateChecks,collectCalls:b.work.collectCalls});
}
{
  const make=()=>{const f=new WaterSandField(128,512),p=defaultP011ParticleParams();p.nmax=16;
    a.equal(f.setParticleRoute(p).ok,true);f.setNaturalObservationRouteId('dual-field-particle-v1');return f;};
  const off=make(),on=make();on.setPoolProgressDiagnosticEnabled(true);
  const h=new Float64Array(128),l=new Float64Array(128);h[64]=.35;
  for(const f of [off,on]){
    const r=f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:8,sequence:1,inputTime:0,x:64.5,y:100,seed:1});
    a.equal(r.admittedH,.35);
  }
  a.deepEqual(on.particleBinding.pool.snapshot(),off.particleBinding.pool.snapshot());
  a.deepEqual(on.mh,off.mh);a.ok(on.getPoolProgressDiagnostic().includes('mergeCandidateChecks'));
  a.equal(off.getPoolProgressDiagnostic(),null);passed++;
}
for(const nmax of [16,512]){
  const f=new WaterSandField(128,512),p=defaultP011ParticleParams();p.nmax=nmax;
  a.equal(f.setParticleRoute(p).ok,true);f.setNaturalObservationRouteId('dual-field-particle-v1');
  const b=f.particleBinding;b.work.diagnosticEnabled=true;
  for(let i=0;i<nmax;i++)a.ok(b.pool.allocate({mass:.1,x:(i%128)+.5,y:100,vx:0,vy:0,phase:0,sourceKind:0,sourceId:i+1})>=0);
  const before=b.pool.snapshot(),h=new Float64Array(128),l=new Float64Array(128);h[64]=.1;
  const r=f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:77,sequence:1,inputTime:0,x:64.5,y:100,seed:1});
  a.equal(r.admittedH,0);a.deepEqual(b.pool.snapshot(),before);a.equal(b.work.mergeSuccess,0);passed++;
  measurements.push({nmax,case:'incompatible-full',mergeCalls:b.work.mergeCalls,candidateChecks:b.work.mergeCandidateChecks,collectCalls:b.work.collectCalls});
}
console.log(JSON.stringify({suite:'T96 pool hot',passed,failed:0,measurements}));
