'use strict';
// S4 conservative whole-source call graph; unresolved calls fail closed.
const fs=require('fs'),path=require('path');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const files=['P011ParticleField.ets','P011ParticlePrototype.ets','P011ParticleRefundAdapter.ets',
  'P011ParticleHot.ets','P011ParticleHotBudget.ets','P011ParticleHotPrototype.ets','P011ParticleHotEvidence.ets'];
const roots=new Set(['beginStepHot','checkpointHot','collectParticlePlanInto','compareParticlePlansHot','applyParticlePlanInto',
  'preparePatchHot','publishPatchHot','restorePatchHot','discardPatchHot','finishPatchHot','rollbackStepHot','finishStepHot',
  'copyStateToHot','guardAdmitInto','reserveInto','refundInto','closeInto']);
const pure=new Set(['Math.floor','Math.max','Math.min','Math.sin','Math.cos','Math.tan','Math.exp','Math.abs','Math.imul',
  'Number.isSafeInteger','Number.isFinite','Number.isInteger']);
function conservativeAudit(){
  const nodes=[],byName=new Map(),typed=new Set();
  for(const file of files){
    const ast=ts.createSourceFile(file,fs.readFileSync(path.join(__dirname,'../entry/src/main/ets/physics',file),'utf8'),ts.ScriptTarget.Latest,true,ts.ScriptKind.TS);
    function gather(n,owner=''){
      if(ts.isClassDeclaration(n))owner=n.name?.text||'';
      if((ts.isPropertyDeclaration(n)||ts.isPropertySignature(n))&&n.type&&/^(Float64Array|Float32Array|Uint8Array|Uint32Array|Int32Array)$/.test(n.type.getText(ast)))typed.add(n.name.getText(ast));
      if(ts.isPropertyDeclaration(n)&&n.initializer&&ts.isNewExpression(n.initializer)&&/^(Float64Array|Float32Array|Uint8Array|Uint32Array|Int32Array)$/.test(n.initializer.expression.getText(ast)))typed.add(n.name.getText(ast));
      if((ts.isFunctionDeclaration(n)||ts.isMethodDeclaration(n)||ts.isGetAccessorDeclaration(n))&&n.body){
        const name=n.name?.getText(ast)||'',key=file+'::'+(owner?owner+'.':'')+name;
        const item={key,file,owner,name,node:n,ast,edges:[],allocations:[],unresolved:[]};nodes.push(item);
        if(!byName.has(name))byName.set(name,[]);byName.get(name).push(item);
      }ts.forEachChild(n,c=>gather(c,owner));
    }gather(ast);
  }
  const reachable=new Set(),queue=nodes.filter(n=>roots.has(n.name)||(ts.isGetAccessorDeclaration(n.node)&&n.file==='P011ParticleHot.ets'));
  function position(n,item){const p=item.ast.getLineAndCharacterOfPosition(n.getStart(item.ast));return {file:item.file,line:p.line+1,text:n.getText(item.ast).slice(0,160)};}
  while(queue.length){const item=queue.pop();if(reachable.has(item.key))continue;reachable.add(item.key);
    function scan(n){let allocation='';
      if(ts.isNewExpression(n))allocation='constructor';else if(ts.isObjectLiteralExpression(n))allocation='object-literal';
      else if(ts.isArrayLiteralExpression(n))allocation='array-literal';else if(ts.isArrowFunction(n)||ts.isFunctionExpression(n))allocation='closure';
      else if(ts.isTemplateExpression(n))allocation='template-string';else if(ts.isThrowStatement(n))allocation='exception';
      else if(ts.isBinaryExpression(n)&&n.operatorToken.kind===ts.SyntaxKind.PlusToken&&(ts.isStringLiteralLike(n.left)||ts.isStringLiteralLike(n.right)))allocation='string-concat';
      if(allocation)item.allocations.push({kind:allocation,...position(n,item)});
      if(ts.isCallExpression(n)){const expression=n.expression,text=expression.getText(item.ast);
        const name=ts.isPropertyAccessExpression(expression)?expression.name.text:ts.isIdentifier(expression)?expression.text:'';
        const receiver=ts.isPropertyAccessExpression(expression)?expression.expression:null;
        const nativeArray=receiver&&ts.isPropertyAccessExpression(receiver)&&typed.has(receiver.name.text);
        if(pure.has(text)||((name==='set'||name==='fill')&&nativeArray)){}
        else if(byName.has(name)){for(const target of byName.get(name)){item.edges.push(target.key);queue.push(target);}}
        else item.unresolved.push(position(n,item));
      }ts.forEachChild(n,scan);
    }scan(item.node.body);
  }
  const registry=JSON.parse(fs.readFileSync(path.join(__dirname,'fixtures/p011_particle_allocation_registry_v1.json'),'utf8'));
  const legacy=registry.sites.map(s=>{const owners=nodes.filter(n=>n.file===s.file&&n.name===s.owner);return {...s,
    Reachability:owners.some(n=>reachable.has(n.key))?'HOT_REACHABLE':'COLD_DIAGNOSTIC_COMPAT',
    classification:owners.some(n=>reachable.has(n.key))?'STILL_REACHABLE':'MIGRATED_TO_COLD',ownerNodes:owners.map(n=>n.key)};});
  const visited=nodes.filter(n=>reachable.has(n.key)),allocations=visited.flatMap(n=>n.allocations.map(a=>({owner:n.key,...a}))),
    unresolved=visited.flatMap(n=>n.unresolved.map(a=>({owner:n.key,...a})));
  const complete=registry.count===60&&legacy.length===60&&new Set(legacy.map(s=>s.id)).size===60&&
    legacy.filter(s=>s.id.startsWith('F')).length===31&&legacy.filter(s=>s.id.startsWith('P')).length===29&&legacy.every(s=>s.ownerNodes.length>0);
  const remaining=legacy.filter(s=>s.Reachability==='HOT_REACHABLE').length+allocations.length+unresolved.length;
  return {status:complete&&remaining===0?'ZERO_ALLOCATION':'NOT_ZERO_ALLOCATION',registryComplete:complete,legacySiteCount:legacy.length,
    legacyRegistry:legacy,remainingSiteCount:remaining,hotAllocationSites:allocations,unresolvedCalls:unresolved,
    roots:visited.filter(n=>roots.has(n.name)).map(n=>n.key),reachableNodes:visited.map(n=>({key:n.key,edges:[...new Set(n.edges)]})),
    nativeAllowlist:[...pure,'TypedArray.set (declared existing storage)','TypedArray.fill (declared existing storage)']};
}
// No-argument API/CLI remains the original conservative graph and exit=2.
function audit(options){return options===undefined?conservativeAudit():require('./tools/p011_hot_context_graph.cjs').audit(options);}
module.exports={audit};if(require.main===module){
  const profileIndex=process.argv.indexOf('--profile'),sourceIndex=process.argv.indexOf('--source-root');
  const r=profileIndex<0?audit():audit({profile:process.argv[profileIndex+1],sourceRoot:sourceIndex<0?undefined:process.argv[sourceIndex+1]});
  console.log(JSON.stringify(r,null,2));if(r.status!=='ZERO_ALLOCATION')process.exitCode=2;
}
