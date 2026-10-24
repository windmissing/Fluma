// R3 evidence probe. A zero-allocation claim must include the whole M1 path.
'use strict';
const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(mod,file)=>mod._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,file);
const {P011ParticleField}=require('../entry/src/main/ets/physics/P011ParticleField.ets');
const params={massPackage:0.1,nmax:8,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:2,dragLight:2,
  liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0};
const id={runId:'R3',substepId:1,dt:1/60,effectiveTiltRad:0,seed:1,paramsRevision:1,inputRevision:1};
const view={cols:128,capacity:512,effectiveTiltRad:0,sandTopAt:()=>100,waterTopAt:()=>460,suspAt:()=>0,freeSpaceAt:()=>360};
const pool=new P011ParticleField(128,8);pool.allocate({mass:1,x:60.5,y:100,vx:0,vy:0,phase:0,sourceKind:0,sourceId:1});
pool.beginStep(id,params,view);pool.warmUpTransactions();
const expected=pool.snapshot();const requests=[],grants=[];
for(let n=0;n<100;n++){
  const key='R3-'+n;
  requests.push({requestKey:key,substepId:1,ordinal:n,kind:'deposit',particleId:1,sourceId:1,phase:0,sourceKind:0,
    column:60,targetColumn:-1,requested:1,totalWeight:1,predictedX:60.5,predictedY:100+1e-6,predictedVx:0,predictedVy:0,mergedIds:[]});
  grants.push({requestKey:key,grantedHeavy:0,grantedLight:0,disposition:'WAIT'});
}
const before=pool.transactionBefore,after=pool.transactionAfter,scratch=pool.transactionScratch;
const names=['Float64Array','Uint8Array','Uint32Array','Int32Array'];const originals=names.map(n=>global[n]);
const handles=new Array(100);
let typedArrayConstructors=0;
try{
  names.forEach((name,i)=>global[name]=new Proxy(originals[i],{construct(target,args){typedArrayConstructors++;return Reflect.construct(target,args);}}));
  for(let n=0;n<100;n++){const patch=pool.prepare(requests[n],grants[n]);assert.ok(patch);handles[n]=patch;pool.publish(patch);pool.finishPatch(patch);}
}finally{names.forEach((name,i)=>global[name]=originals[i]);}
assert.equal(pool.transactionBefore,before);assert.equal(pool.transactionAfter,after);assert.equal(pool.transactionScratch,scratch);
assert.equal(typedArrayConstructors,0);
assert.equal(pool.snapshot().pMass[0],expected.pMass[0]);
const sites=[];
const methods=new Set(['beginStep','prepare','edit','mergePair','mergeOldest','collectParticlePlan','record','finish',
  'applyParticlePlan','checkpoint','rollback','apply','executeParticlePlan','reserve','refund','close','view','guard']);
for(const file of ['P011ParticleField.ets','P011ParticlePrototype.ets']){
  const path=require.resolve('../entry/src/main/ets/physics/'+file),source=fs.readFileSync(path,'utf8');
  const ast=ts.createSourceFile(file,source,ts.ScriptTarget.Latest,true,ts.ScriptKind.TS);
  function scan(node,owner='',hot=false){
    if(ts.isMethodDeclaration(node)||ts.isFunctionDeclaration(node)){
      owner=node.name?.getText(ast)||'';hot=methods.has(owner);
    }
    let kind='';
    if(hot){
      if(ts.isNewExpression(node)&&!ts.isThrowStatement(node.parent))kind='constructor';
      else if(ts.isArrayLiteralExpression(node))kind='array-literal';
      else if(ts.isObjectLiteralExpression(node))kind='object-literal';
      else if(ts.isArrowFunction(node)||ts.isFunctionExpression(node))kind='closure';
      else if(ts.isCallExpression(node)&&/JSON\.stringify|\.(slice|map|find|snapshot|checkpoint|view)\b/.test(node.expression.getText(ast)))kind='allocating-helper';
    }
    if(kind){const pos=ast.getLineAndCharacterOfPosition(node.getStart(ast));sites.push({file,owner,line:pos.line+1,kind,text:node.getText(ast).slice(0,110)});}
    ts.forEachChild(node,child=>scan(child,owner,hot));
  }scan(ast);
}
console.log(JSON.stringify({scope:'whole-M1 remaining normal-path allocation sites + buffered pool probe',
  runtime:{iterations:100,typedArrayConstructors,poolScratchReused:true,beforeAfterBuffersReused:true,distinctReturnedPatchObjects:new Set(handles).size},
  status:sites.length?'NOT_ZERO_ALLOCATION':'PASS',remainingSiteCount:sites.length,sites},null,2));
if(sites.length)process.exitCode=2;
