'use strict';
// T168: finite abstract execution. Unsupported operations are evidence, never exemptions.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),cp=require('node:child_process');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const ROOT_NAMES=['beginStepHot','checkpointHot','collectParticlePlanInto','compareParticlePlansHot','applyParticlePlanInto',
 'preparePatchHot','publishPatchHot','restorePatchHot','discardPatchHot','finishPatchHot','rollbackStepHot','finishStepHot',
 'copyStateToHot','guardAdmitInto','reserveInto','refundInto','closeInto'];
const ORIGINAL_FILES=['P011ParticleField.ets','P011ParticlePrototype.ets','P011ParticleRefundAdapter.ets',
 'P011ParticleHot.ets','P011ParticleHotBudget.ets','P011ParticleHotPrototype.ets','P011ParticleHotEvidence.ets'];
const N={tag:'NULL'},U={tag:'UNKNOWN'},T={tag:'TRUE'},F={tag:'FALSE'},D={tag:'UNDEFINED'};
const hash=x=>crypto.createHash('sha256').update(x).digest('hex');
const brief=v=>v.tag+(v.id?':'+v.id:'')+(v.type?':type='+v.type:'')+(v.proven?':ENTRY_PROVEN':'')+(v.dynamicClass?':class='+v.dynamicClass:'');
const same=(a,b)=>brief(a)===brief(b);
const join=(a,b)=>{
 if(same(a,b))return a;
 // Preserve an object's identity through nullable/narrowed branch joins.
 // Losing it to a scalar UNKNOWN used to lose derived-this on rollback.
 if(a.tag==='REF'&&b.tag==='REF'&&a.id===b.id)return {...a,
  type:[...new Set([...(a.type||'').split(' | '),...(b.type||'').split(' | ')])].sort().join(' | '),proven:Boolean(a.proven&&b.proven)};
 if(a.tag==='REF'&&['NULL','UNDEFINED'].includes(b.tag))return {...a,type:[...new Set([...(a.type||'').split(' | '),b.tag==='NULL'?'null':'undefined'])].sort().join(' | ')};
 if(b.tag==='REF'&&['NULL','UNDEFINED'].includes(a.tag))return join(b,a);
 return U;
};
const truth=v=>v.tag==='NULL'||v.tag==='FALSE'||v.tag==='UNDEFINED'?false:v.tag==='TRUE'?true:v.tag==='REF'&&!/null|undefined/.test(v.type||'')?true:null;
function typeName(checker,node){try{return checker.typeToString(checker.getTypeAtLocation(node));}catch{return 'unknown';}}
function model(sourceRoot){
 const directory=path.resolve(sourceRoot),sources=new Map(),nodes=[],classes=[],objects=[],interfaces=new Map();
 const all=fs.readdirSync(directory).filter(f=>f.endsWith('.ets'));
 const virtual=all.map(f=>path.join(directory,f.replace(/\.ets$/,'.ts')));
 const host=ts.createCompilerHost({target:ts.ScriptTarget.ES2020}),sourceBase=path.dirname(directory);
 const read=host.readFile.bind(host),exists=host.fileExists.bind(host);
 host.fileExists=f=>path.resolve(f).startsWith(sourceBase+path.sep)&&f.endsWith('.ts')?exists(f.slice(0,-3)+'.ets'):exists(f);
 host.readFile=f=>path.resolve(f).startsWith(sourceBase+path.sep)&&f.endsWith('.ts')?read(f.slice(0,-3)+'.ets'):read(f);
 host.getSourceFile=(fileName,languageVersion)=>{const source=host.readFile(fileName);return source===undefined?undefined:ts.createSourceFile(fileName,source,languageVersion,true,ts.ScriptKind.TS);};
 const program=ts.createProgram(virtual,{target:ts.ScriptTarget.ES2020,strictNullChecks:true,noEmit:true,skipLibCheck:true},host),checker=program.getTypeChecker();
 function fileId(sf){return path.relative(directory,sf.fileName).replaceAll('\\','/').replace(/\.ts$/,'.ets');}
 function keyOf(n){const sf=n.getSourceFile(),owner=ts.isClassDeclaration(n.parent)||ts.isInterfaceDeclaration(n.parent)?n.parent.name.text:ts.isObjectLiteralExpression(n.parent)?'object@'+n.parent.pos:'';
  return fileId(sf)+'::'+(owner?owner+'.':'')+(n.name?.getText(sf)||((ts.isArrowFunction(n)||ts.isFunctionExpression(n))?'closure@'+n.getStart():'<anonymous>'));}
 for(const sf of program.getSourceFiles())if(path.resolve(sf.fileName).startsWith(sourceBase+path.sep)){
  const file=fileId(sf);sources.set(file,sf);
  function walk(n){
   if(ts.isClassDeclaration(n))classes.push(n);
   if(ts.isObjectLiteralExpression(n))objects.push(n);
   if(ts.isInterfaceDeclaration(n))interfaces.set(n.name.text,n);
   if((ts.isFunctionDeclaration(n)||ts.isMethodDeclaration(n)||ts.isGetAccessorDeclaration(n)||ts.isConstructorDeclaration(n)||ts.isArrowFunction(n)||ts.isFunctionExpression(n))&&n.body)
    nodes.push({key:keyOf(n),node:n,file,owner:n.parent.name?.text||'',name:n.name?.getText(sf)||'<constructor>'});
   ts.forEachChild(n,walk);
  }walk(sf);
 }
 const byNode=new Map(nodes.map(n=>[n.node,n]));
 function implementations(decl){
  const owner=decl.parent;if(!ts.isInterfaceDeclaration(owner))return [];
  const iface=checker.getTypeAtLocation(owner);
  return [...classes,...objects].filter(c=>checker.isTypeAssignableTo(checker.getTypeAtLocation(c),iface))
   .flatMap(c=>{const prop=checker.getTypeAtLocation(c).getProperty(decl.name.getText());return prop?.declarations?.filter(d=>d.body)||[];});
 }
 function declarations(node){let sym=checker.getSymbolAtLocation(node);if(sym?.flags&ts.SymbolFlags.Alias)sym=checker.getAliasedSymbol(sym);
  return sym?.declarations||[];}
 function descends(c,name,seen=new Set()){
  if(!c||seen.has(c))return false;if(c.name?.text===name)return true;seen.add(c);
  return (c.heritageClauses||[]).filter(h=>h.token===ts.SyntaxKind.ExtendsKeyword).some(h=>h.types.some(t=>descends(classes.find(x=>x.name?.text===t.expression.getText()),name,seen)));
 }
 function targets(node,receiver=null,superCall=false){
  const staticTargets=declarations(node).flatMap(d=>d.body?[d]:ts.isMethodSignature(d)?implementations(d):[]);
  if(!superCall&&receiver?.dynamicClass&&node.text){const cls=classes.find(c=>c.name.text===receiver.dynamicClass);
   const property=cls?checker.getTypeAtLocation(cls).getProperty(node.text):null;
   if(property?.declarations?.some(d=>d.body))return property.declarations.filter(d=>d.body).map(d=>byNode.get(d)).filter(Boolean);
  }
  const virtual=[];
  if(!superCall&&receiver&&!receiver.dynamicClass)for(const d of staticTargets)if(ts.isMethodDeclaration(d)&&ts.isClassDeclaration(d.parent)){
   for(const cls of classes.filter(c=>descends(c,d.parent.name.text))){
    const property=checker.getTypeAtLocation(cls).getProperty(d.name.getText());virtual.push(...(property?.declarations||[]).filter(p=>p.body));
   }
  }
  return [...new Set([...staticTargets,...virtual])].map(d=>byNode.get(d)).filter(Boolean);
 }
 function pos(n){const sf=n.getSourceFile(),p=sf.getLineAndCharacterOfPosition(n.getStart());return {file:fileId(sf),line:p.line+1,start:n.getStart(),end:n.end,text:n.getText().slice(0,240)};}
 return {directory,sources,nodes,classes,interfaces,checker,keyOf,targets,declarations,pos,byNode,program,descends};
}
function factInventory(m){
 // Pin only configuration writes and their enclosing context, not entire methods or files.
 // Any extra/reversed/rebound write invalidates the entry proof and fails closed.
 const fields=new Set(['poolProgress','diagnostic','fieldColumnEvidenceV2','diagnosticEnabled']),items=[];
 for(const [file,sf] of m.sources){
  function add(n,kind){let parent=n.parent;while(parent&&!ts.isFunctionLike(parent)&&!ts.isClassDeclaration(parent))parent=parent.parent;
   const controlFlowSha256=parent?hash(parent.getText()):null;
   items.push({file,owner:parent?m.keyOf(parent):'',kind,text:n.getText().replace(/\s+/g,' '),controlFlowSha256,...m.pos(n)});}
  function walk(n){
   if(ts.isPropertyDeclaration(n)&&fields.has(n.name.getText()))add(n,'initializer');
   if(ts.isBinaryExpression(n)&&n.operatorToken.kind>=ts.SyntaxKind.FirstAssignment&&n.operatorToken.kind<=ts.SyntaxKind.LastAssignment){
    if(ts.isPropertyAccessExpression(n.left)&&fields.has(n.left.name.text))add(n,'write');
    if(ts.isElementAccessExpression(n.left)&&n.left.argumentExpression&&ts.isStringLiteralLike(n.left.argumentExpression)&&fields.has(n.left.argumentExpression.text))add(n,'computed-write');
   }
   ts.forEachChild(n,walk);
  }walk(sf);
 }
 return items.sort((a,b)=>(a.file+a.start).localeCompare(b.file+b.start));
}
function inventoryIdentity(items){return items.map(({file,owner,kind,text,controlFlowSha256})=>({file,owner,kind,text,controlFlowSha256})).sort((a,b)=>JSON.stringify(a).localeCompare(JSON.stringify(b)));}
function audit(options={}){
 const repo=path.resolve(__dirname,'../..'),sourceRoot=options.sourceRoot||path.join(repo,'entry/src/main/ets/physics');
 const profile=options.profile||'OFF_ALL',m=model(sourceRoot),checker=m.checker;
 const issues=[],allocations=new Map(),allocationEffects=new Map(),unresolved=new Map(),pruned=new Map(),contexts=new Map(),queue=[],edges=new Map(),returns=new Map(),outputs=new Map(),active=new Set(),storageProofs=new Map(),havocProofs=new Map();
 // Monotone may-points-to graph: union across branches/contexts is conservative.
 const objectValues=new Map(),points=new Map(),collectionValues=new Map(),closureValues=new Map();
 function remember(v){if(v?.tag==='REF'&&v.id){objectValues.set(v.id,v);if(!points.has(v.id))points.set(v.id,new Set());}return v;}
 function link(from,to){if(from?.tag!=='REF'||to?.tag!=='REF')return;remember(from);remember(to);points.get(from.id).add(to.id);}
 function reachable(values){const out=new Set(),work=values.filter(v=>v?.tag==='REF').map(v=>v.id);while(work.length){const id=work.pop();if(out.has(id))continue;out.add(id);for(const next of points.get(id)||[])work.push(next);}return out;}
 function tainted(s,v){return s.heap.has('!escaped')||s.heap.has('!taint:'+v.id);}
 function builtinConstructor(n,names){
  return ts.isIdentifier(n)&&names.includes(n.text)&&m.declarations(n).length>0&&m.declarations(n).every(d=>d.getSourceFile().isDeclarationFile);
 }
 function effect(item,n,kind,ctx){record(allocationEffects,item,n,{kind},ctx);}
 const usedFiles=new Set(ORIGINAL_FILES),facts=factInventory(m);
 const contractPath=options.factContractPath||path.join(repo,'test/fixtures/t168_profile_write_contract.json');
 let contract;try{contract=JSON.parse(fs.readFileSync(contractPath,'utf8'));}catch(e){issues.push({reason:'FACT_CONTRACT_MISSING',detail:e.message});}
 const factsMatch=contract&&JSON.stringify(inventoryIdentity(facts))===JSON.stringify(contract.writes);
 if(!factsMatch)issues.push({reason:'CONFIGURATION_WRITE_INVENTORY_CHANGED',expected:contract?.writes,actual:inventoryIdentity(facts)});
 const defaults=[['P011ParticleHot.ets','P011ParticleHotWorkspace','poolProgress','null'],['P011ParticleHot.ets','P011ParticleHotWorkspace','fieldColumnEvidenceV2','null'],
  ['P011ParticleHot.ets','P011ParticleHotWorkspace','diagnosticEnabled','false'],['P011MergeExactIndex.ets','P011MergeExactIndex','diagnostic','null']];
 const defaultProof=defaults.map(([file,owner,field,expected])=>{const c=m.classes.find(c=>c.name.text===owner),n=c?.members.find(n=>n.name?.getText()===field);
  return {file,owner,field,expected,actual:n?.initializer?.getText(),verified:n?.initializer?.getText()===expected,source:n?m.pos(n):null};});
 if(defaultProof.some(p=>!p.verified))issues.push({reason:'DEFAULT_FACT_UNPROVEN'});
 const coldNames=['configurePoolProgressDiagnostic','setPoolProgressDiagnosticEnabled','setTiltPreviewTraceSink'];
 const cold=m.nodes.filter(n=>n.file==='WaterSandField.ets'&&coldNames.includes(n.name)).map(n=>({key:n.key,source:m.pos(n.node),sha256:hash(n.node.getText())}));
 // The approved cold functions are source-pinned as well: changing a guard, early return,
 // alias, or call while retaining identical assignment text must invalidate this proof.
 if(contract&&JSON.stringify(cold.map(x=>({key:x.key,sha256:x.sha256})))!==JSON.stringify(contract.cold))issues.push({reason:'COLD_FACT_CONTROL_FLOW_CHANGED'});
 const validProfiles=['OFF_ALL','PROGRESS_OFF_EVIDENCE_ON','PROGRESS_ON','UNKNOWN'];
 if(!validProfiles.includes(profile))issues.push({reason:'UNKNOWN_PROFILE',profile});
 const factsValid=Boolean(factsMatch&&defaultProof.every(p=>p.verified)&&cold.length===3&&!issues.length);
 const profileSlots=profile==='OFF_ALL'?{poolProgress:N,fieldColumnEvidenceV2:N,diagnosticEnabled:F,diagnostic:N}:
  profile==='PROGRESS_OFF_EVIDENCE_ON'?{poolProgress:N,diagnostic:N,diagnosticEnabled:F,fieldColumnEvidenceV2:'ON'}:
  profile==='PROGRESS_ON'?{poolProgress:'ON',diagnostic:'ON',diagnosticEnabled:T,fieldColumnEvidenceV2:U}:{};
 function value(node,id,proven=false){const type=typeName(checker,node);
  if(type==='null')return N;if(type==='undefined'||type==='void')return D;
  if(type==='true')return T;if(type==='false')return F;
  if(type==='boolean'||type==='number'||type==='string'||type==='any'||type==='unknown'||type==='never'||/^[\d'"-]/.test(type))return {...U,type};
  return remember({tag:'REF',id,type,proven});
 }
 function typed(v){return /^(?:Float64Array|Float32Array|Uint8Array|Uint16Array|Uint32Array|Int8Array|Int16Array|Int32Array)(?:<.*>)?$/.test(v.type||'');}
 const fixedArrays=new Map();
 function fixedArray(node,base){
  const declaration=m.declarations(node.name||node).find(d=>ts.isPropertyDeclaration(d));
  if(!declaration||!ts.isClassDeclaration(declaration.parent)||!(ts.getCombinedModifierFlags(declaration)&ts.ModifierFlags.Readonly))return null;
  const owner=declaration.parent.name.text,name=declaration.name.getText();
  if(!(base.type||'').split(' | ').includes(owner))return null;
  const key=m.keyOf(declaration);if(fixedArrays.has(key))return fixedArrays.get(key);
  const cls=declaration.parent,ctor=cls.members.find(ts.isConstructorDeclaration),initializers=[];
  if(declaration.initializer)initializers.push(declaration.initializer);
  let unsafe=false;
  function constructorScan(n){
   if(ts.isBinaryExpression(n)&&ts.isPropertyAccessExpression(n.left)&&n.left.expression.kind===ts.SyntaxKind.ThisKeyword&&n.left.name.text===name){
    if(n.operatorToken.kind===ts.SyntaxKind.EqualsToken)initializers.push(n.right);else unsafe=true;
   }
   if(ts.isPropertyAccessExpression(n)&&n.expression.kind===ts.SyntaxKind.ThisKeyword&&n.name.text===name){
    const p=n.parent;
    if(!(ts.isBinaryExpression(p)&&p.left===n)||p.operatorToken.kind!==ts.SyntaxKind.EqualsToken){
     if(!(ts.isElementAccessExpression(p)&&p.expression===n)&&!(ts.isPropertyAccessExpression(p)&&p.expression===n&&p.name.text==='length'))unsafe=true;
    }
   }
   ts.forEachChild(n,constructorScan);
  }if(ctor?.body)constructorScan(ctor.body);
  const init=initializers[0];
  const info=!unsafe&&initializers.length===1&&ts.isArrayLiteralExpression(init)&&init.elements.every(e=>!ts.isSpreadElement(e)&&e.kind!==ts.SyntaxKind.OmittedExpression)?
   {owner,field:name,length:init.elements.length,initializer:m.pos(init),declaration:m.pos(declaration),sha256:hash(init.getText()),kind:'COLD_OWNED_DENSE_DATA_ARRAY'}:null;
  fixedArrays.set(key,info);return info;
 }
 function indexBounds(node){
  if(ts.isParenthesizedExpression(node))return indexBounds(node.expression);
  if(ts.isNumericLiteral(node)){const n=Number(node.text);return Number.isSafeInteger(n)?{min:n,max:n,witness:m.pos(node)}:null;}
  if(!ts.isIdentifier(node))return null;
  const symbol=checker.getSymbolAtLocation(node);
  for(let loop=node.parent;loop;loop=loop.parent)if(ts.isForStatement(loop)&&loop.initializer&&ts.isVariableDeclarationList(loop.initializer)&&loop.initializer.declarations.length===1){
   const d=loop.initializer.declarations[0],condition=loop.condition,step=loop.incrementor;
   if(!ts.isIdentifier(d.name)||checker.getSymbolAtLocation(d.name)!==symbol||!d.initializer||!ts.isNumericLiteral(d.initializer)||!condition||!ts.isBinaryExpression(condition)||!ts.isIdentifier(condition.left)||checker.getSymbolAtLocation(condition.left)!==symbol||!ts.isNumericLiteral(condition.right))continue;
   const start=Number(d.initializer.text),end=Number(condition.right.text),op=condition.operatorToken.kind;
   const forward=step&&(ts.isPostfixUnaryExpression(step)||ts.isPrefixUnaryExpression(step))&&step.operator===ts.SyntaxKind.PlusPlusToken&&ts.isIdentifier(step.operand)&&checker.getSymbolAtLocation(step.operand)===symbol;
   if(!forward||!Number.isSafeInteger(start)||!Number.isSafeInteger(end)||![ts.SyntaxKind.LessThanToken,ts.SyntaxKind.LessThanEqualsToken].includes(op))continue;
   let modified=false;
   function scan(n){if(ts.isBinaryExpression(n)&&n.operatorToken.kind>=ts.SyntaxKind.FirstAssignment&&n.operatorToken.kind<=ts.SyntaxKind.LastAssignment&&ts.isIdentifier(n.left)&&checker.getSymbolAtLocation(n.left)===symbol)modified=true;
    if((ts.isPostfixUnaryExpression(n)||ts.isPrefixUnaryExpression(n))&&ts.isIdentifier(n.operand)&&checker.getSymbolAtLocation(n.operand)===symbol)modified=true;
    ts.forEachChild(n,scan);
   }scan(loop.statement);
   if(!modified)return {min:start,max:end-(op===ts.SyntaxKind.LessThanToken?1:0),witness:m.pos(loop),integerStep:1};
  }return null;
 }
 function place(item,node){return {owner:item.key,...m.pos(node)};}
 function record(map,item,node,data,ctx){const p=place(item,node),k=p.owner+':'+p.start+':'+p.end+':'+(data.reason||data.kind||'');
  if(!map.has(k))map.set(k,{...p,...data,contextKeys:[],witness:ctx.trace});
  const out=map.get(k);if(!out.contextKeys.includes(ctx.key))out.contextKeys.push(ctx.key);
 }
 function unknown(item,node,reason,ctx){record(unresolved,item,node,{reason},ctx);}
 function prune(item,node,reason,v,ctx){record(pruned,item,node,{reason,abstractValue:brief(v),source:v.origin||v.id||'derived',argumentsSkipped:ts.isCallExpression(node)?node.arguments.map(a=>a.getText()):[]},ctx);}
 function clone(s){return {env:new Map(s.env),heap:new Map(s.heap),completion:s.completion,result:s.result};}
 function merge(a,b){const out=clone(a);for(const k of new Set([...a.env.keys(),...b.env.keys()]))out.env.set(k,join(a.env.get(k)||U,b.env.get(k)||U));
  for(const k of new Set([...a.heap.keys(),...b.heap.keys()]))out.heap.set(k,join(a.heap.get(k)||U,b.heap.get(k)||U));
  out.completion=a.completion===b.completion?a.completion:'normal';out.result=join(a.result||U,b.result||U);return out;}
 function stateId(s){return JSON.stringify([...s.env,...s.heap].map(([k,v])=>[k,brief(v)]).sort());}
 function property(base,name,node,s,item,ctx){
  const k=base.id+'.'+name;if(s.heap.has(k))return s.heap.get(k);
  const isWorkspace=/^P011ParticleHotWorkspace(?: \| null)?$/.test(base.type||'');
  const isIndex=base.type==='P011MergeExactIndex';
  if(!tainted(s,base)&&factsValid&&base.proven&&((isWorkspace&&['poolProgress','fieldColumnEvidenceV2','diagnosticEnabled'].includes(name))||(isIndex&&name==='diagnostic'))){
   const entry=profileSlots[name];if(entry==='ON')return {...value(node,k,true),origin:'profile:'+profile+':'+k};
   if(entry)return {...entry,origin:'profile:'+profile+':'+k};
  }
  // Getters execute; do not treat a property-access as mere storage.
  const ds=m.declarations(node.name||node);const getters=ds.filter(d=>ts.isGetAccessorDeclaration(d)&&d.body);
  if(getters.length){for(const n of getters)enqueue(m.byNode.get(n),[],base,s,item,ctx,node);return U;}
  const result={...value(node,k,base.proven&&name==='mergeIndex'),origin:'property:'+k};
  const array=fixedArray(node,base);if(array){result.fixedArray=array;result.builtin='Array';}
  const decl=ds.find(d=>ts.isPropertyDeclaration(d));
  if(decl?.initializer&&ts.isNewExpression(decl.initializer)&&builtinConstructor(decl.initializer.expression,['Map','Set'])&&
    (ts.getCombinedModifierFlags(decl)&ts.ModifierFlags.Readonly)){
   result.builtin=decl.initializer.expression.getText();result.coldContentsUnknown=true;
  }
  link(base,result);return remember(result);
 }
 function invalidate(s,item,node,ctx,values){
  const escaped=values.filter(v=>v.tag==='REF');
  if(escaped.length){unknown(item,node,'UNKNOWN_CALL_OR_ALIAS_ESCAPE',ctx);
   const affected=reachable(escaped),opaque=escaped.some(v=>v.coldContentsUnknown||(!v.local&&!v.proven&&!v.fixedArray&&!v.builtin&&!v.callable));
   // Opaque roots may already carry hidden/shared aliases. Local objects only
   // expose their transitive contents, never an unrelated Workspace by name.
   if(opaque){for(const [k]of s.heap)s.heap.set(k,U);s.heap.set('!escaped',U);}
   else for(const id of affected){s.heap.set('!taint:'+id,U);for(const k of s.heap.keys())if(k.startsWith(id+'.'))s.heap.set(k,U);}
   record(havocProofs,item,node,{kind:opaque?'OPAQUE_SHARED_HAVOC':'REACHABLE_OBJECT_HAVOC',affected:[...affected],arguments:escaped.map(brief),preservedUnreachableEntryObjects:!opaque},ctx);
  }
 }
 function enqueue(target,args,self,s,item,parent,node){
  if(!target)return U;
  const paramValues=target.node.parameters.map((p,i)=>args[i]||value(p,`${parent.key}:${p.name.getText()}:unknown`));
  const inherited=[...s.heap];
  const signature=target.key+'|this='+brief(self)+'|'+paramValues.map(brief).join(',')+'|slots='+inherited.map(([k,v])=>k+'='+brief(v)).sort().join(',');
  const key=target.key+'#'+hash(signature).slice(0,20);
  if(contexts.size>2500){unknown(item,node,'CONTEXT_LIMIT_FAIL_CLOSED',parent);return U;}
  if(!contexts.has(key)){
   const trace=[...parent.trace,{...m.pos(node),caller:item.key,callee:target.key}];
   const closure=closureValues.get(self?.id);
   const c={key,item:target,args:paramValues,self:closure?.lexicalThis||self,captures:closure?.env,heap:inherited,trace};contexts.set(key,c);
  }
  edges.set(parent.key+'>'+key+':'+node.pos,{from:parent.key,to:key,site:place(item,node)});usedFiles.add(target.file);
  if(active.has(key)){unknown(item,node,'RECURSIVE_SUMMARY_NOT_PROVEN',parent);invalidate(s,item,node,parent,[self,...args]);return U;}
  if(!outputs.has(key))execute(contexts.get(key));
  const output=outputs.get(key);if(output)for(const [slot,v]of output.heap)s.heap.set(slot,v);
  return returns.get(key)||U;
 }
 function assign(left,v,s,item,ctx){
  if(ts.isIdentifier(left)){
   s.env.set(left.text,v);
   for(const [id,c]of closureValues)if(c.env.has(left.text)){
    c.env.set(left.text,join(c.env.get(left.text),v));link(objectValues.get(id),v);
    unknown(item,left,'CAPTURED_BINDING_REASSIGNMENT_REQUIRES_LIVE_CELL',ctx);
   }return;
  }
  if(ts.isPropertyAccessExpression(left)){
   const base=expr(left.expression,s,item,ctx),name=left.name.text;
   if(name==='prototype'||left.expression.getText().includes('.prototype')){unknown(item,left,'BUILTIN_PROTOTYPE_MUTATION',ctx);s.heap.set('!escaped',U);}
   if(name==='length'&&base.fixedArray)unknown(item,left,'FIXED_ARRAY_LENGTH_MUTATION',ctx);
   if(['poolProgress','diagnosticEnabled','fieldColumnEvidenceV2','diagnostic'].includes(name))unknown(item,left,'HOT_CONFIGURATION_MUTATION',ctx);
   if(base.tag==='REF'){
    const key=base.id+'.'+name;if(v.tag!=='UNKNOWN'||s.heap.has(key))s.heap.set(key,v);link(base,v);
   }else unknown(item,left,'UNKNOWN_ASSIGNMENT_RECEIVER',ctx);
   return;
  }
  if(ts.isElementAccessExpression(left)){
   const base=expr(left.expression,s,item,ctx);if(left.argumentExpression)expr(left.argumentExpression,s,item,ctx);
   if(!typed(base)){
    const bounds=left.argumentExpression?indexBounds(left.argumentExpression):null;
    if(base.fixedArray&&bounds&&bounds.min>=0&&bounds.max<base.fixedArray.length&&!tainted(s,base))record(storageProofs,item,left,{kind:'BOUNDED_EXISTING_ARRAY_ELEMENT_OVERWRITE',ownerStorage:base.fixedArray,indexBounds:bounds,valueEffect:'REFERENCE_OR_PRIMITIVE_SLOT_REPLACEMENT_NO_LENGTH_CHANGE'},ctx);
    else unknown(item,left,'COMPUTED_WRITE_ALIAS_NOT_PROVEN_FIXED_STORAGE',ctx);
   }link(base,v);return;
  }
  unknown(item,left,'UNSUPPORTED_ASSIGNMENT_TARGET',ctx);
 }
 function refine(n,wanted,s,item,ctx){
  if(ts.isParenthesizedExpression(n))return refine(n.expression,wanted,s,item,ctx);
  if(ts.isPrefixUnaryExpression(n)&&n.operator===ts.SyntaxKind.ExclamationToken)return refine(n.operand,!wanted,s,item,ctx);
  if(!ts.isBinaryExpression(n))return;
  const op=n.operatorToken.kind;
  if(op===ts.SyntaxKind.BarBarToken&&!wanted||op===ts.SyntaxKind.AmpersandAmpersandToken&&wanted){refine(n.left,wanted,s,item,ctx);refine(n.right,wanted,s,item,ctx);return;}
  const equal=op===ts.SyntaxKind.EqualsEqualsEqualsToken||op===ts.SyntaxKind.EqualsEqualsToken;
  const notEqual=op===ts.SyntaxKind.ExclamationEqualsEqualsToken||op===ts.SyntaxKind.ExclamationEqualsToken;
  if(!equal&&!notEqual)return;
  const a=expr(n.left,s,item,ctx),b=expr(n.right,s,item,ctx),sameArm=equal?wanted:!wanted;
  function store(target,v){
   if(ts.isIdentifier(target))s.env.set(target.text,v);
   else if(ts.isPropertyAccessExpression(target)){
    const receiver=expr(target.expression,s,item,ctx);if(receiver.tag==='REF')s.heap.set(receiver.id+'.'+target.name.text,v);
   }
  }
  if(sameArm){
   if(a.proven&&!b.proven)store(n.right,a);else if(b.proven&&!a.proven)store(n.left,b);
   else if(['NULL','TRUE','FALSE','UNDEFINED'].includes(a.tag))store(n.right,a);
   else if(['NULL','TRUE','FALSE','UNDEFINED'].includes(b.tag))store(n.left,b);
  }else{
   // Non-null narrowing does not imply OFF or a particular instance.
   if(['NULL','UNDEFINED'].includes(b.tag)&&a.tag==='REF')store(n.left,{...a,type:(a.type||'').replace(/ \| null| \| undefined/g,'')});
   if(['NULL','UNDEFINED'].includes(a.tag)&&b.tag==='REF')store(n.right,{...b,type:(b.type||'').replace(/ \| null| \| undefined/g,'')});
  }
 }
 function expr(n,s,item,ctx){
  if(!n)return D;
  if(ts.isParenthesizedExpression(n)||ts.isAsExpression(n)||ts.isTypeAssertionExpression(n)||ts.isNonNullExpression(n)||ts.isPartiallyEmittedExpression(n))return expr(n.expression,s,item,ctx);
  if(n.kind===ts.SyntaxKind.NullKeyword)return N;if(n.kind===ts.SyntaxKind.TrueKeyword)return T;if(n.kind===ts.SyntaxKind.FalseKeyword)return F;
  if(n.kind===ts.SyntaxKind.ThisKeyword)return ctx.self;
  if(n.kind===ts.SyntaxKind.SuperKeyword)return ctx.self;
  if(ts.isNumericLiteral(n)||ts.isStringLiteralLike(n)||n.kind===ts.SyntaxKind.NoSubstitutionTemplateLiteral)return {...U,type:ts.isNumericLiteral(n)?'number':'string'};
  if(ts.isIdentifier(n)){
   if(n.text==='undefined')return D;if(s.env.has(n.text))return s.env.get(n.text);
   const ds=m.declarations(n),d=ds.find(d=>ts.isVariableDeclaration(d));
   if(d?.initializer&&(ts.isNumericLiteral(d.initializer)||ts.isStringLiteralLike(d.initializer)))return {...U,type:ts.isNumericLiteral(d.initializer)?'number':'string'};
   return value(n,'global:'+n.text);
  }
  if(ts.isPropertyAccessExpression(n)){
   const base=expr(n.expression,s,item,ctx);
   if((n.questionDotToken||(n.flags&ts.NodeFlags.OptionalChain))&&(base.tag==='NULL'||base.tag==='UNDEFINED')){prune(item,n,'OPTIONAL_NULL_ACCESS',base,ctx);return D;}
   if(tainted(s,base))return value(n,'escaped:'+n.getText());
   return property(base,n.name.text,n,s,item,ctx);
  }
  if(ts.isElementAccessExpression(n)){const base=expr(n.expression,s,item,ctx);
   if((n.questionDotToken||(n.flags&ts.NodeFlags.OptionalChain))&&(base.tag==='NULL'||base.tag==='UNDEFINED')){prune(item,n,'OPTIONAL_NULL_ELEMENT',base,ctx);return D;}
   expr(n.argumentExpression,s,item,ctx);return value(n,'element:'+n.getText());}
  if(ts.isConditionalExpression(n)){
   const c=expr(n.condition,s,item,ctx),t=truth(c);
   if(t!==null){prune(item,t?n.whenFalse:n.whenTrue,'CONDITIONAL_UNTAKEN',c,ctx);return expr(t?n.whenTrue:n.whenFalse,s,item,ctx);}
   const a=clone(s),b=clone(s),va=expr(n.whenTrue,a,item,ctx),vb=expr(n.whenFalse,b,item,ctx),merged=merge(a,b);s.env=merged.env;s.heap=merged.heap;return join(va,vb);
  }
  if(ts.isBinaryExpression(n)){
   const op=n.operatorToken.kind;
   if(op===ts.SyntaxKind.EqualsToken){const v=expr(n.right,s,item,ctx);assign(n.left,v,s,item,ctx);return v;}
   if(op>=ts.SyntaxKind.FirstCompoundAssignment&&op<=ts.SyntaxKind.LastCompoundAssignment){expr(n.left,s,item,ctx);expr(n.right,s,item,ctx);assign(n.left,U,s,item,ctx);return U;}
   const a=expr(n.left,s,item,ctx);
   if([ts.SyntaxKind.AmpersandAmpersandToken,ts.SyntaxKind.BarBarToken,ts.SyntaxKind.QuestionQuestionToken].includes(op)){
    const t=truth(a),take=op===ts.SyntaxKind.QuestionQuestionToken?(a.tag==='NULL'||a.tag==='UNDEFINED'?true:a.tag==='REF'||a.tag==='TRUE'||a.tag==='FALSE'?false:null):t===null?null:op===ts.SyntaxKind.AmpersandAmpersandToken?t:!t;
    if(take===false){prune(item,n.right,'SHORT_CIRCUIT',a,ctx);return a;}
    if(take===true)return expr(n.right,s,item,ctx);
    const b=clone(s),v=expr(n.right,b,item,ctx),merged=merge(s,b);s.env=merged.env;s.heap=merged.heap;return join(a,v);
   }
   const b=expr(n.right,s,item,ctx);
   if([ts.SyntaxKind.EqualsEqualsEqualsToken,ts.SyntaxKind.ExclamationEqualsEqualsToken,ts.SyntaxKind.EqualsEqualsToken,ts.SyntaxKind.ExclamationEqualsToken].includes(op)){
    const exact=v=>['NULL','UNDEFINED','TRUE','FALSE'].includes(v.tag);
    let eq=null;
    if(exact(a)&&exact(b))eq=same(a,b);
    else if((a.tag==='REF'&&['NULL','UNDEFINED'].includes(b.tag)&&!a.type?.includes('null')&&!a.type?.includes('undefined'))||
      (b.tag==='REF'&&['NULL','UNDEFINED'].includes(a.tag)&&!b.type?.includes('null')&&!b.type?.includes('undefined')))eq=false;
    // Nullable typed refs have unknown nullness; same abstract identity is not exact equality.
    if(eq!==null)return (op===ts.SyntaxKind.ExclamationEqualsEqualsToken||op===ts.SyntaxKind.ExclamationEqualsToken?!eq:eq)?T:F;
    return U;
   }
   if(op===ts.SyntaxKind.PlusToken&&(/string/.test(a.type||'')||/string/.test(b.type||'')))record(allocations,item,n,{kind:'string-concat'},ctx);
   if(op===ts.SyntaxKind.CommaToken)return b;
   return {...U,type:typeName(checker,n)};
  }
  if(ts.isPrefixUnaryExpression(n)){
   const v=expr(n.operand,s,item,ctx);if(n.operator===ts.SyntaxKind.ExclamationToken){const t=truth(v);return t===null?U:t?F:T;}
   if([ts.SyntaxKind.PlusPlusToken,ts.SyntaxKind.MinusMinusToken].includes(n.operator))assign(n.operand,U,s,item,ctx);return U;
  }
  if(ts.isPostfixUnaryExpression(n)){expr(n.operand,s,item,ctx);assign(n.operand,U,s,item,ctx);return U;}
  if(ts.isTypeOfExpression(n)||ts.isVoidExpression(n)){expr(n.expression,s,item,ctx);return U;}
  if(ts.isCallExpression(n)){
   const callee=n.expression;
   let receiver=null;
   if(ts.isPropertyAccessExpression(callee)){
    receiver=expr(callee.expression,s,item,ctx);
    if((callee.questionDotToken||n.questionDotToken||(n.flags&ts.NodeFlags.OptionalChain))&&(receiver.tag==='NULL'||receiver.tag==='UNDEFINED')){prune(item,n,'OPTIONAL_NULL_CALL_AND_ARGUMENTS',receiver,ctx);return D;}
    if(receiver.tag==='NULL'||receiver.tag==='UNDEFINED'){s.completion='throw';prune(item,n,'NULL_RECEIVER_THROWS_BEFORE_CALLEE',receiver,ctx);return U;}
   }else if(ts.isElementAccessExpression(callee)){expr(callee,s,item,ctx);unknown(item,n,'DYNAMIC_CALLEE',ctx);}
   const args=n.arguments.map(a=>expr(a,s,item,ctx));
   const text=callee.getText(),pure=/^(Math\.(floor|max|min|sin|cos|tan|exp|abs|imul)|Number\.(isSafeInteger|isFinite|isInteger))$/.test(text);
   // The native exemption is based on the checker receiver type, never a property name.
   if(pure)return U;
   if(receiver&&typed(receiver)&&ts.isPropertyAccessExpression(callee)&&['set','fill'].includes(callee.name.text))return U;
   if(ts.isPropertyAccessExpression(callee)&&receiver&&!tainted(s,receiver)){
    const name=callee.name.text,brand=receiver.builtin;
    if(brand==='Iterator'&&name==='next'){effect(item,n,'BUILTIN_ITERATOR_RESULT',ctx);return U;}
    // Brands come only from an actual lib declaration plus an owned new/readonly
    // initializer. Same-named user methods never reach these summaries.
    if(['Map','Set','Array'].includes(brand)){
     if((brand==='Map'&&name==='set')||(brand==='Set'&&name==='add')||(brand==='Array'&&['push','unshift','splice'].includes(name))){
      effect(item,n,brand+'_POTENTIAL_GROWTH',ctx);for(const arg of args)link(receiver,arg);
      const previous=collectionValues.get(receiver.id);for(const arg of args)collectionValues.set(receiver.id,previous?join(previous,arg):arg);
      return receiver;
     }
     if(['keys','values','entries'].includes(name)){
      effect(item,n,brand+'_ITERATOR_CREATION',ctx);const iterator=remember({tag:'REF',id:'iterator:'+item.key+':'+n.pos,type:'Iterator',local:true,builtin:'Iterator'});link(iterator,receiver);return iterator;
     }
     if(brand==='Map'&&name==='get')return collectionValues.get(receiver.id)||U;
     if(['has','delete','clear'].includes(name))return U;
    }
   }
   if(receiver&&ts.isPropertyAccessExpression(callee)&&((receiver.type==='number'&&callee.name.text==='toString')||(receiver.type==='string'&&['padStart','padEnd','toString','slice','substring','concat'].includes(callee.name.text)))){
    effect(item,n,'STRING_RESULT',ctx);return {...U,type:'string'};
   }
   const localCallable=!ts.isPropertyAccessExpression(callee)&&!ts.isElementAccessExpression(callee)?expr(callee,s,item,ctx):null;
   if(localCallable?.callable){const closure=closureValues.get(localCallable.id);if(closure&&!tainted(s,localCallable))return enqueue(closure.target,args,localCallable,s,item,ctx,n);}
   const target=m.targets(ts.isPropertyAccessExpression(callee)?callee.name:callee,receiver,ts.isPropertyAccessExpression(callee)&&callee.expression.kind===ts.SyntaxKind.SuperKeyword);
   if(!target.length){unknown(item,n,'UNRESOLVED_CALL',ctx);invalidate(s,item,n,ctx,[...(receiver?[receiver]:[]),...args]);return U;}
   let out=null;
   for(const to of target){const v=enqueue(to,args,receiver||U,s,item,ctx,n);out=out?join(out,v):v;}
   return out||U;
  }
  if(ts.isNewExpression(n)){
   record(allocations,item,n,{kind:'constructor'},ctx);expr(n.expression,s,item,ctx);const args=(n.arguments||[]).map(a=>expr(a,s,item,ctx));
   const v=remember({...value(n,'new:'+item.key+':'+n.pos),local:true,dynamicClass:ts.isIdentifier(n.expression)?n.expression.text:undefined});
   if(builtinConstructor(n.expression,['Map','Set','Array']))v.builtin=n.expression.text;
   for(const a of args)link(v,a);
   const cls=m.classes.find(c=>c.name?.text===v.dynamicClass),ctor=cls?.members.find(ts.isConstructorDeclaration);
   if(cls)for(const p of cls.members)if(ts.isPropertyDeclaration(p)&&p.initializer){
    const a=expr(p.initializer,s,item,{...ctx,self:v});link(v,a);s.heap.set(v.id+'.'+p.name.getText(),a);
   }
   if(ctor?.body)enqueue(m.byNode.get(ctor),args,v,s,item,ctx,n);
   return v;
  }
  if(ts.isObjectLiteralExpression(n)){
   record(allocations,item,n,{kind:'object-literal'},ctx);
   const v=remember({...value(n,'literal:'+item.key+':'+n.pos),local:true});
   for(const p of n.properties){if(ts.isPropertyAssignment(p)||ts.isShorthandPropertyAssignment(p)){
    const a=expr(ts.isPropertyAssignment(p)?p.initializer:p.name,s,item,ctx);link(v,a);s.heap.set(v.id+'.'+p.name.getText(),a);
   }else if(ts.isSpreadAssignment(p)){link(v,expr(p.expression,s,item,ctx));unknown(item,p,'SPREAD_OBJECT',ctx);}else unknown(item,p,'OBJECT_METHOD_OR_ACCESSOR',ctx);}
   return v;
  }
  if(ts.isArrayLiteralExpression(n)){
   record(allocations,item,n,{kind:'array-literal'},ctx);const v=remember({...value(n,'array:'+n.pos),local:true,builtin:'Array'});for(const e of n.elements)link(v,expr(e,s,item,ctx));return v;
  }
  if(ts.isArrowFunction(n)||ts.isFunctionExpression(n)){
   record(allocations,item,n,{kind:'closure'},ctx);const v=remember({tag:'REF',id:'closure:'+item.key+':'+n.pos,type:'Function',local:true,callable:true});
   const used=new Set();let usesThis=false;function capture(x){if(ts.isIdentifier(x))used.add(x.text);if(x.kind===ts.SyntaxKind.ThisKeyword)usesThis=true;ts.forEachChild(x,capture);}capture(n.body);
   const captures=new Map([...s.env].filter(([k])=>used.has(k)));
   const previous=closureValues.get(v.id);let lexicalThis=ts.isArrowFunction(n)?ctx.self:null;
   if(previous){
    for(const key of new Set([...previous.env.keys(),...captures.keys()]))captures.set(key,join(previous.env.get(key)||U,captures.get(key)||U));
    if(lexicalThis&&previous.lexicalThis)lexicalThis=join(previous.lexicalThis,lexicalThis);
    if([...captures].some(([k,a])=>!same(a,previous.env.get(k)||U))||(lexicalThis&&previous.lexicalThis&&!same(lexicalThis,previous.lexicalThis)))unknown(item,n,'NOT_IMPLEMENTED_ANALYZER:CLOSURE_CAPTURE_CONTEXT_JOIN',ctx);
   }
   closureValues.set(v.id,{target:m.byNode.get(n),env:captures,lexicalThis});
   for(const a of captures.values())link(v,a);if(ts.isArrowFunction(n)&&usesThis)link(v,ctx.self);return v;
  }
  if(ts.isTemplateExpression(n)){record(allocations,item,n,{kind:'template-string'},ctx);for(const p of n.templateSpans)expr(p.expression,s,item,ctx);return U;}
  unknown(item,n,'UNSUPPORTED_EXPRESSION:'+ts.SyntaxKind[n.kind],ctx);ts.forEachChild(n,c=>{if(ts.isExpressionNode(c))expr(c,s,item,ctx);});return U;
 }
 function statement(n,s,item,ctx){
  if(!n||s.completion!=='normal')return s;
  if(ts.isBlock(n)){for(const x of n.statements){if(s.completion!=='normal'){prune(item,x,'AFTER_ABRUPT_COMPLETION',U,ctx);continue;}s=statement(x,s,item,ctx);}return s;}
  if(ts.isVariableStatement(n)){for(const d of n.declarationList.declarations){if(!ts.isIdentifier(d.name)){unknown(item,d,'DESTRUCTURING_NOT_MODELED',ctx);if(d.initializer)expr(d.initializer,s,item,ctx);}else s.env.set(d.name.text,d.initializer?expr(d.initializer,s,item,ctx):D);}return s;}
  if(ts.isExpressionStatement(n)){expr(n.expression,s,item,ctx);return s;}
  if(ts.isReturnStatement(n)){s.result=expr(n.expression,s,item,ctx);s.completion='return';ctx.exits.push(clone(s));return s;}
  if(ts.isThrowStatement(n)){record(allocations,item,n,{kind:'exception'},ctx);expr(n.expression,s,item,ctx);s.completion='throw';return s;}
  if(ts.isIfStatement(n)){
   const v=expr(n.expression,s,item,ctx),t=truth(v);
   if(t!==null){if(t&&n.elseStatement)prune(item,n.elseStatement,'IF_UNTAKEN',v,ctx);if(!t)prune(item,n.thenStatement,'IF_UNTAKEN',v,ctx);return statement(t?n.thenStatement:n.elseStatement,s,item,ctx);}
   const sa=clone(s),sb=clone(s);refine(n.expression,true,sa,item,ctx);refine(n.expression,false,sb,item,ctx);
   const a=statement(n.thenStatement,sa,item,ctx),b=statement(n.elseStatement,sb,item,ctx);
   if(a.completion!=='normal'&&b.completion==='normal'){ctx.abrupt.push(clone(a));return b;}
   if(b.completion!=='normal'&&a.completion==='normal'){ctx.abrupt.push(clone(b));return a;}
   return merge(a,b);
  }
  if(ts.isTryStatement(n)){
   const before=clone(s),abruptStart=ctx.abrupt.length,after=statement(n.tryBlock,s,item,ctx);
   let out=after;
   if(n.catchClause){const c=merge(before,after);c.completion='normal';if(n.catchClause.variableDeclaration)c.env.set(n.catchClause.variableDeclaration.name.getText(),U);
    out=merge(after,statement(n.catchClause.block,c,item,ctx));}
   if(n.finallyBlock){const completion=out.completion,result=out.result;
    // Include implicit throw states and early returns, including the states
    // removed from ordinary continuation by an unknown if branch.
    let finalInput=merge(before,out);for(const exit of ctx.abrupt.slice(abruptStart))finalInput=merge(finalInput,exit);
    finalInput.completion='normal';out=statement(n.finallyBlock,finalInput,item,ctx);if(out.completion==='normal'){out.completion=completion;out.result=result;}}
   return out;
  }
  if(ts.isForOfStatement(n)){
   const iterable=expr(n.expression,s,item,ctx);
   // Unknown protocols remain a named analyzer limitation. Known local class
   // protocols execute the real iterator/next/return bodies, including close.
   const cls=m.classes.find(c=>c.name?.text===iterable.dynamicClass);
   function methodOf(c,name,seen=new Set()){
    if(!c||seen.has(c))return null;seen.add(c);
    const own=c.members.find(p=>p.name?.getText()===name&&p.body);if(own)return own;
    for(const clause of c.heritageClauses||[])if(clause.token===ts.SyntaxKind.ExtendsKeyword)for(const t of clause.types){
     const parent=m.classes.find(x=>x.name?.text===t.expression.getText()),found=methodOf(parent,name,seen);if(found)return found;
    }return null;
   }
   const start=methodOf(cls,'[Symbol.iterator]');
   let iterator;
   if(start)iterator=enqueue(m.byNode.get(start),[],iterable,s,item,ctx,n);
   else if(['Map','Set','Array','Iterator'].includes(iterable.builtin)){
    effect(item,n,'BUILTIN_FOR_OF_ITERATOR_CREATION',ctx);iterator=iterable;
   }else unknown(item,n,'NOT_IMPLEMENTED_ANALYZER:ITERATOR_PROTOCOL',ctx);
   function invoke(name,state,required){
    const c=m.classes.find(c=>c.name?.text===iterator?.dynamicClass),method=methodOf(c,name);
    if(method)return enqueue(m.byNode.get(method),[],iterator,state,item,ctx,n);
    if(iterator?.builtin){if(name==='next')effect(item,n,'BUILTIN_ITERATOR_RESULT',ctx);return U;}
    if(required)unknown(item,n,'NOT_IMPLEMENTED_ANALYZER:ITERATOR_'+name.toUpperCase(),ctx);return U;
   }
   let head=clone(s),converged=false;
   for(let i=0;i<8;i++){
    const step=clone(head),next=invoke('next',step,true);
    const done=property(next,'done',n,step,item,ctx);
    if(truth(done)===true){head=merge(head,step);converged=true;break;}
    const element=property(next,'value',n,step,item,ctx);
    if(ts.isVariableDeclarationList(n.initializer)&&n.initializer.declarations.length===1&&ts.isIdentifier(n.initializer.declarations[0].name))step.env.set(n.initializer.declarations[0].name.text,element);
    else unknown(item,n,'NOT_IMPLEMENTED_ANALYZER:ITERATOR_BINDING',ctx);
    const abruptStart=ctx.abrupt.length,exitStart=ctx.exits.length,body=statement(n.statement,step,item,ctx);
    const abrupt=[...ctx.abrupt.slice(abruptStart),...ctx.exits.slice(exitStart),body];
    for(const exit of abrupt)if(['break','return','throw'].includes(exit.completion))invoke('return',exit,false);
    // A potentially throwing body also requires close; this conservative path
    // includes calls whose JavaScript exception summary is unknown.
    invoke('return',body,false);
    body.completion='normal';const widened=merge(head,body);
    if(stateId(widened)===stateId(head)){head=widened;converged=true;break;}head=widened;
   }
   if(!converged)unknown(item,n,'NOT_IMPLEMENTED_ANALYZER:ITERATOR_FIXPOINT',ctx);
   head.completion='normal';return head;
  }
  if(ts.isForStatement(n)||ts.isWhileStatement(n)||ts.isDoStatement(n)){
   if(ts.isForStatement(n)&&n.initializer){if(ts.isVariableDeclarationList(n.initializer)){
    for(const d of n.initializer.declarations)if(ts.isIdentifier(d.name))s.env.set(d.name.text,d.initializer?expr(d.initializer,s,item,ctx):D);else unknown(item,d,'LOOP_DESTRUCTURING',ctx);
   }else expr(n.initializer,s,item,ctx);}
   const condition=ts.isWhileStatement(n)||ts.isDoStatement(n)?n.expression:n.condition;
   let head=clone(s),converged=false;
   for(let i=0;i<8;i++){
    const step=clone(head),v=condition?expr(condition,step,item,ctx):T;
    if(truth(v)===false&&!ts.isDoStatement(n)){prune(item,n.statement,'LOOP_CONDITION_FALSE',v,ctx);converged=true;break;}
    const body=statement(n.statement,step,item,ctx);
    if(ts.isForStatement(n)&&n.incrementor&&['normal','continue'].includes(body.completion))expr(n.incrementor,body,item,ctx);
    body.completion='normal';const widened=merge(head,body);
    if(stateId(widened)===stateId(head)){head=widened;converged=true;break;}head=widened;
   }
   if(!converged)unknown(item,n,'LOOP_FIXPOINT_NOT_REACHED',ctx);
   head.completion='normal';return head;
  }
  if(ts.isSwitchStatement(n)){
   expr(n.expression,s,item,ctx);let out=clone(s);
   // All entry cases and fallthrough are conservatively reachable.
   for(let i=0;i<n.caseBlock.clauses.length;i++){let branch=clone(s);
    for(let j=i;j<n.caseBlock.clauses.length&&branch.completion==='normal';j++){
     const c=n.caseBlock.clauses[j];if(c.expression)expr(c.expression,branch,item,ctx);
     for(const st of c.statements)branch=statement(st,branch,item,ctx);
    }if(branch.completion==='break')branch.completion='normal';out=merge(out,branch);
   }return out;
  }
  if(ts.isBreakStatement(n)||ts.isContinueStatement(n)){if(n.label)unknown(item,n,'LABELED_CONTROL_FLOW',ctx);s.completion=ts.isBreakStatement(n)?'break':'continue';return s;}
  if(ts.isEmptyStatement(n))return s;
  unknown(item,n,'UNSUPPORTED_STATEMENT:'+ts.SyntaxKind[n.kind],ctx);
  // Traverse unsupported syntax for evidence, while its presence already prevents PASS.
  ts.forEachChild(n,c=>{if(ts.isExpressionNode(c))expr(c,s,item,ctx);else if(ts.isStatement(c))statement(c,s,item,ctx);});return s;
 }
 const rootItems=m.nodes.filter(n=>ORIGINAL_FILES.includes(n.file)&&(ROOT_NAMES.includes(n.name)||(ts.isGetAccessorDeclaration(n.node)&&n.file==='P011ParticleHot.ets')));
 if(ROOT_NAMES.some(name=>!rootItems.some(n=>n.name===name)))issues.push({reason:'ROOT_MISSING'});
 for(const item of rootItems){
  const self=ts.isMethodDeclaration(item.node)||ts.isGetAccessorDeclaration(item.node)?remember({tag:'REF',id:'root:'+item.key+':this',type:item.owner,dynamicClass:m.classes.some(c=>c.name?.text!==item.owner&&m.descends(c,item.owner))?undefined:item.owner,proven:true}):U;
  const args=item.node.parameters.map(p=>value(p,'root:'+item.key+':'+p.name.getText(),typeName(checker,p)==='P011ParticleHotWorkspace'));
  // Only a root Workspace argument carries the OFF assertion. An unrelated typed workspace
  // obtained later is not automatically null. Constructor-owned work paths need real proof.
  const ctx={key:'root:'+item.key,item,args,self,heap:[],trace:[{root:item.key}]};contexts.set(ctx.key,ctx);queue.push(ctx);
 }
 let processed=0;
 function execute(ctx){
  if(processed++>=2500){issues.push({reason:'ANALYSIS_CONTEXT_LIMIT',remaining:queue.length});return;}
  const item=ctx.item;usedFiles.add(item.file);active.add(ctx.key);
  ctx.abrupt=[];ctx.exits=[];
  const s={env:new Map(ctx.captures||[]),heap:new Map(ctx.heap),completion:'normal',result:U};
  for(let i=0;i<item.node.parameters.length;i++){const p=item.node.parameters[i];if(!ts.isIdentifier(p.name))unknown(item,p,'PARAMETER_DESTRUCTURING',ctx);else s.env.set(p.name.text,ctx.args[i]||U);}
  const result=ts.isBlock(item.node.body)?statement(item.node.body,s,item,ctx):s;
  if(!ts.isBlock(item.node.body)){result.result=expr(item.node.body,s,item,ctx);ctx.exits.push(clone(result));}
  let merged=result,returned=null;
  for(const exit of ctx.exits){merged=merge(merged,exit);returned=returned?join(returned,exit.result):exit.result;}
  returns.set(ctx.key,returned||D);outputs.set(ctx.key,merged);active.delete(ctx.key);
 }
 while(queue.length&&processed<2500){const ctx=queue.shift();if(!outputs.has(ctx.key))execute(ctx);}
 if(queue.length)issues.push({reason:'ANALYSIS_CONTEXT_LIMIT',remaining:queue.length});
 // Parse errors anywhere in the true imported closure invalidate the graph.
 const closure=new Set([...usedFiles,'WaterSandField.ets','P011MergeExactIndex.ets']);
 const pending=[...closure];while(pending.length){const f=pending.pop(),sf=m.sources.get(f);if(!sf){issues.push({reason:'MISSING_SOURCE',file:f});continue;}
  if(sf.parseDiagnostics.length)issues.push({reason:'SYNTAX_ERROR',file:f,diagnostics:sf.parseDiagnostics.map(d=>ts.flattenDiagnosticMessageText(d.messageText,' '))});
  for(const n of sf.statements)if(ts.isImportDeclaration(n)&&n.moduleSpecifier.text.startsWith('.')){
   const dep=path.relative(m.directory,path.resolve(m.directory,path.dirname(f),n.moduleSpecifier.text+'.ets')).replaceAll('\\','/');if(!closure.has(dep)){closure.add(dep);pending.push(dep);}
  }
 }
 const registryPath=options.registryPath||path.join(repo,'test/fixtures/p011_particle_allocation_registry_v1.json');let registry={sites:[],count:0};
 try{registry=JSON.parse(fs.readFileSync(registryPath,'utf8'));}catch(e){issues.push({reason:'REGISTRY_MISSING',detail:e.message});}
 const reachableOwners=new Set([...contexts.values()].map(c=>c.item.key));
 const legacy=registry.sites.map(site=>{const owners=m.nodes.filter(n=>n.file===site.file&&n.name===site.owner);
  const binding=contract?.registryBindings?.find(x=>x.id===site.id),siteEvidence=[];
  if(binding&&JSON.stringify(binding.original)===JSON.stringify(site))for(const own of owners){
   function visit(n){const match=binding.bindings.find(b=>b.owner===own.key&&hash(n.getText())===b.sha256);if(match)siteEvidence.push({...m.pos(n),owner:own.key,sha256:match.sha256});ts.forEachChild(n,visit);}visit(own.node.body);
  }
  return {...site,siteEvidence,siteIdentityVerified:siteEvidence.length>0,ownerNodes:owners.map(n=>n.key),Reachability:owners.some(n=>reachableOwners.has(n.key))?'HOT_REACHABLE':'COLD_UNREACHABLE_FROM_PRESERVED_ROOTS',
   ownerEvidence:owners.map(n=>({...m.pos(n.node),key:n.key,sha256:hash(n.node.getText())})),witness:owners.some(n=>reachableOwners.has(n.key))?[...contexts.values()].find(c=>owners.some(n=>n.key===c.item.key)).trace:null};});
 const complete=registry.count===60&&legacy.length===60&&new Set(legacy.map(s=>s.id)).size===60&&legacy.filter(s=>s.id.startsWith('F')).length===31&&legacy.filter(s=>s.id.startsWith('P')).length===29&&legacy.every(s=>s.ownerNodes.length&&s.siteIdentityVerified);
 if(!complete)issues.push({reason:'REGISTRY_INCOMPLETE'});
 let commit='UNKNOWN',dirty='UNKNOWN';try{commit=cp.execFileSync('git',['rev-parse','HEAD'],{cwd:repo,encoding:'utf8'}).trim();dirty=cp.execFileSync('git',['status','--porcelain','--untracked-files=no'],{cwd:repo,encoding:'utf8'}).trim();}catch{}
 const sourceManifest=[...closure].sort().map(file=>{const p=path.join(m.directory,file);if(!fs.existsSync(p))return {file,status:'MISSING'};
  const bytes=fs.readFileSync(p);return {file,sha256:hash(bytes),bytes:bytes.length,newline:bytes.includes(Buffer.from('\r\n'))?'CRLF':'LF',hashScope:'WORKTREE_RAW_BYTES',parsedTextSha256:hash(m.sources.get(file)?.text||'')};});
 const remaining=allocations.size+allocationEffects.size+unresolved.size+legacy.filter(s=>s.Reachability==='HOT_REACHABLE').length+issues.length;
 const unimplemented=x=>/NOT_IMPLEMENTED_ANALYZER:|NOT_MODELED|UNSUPPORTED_|OBJECT_METHOD_OR_ACCESSOR|REQUIRES_LIVE_CELL|FIXPOINT_NOT_REACHED|CONTEXT_LIMIT|RECURSIVE_SUMMARY_NOT_PROVEN/.test(x.reason);
 return {analysisRevision:'T168_CONTEXT_V2',scope:'Preserved roots, source allocations, builtin allocation effects and fail-closed proof gaps',
  profile,status:complete&&remaining===0?'ZERO_ALLOCATION':'NOT_ZERO_ALLOCATION',remainingSiteCount:remaining,productionHotPathZeroAllocation:complete&&remaining===0,
  profileFacts:{assertion:profile,valid:factsValid&&profile!=='UNKNOWN',defaults:defaultProof,coldBoundary: cold,writeInventory:facts,sourceContract:contractPath,sourceContractSha256:fs.existsSync(contractPath)?hash(fs.readFileSync(contractPath)):null,limitations:'Entry assertions only; operation must not reconfigure. Approved AST write/control-flow fingerprints plus actual defaults are pinned, not arbitrary cold-program theorem proving. Raw configuration changes/unknown escapes fail closed; does not prove runtime GC/SDK implicit allocation.'},
  analysisInputManifest:[__filename,path.join(repo,'test/probe_p011_particle_hot_graph.cjs'),contractPath,registryPath].map(file=>({file,sha256:fs.existsSync(file)?hash(fs.readFileSync(file)):null})),
  sourceManifest:{commit,dirty,sourceRoot:m.directory,files:sourceManifest},rootManifest:rootItems.map(n=>({key:n.key,...m.pos(n.node)})),
  legacyRegistry:legacy,legacySiteCount:legacy.length,registryComplete:complete,reachableContexts:[...contexts.values()].map(c=>({key:c.key,owner:c.item.key,self:brief(c.self),parameters:c.args.map(brief),witness:c.trace})),
  callEdges:[...edges.values()],prunedBranches:[...pruned.values()],storageProofs:[...storageProofs.values()],havocProofs:[...havocProofs.values()],allocationSites:[...allocations.values()],allocationEffects:[...allocationEffects.values()],unresolvedCalls:[...unresolved.values()],analysisIssues:issues,
  layers:{SOURCE_ALLOCATIONS:[...allocations.values()],ALLOCATION_EFFECTS:[...allocationEffects.values()],PROOF_GAPS:[...unresolved.values()].filter(x=>!unimplemented(x)),NOT_IMPLEMENTED_ANALYZER:[...unresolved.values()].filter(unimplemented)},
  soundnessLimits:['External/unknown callbacks fail closed','Unsupported syntax fails closed','No unbounded numeric simulation; loop finite-domain fixpoint','Interface implementation set is local closed-source set; external implementations remain outside proof scope'],
  nativeRules:['Math numeric intrinsics (fixed original set)','Number validation intrinsics (fixed original set)','TypedArray.set/fill only after receiver checker type validation']};
}
module.exports={audit,model,factInventory,inventoryIdentity};
