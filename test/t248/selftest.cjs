'use strict';
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.resolve(process.argv[2]||'.codex/local-test/t248-build2/entry/src/main/ets'),{T248Observation:O}=require(base+'/diagnostics/t248/T248Observation.ets');
const {P051DerivedFacadeV2:F}=require(base+'/physics/derived/P051DerivedFacadeV2.ets');
const cfg={isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:192,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'};
const f=new F(cfg),o=new O();assert(f.inject({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:1,inputTimeMs:1,x:64.5,y:2,seed:51,requestedH:.3,requestedL:0}).ok);
let land=0,landMass=0;for(let i=0;i<65;i++){
 const result=f.step(1/30);assert(result.ok);o.recordStep(1/30,result.ok,result.original.stepped);
 const before=f.exportCheckpointCold();const stats=JSON.parse(o.capture(before.authority));const events=JSON.parse(f.t248LandCold());
 if(events.length){land+=events.length;landMass+=events.reduce((s,e)=>s+e.granted,0);}
 const after=f.exportCheckpointCold();assert.deepEqual(after,before,'cold observer changed authority/cache/ledger');
 const slot=f.createPublishedSlotCold();assert.equal(f.copyPublishedRenderStateInto(slot),'OK');const pub=slot.exportPayloadCold();assert.deepEqual(pub.authority.bh,before.authority.bh);assert.deepEqual(pub.authority.mh,before.authority.mh);
 assert.equal(stats.bh,Array.from(before.authority.bh).reduce((a,b)=>a+b,0));
}
assert(land>0);assert(landMass>0);assert(Math.abs(o.acceptedSeconds-65/30)<1e-12);
let before=o.acceptedSeconds;o.recordStep(1,false,true);o.recordStep(1,true,false);o.recordStep(NaN,true,true);assert.equal(o.acceptedSeconds,before);
console.log(JSON.stringify({revision:'T248_OFFLINE_V1',passed:5,failed:0,checks:['65 successful dt accumulated','failed/nonstep/NaN excluded','LAND event page observed without state mutation','checkpoint byte/acc/cache equality before/after probe','publication bh/mh equals authority'],landPageRows:land,landPageGrantedMass:landMass,note:'event page occurrences, not cumulative unique LAND count; device joins original keys',device:'NOT_RUN'}));
