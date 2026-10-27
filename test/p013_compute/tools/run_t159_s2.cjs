'use strict';
const fs=require('node:fs'),path=require('node:path'),Module=require('node:module'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.resolve('.codex/local-test/t159-project/entry/src/main/ets/physics');
const selector=require(path.join(base,'T159Selector.ets')),meta=require(path.join(base,'T159Metadata.ets'));
for(const route of ['LEGACY_SCAN','T123','GPU']){
 let calls=0;meta.t159ResetMetadata();meta.t159Role(5);
 selector.t159Arm(route,route==='GPU'?{selectPair(p,r,w){calls++;meta.t159PoolVersion(p);const found=w.mergeIndex.find(p,r);return{status:found?'PAIR':'NO_PAIR',slotA:w.mergeIndex.selectedA,slotB:w.mergeIndex.selectedB,errorCode:'OFFLINE_STAND_IN'};}}:null);
 try {
  const file=path.resolve('test/run_p011_particle_hot_patch_units.cjs');let source=fs.readFileSync(file,'utf8');
  source=source.replace("require('../entry/src/main/ets/physics/P011ParticleHot.ets')",`require(${JSON.stringify(path.join(base,'P011ParticleHot.ets'))})`);
  source=source.replace("path.resolve('entry/src/main/ets/physics/P011ParticleHot.ets')",`path.resolve(${JSON.stringify(path.join(base,'P011ParticleHot.ets'))})`);
  const m=new Module(file,module);m.filename=file;m.paths=Module._nodeModulePaths(path.dirname(file));m._compile(source,file);
  if(route==='GPU')assert.ok(calls>0);
  console.log(JSON.stringify({route:route==='GPU'?'GPU_INTERFACE_CPU_STAND_IN':route,suite:'original_S2_12_tests',status:'PASS',selectorCalls:calls,gpuExecution:'NOT_RUN'}));
 }finally{selector.t159Disarm();}
}
