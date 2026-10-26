// Runs the unmodified S2 transaction tests against the generated Hot closure.
// The injected selector is a CPU contract stand-in; GPU execution is separate.
'use strict';
const fs=require('node:fs'),path=require('node:path'),Module=require('node:module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const generated=path.resolve('.codex/local-test/t141-variant/entry/src/main/ets/physics');
if(!fs.existsSync(path.join(generated,'P011ParticleHot.ets')))
  throw new Error('GENERATE_VARIANT_FIRST');
const hook=require(path.join(generated,'P011GpuPairSelector.ets'));
const {P011MergeExactIndex}=require(path.join(generated,'P011MergeExactIndex.ets'));
const indexes=new Map();let calls=0;
hook.registerT141PairBinding({selectPair(p,radius){calls++;
  let index=indexes.get(p.nmax);if(!index){index=new P011MergeExactIndex(p.nmax);indexes.set(p.nmax,index);}
  const found=index.find(p,radius);
  if(index.invalid)return {status:'ERROR',slotA:-1,slotB:-1,errorCode:'INPUT_INVALID'};
  return found?{status:'PAIR',slotA:index.selectedA,slotB:index.selectedB,errorCode:'NONE'}:
    {status:'NO_PAIR',slotA:-1,slotB:-1,errorCode:'NONE'};
}});
const testPath=path.resolve('test/run_p011_particle_hot_patch_units.cjs');
let source=fs.readFileSync(testPath,'utf8');
const original="require('../entry/src/main/ets/physics/P011ParticleHot.ets')";
if(source.split(original).length!==2)throw new Error('TEST_IMPORT_WHITELIST_MISMATCH');
source=source.replace(original,`require(${JSON.stringify(path.join(generated,'P011ParticleHot.ets'))})`);
const testModule=new Module(testPath,module);testModule.filename=testPath;testModule.paths=Module._nodeModulePaths(path.dirname(testPath));
testModule._compile(source,testPath);
if(calls<1)throw new Error('MERGE_SELECTOR_NOT_EXERCISED');
console.log(`T141 isolated S2 transaction selector calls ${calls} PASS`);
