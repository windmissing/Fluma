const assert=require('node:assert/strict');
const {loader}=require('../t351/load.cjs');
const roots=[process.argv[2],'entry/src/main/ets'];
assert(roots[0],'pass the pre-T381 source root');
// Both modules issue their own run IDs from wall time and an in-module counter.
Date.now=()=>1700000000000;
const load=roots.map(loader);
const config={isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:32,sourceId:51,
 backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'};
const bytes=a=>Buffer.from(a.buffer,a.byteOffset,a.byteLength).toString('hex');
const fieldKeys=['bh','bl','mh','ml','suspH','suspL','sandTopScratch','waterTop','substepMotionBudgetRemaining','substepMotionBudgetUsed'];
const old=load.map(fn=>new (fn('physics/derived/P051DerivedFacadeV2').P051DerivedFacadeV2)(config));
const wet=load.map(fn=>{
 const {WaterSandField,defaultWaterSandParams}=fn('physics/WaterSandField');
 const f=new WaterSandField(128,512);
 f.configure(defaultWaterSandParams());
 f.initialize({waterRegions:[{startCol:0,endColInclusive:127,bottom:300,height:160}],
  heavyDepositedRegions:[{startCol:0,endColInclusive:127,bottom:0,height:100}],
  lightDepositedRegions:[{startCol:0,endColInclusive:127,bottom:100,height:100}],
  heavySuspendedRegions:[],lightSuspendedRegions:[]});
 return f;
});
for(let i=1;i<=30;i++){
 if(i===1||i===16){
  const r={revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:i,inputTimeMs:i*100,
   x:63.99999,y:2,seed:51,requestedH:1.2,requestedL:0};
  const results=old.map(f=>f.inject(r));
  assert(results.every(r=>r.ok));
  assert.deepEqual(...results);
 }
 const tilt=i<16?25:-25;
 old.forEach(f=>f.setTilt(tilt));
 wet.forEach(f=>f.setTilt(tilt));
 const results=old.map(f=>f.step(1/60));
 assert(results.every(r=>r.ok));assert.deepEqual(...results);
 const w=wet.map(f=>f.step(1/60));
 assert(w.every(r=>r.ok));assert.deepEqual(...w);
 assert.deepEqual(...wet.map(f=>fieldKeys.map(k=>bytes(f[k]))));
 assert.deepEqual(...old.map(f=>fieldKeys.map(k=>bytes(f.field[k]))));
 assert.deepEqual(...old.map(f=>structuredClone(f.exportCheckpointCold())));
 const events=old.map(f=>{
  const e=f.field.particleBinding.work.events;
  return [e.count,e.lineageCount,...['kind','particleId','column','targetColumn','requested','granted',
   'remainder','lineage'].map(k=>bytes(e[k]))];
 });
 assert.deepEqual(...events);
}
console.log(JSON.stringify({steps:30,admissions:2,legacyDryCheckpointEventsAndRawEqual:true,wetRawEqual:true}));
