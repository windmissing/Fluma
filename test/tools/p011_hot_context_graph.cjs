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
const brief=v=>v.tag+(v.id?':'+v.id:'')+(v.type?':type='+v.type:'')+(v.proven?':ENTRY_PROVEN':'');
const same=(a,b)=>brief(a)===brief(b);
const join=(a,b)=>same(a,b)?a:U;
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
  return fileId(sf)+'::'+(owner?owner+'.':'')+(n.name?.getText(sf)||'<anonymous>');}
 for(const sf of program.getSourceFiles())if(path.resolve(sf.fileName).startsWith(sourceBase+path.sep)){
  const file=fileId(sf);sources.set(file,sf);
  function walk(n){
   if(ts.isClassDeclaration(n))classes.push(n);
   if(ts.isObjectLiteralExpression(n))objects.push(n);
   if(ts.isInterfaceDeclaration(n))interfaces.set(n.name.text,n);
   if((ts.isFunctionDeclaration(n)||ts.isMethodDeclaration(n)||ts.isGetAccessorDeclaration(n)||ts.isConstructorDeclaration(n))&&n.body)
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
 function targets(node){return declarations(node).flatMap(d=>d.body?[d]:ts.isMethodSignature(d)?implementations(d):[]).map(d=>byNode.get(d)).filter(Boolean);}
 function pos(n){const sf=n.getSourceFile(),p=sf.getLineAndCharacterOfPosition(n.getStart());return {file:fileId(sf),line:p.line+1,start:n.getStart(),end:n.end,text:n.getText().slice(0,240)};}
 return {directory,sources,nodes,classes,interfaces,checker,keyOf,targets,declarations,pos,byNode,program};
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
 const issues=[],allocations=new Map(),unresolved=new Map(),pruned=new Map(),contexts=new Map(),queue=[],edges=new Map(),returns=new Map(),outputs=new Map(),active=new Set();
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
  return {tag:'REF',id,type,proven};
 }
 function typed(v){return /^(?:Float64Array|Float32Array|Uint8Array|Uint16Array|Uint32Array|Int8Array|Int16Array|Int32Array)(?:<.*>)?$/.test(v.type||'');}
 function place(item,node){return {owner:item.key,...m.pos(node)};}
 function record(map,item,node,data,ctx){const p=place(item,node),k=p.owner+':'+p.start+':'+(data.reason||data.kind||'');
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
  if(factsValid&&base.proven&&((isWorkspace&&['poolProgress','fieldColumnEvidenceV2','diagnosticEnabled'].includes(name))||(isIndex&&name==='diagnostic'))){
   const entry=profileSlots[name];if(entry==='ON')return {...value(node,k,true),origin:'profile:'+profile+':'+k};
   if(entry)return {...entry,origin:'profile:'+profile+':'+k};
  }
  // Getters execute; do not treat a property-access as mere storage.
  const ds=m.declarations(node.name||node);const getters=ds.filter(d=>ts.isGetAccessorDeclaration(d)&&d.body);
  if(getters.length){for(const n of getters)enqueue(m.byNode.get(n),[],base,s,item,ctx,node);return U;}
  return {...value(node,k,base.proven&&name==='mergeIndex'),origin:'property:'+k};
 }
 function invalidate(s,item,node,ctx,values){
  const escaped=values.filter(v=>v.tag==='REF');
  if(escaped.length){unknown(item,node,'UNKNOWN_CALL_OR_ALIAS_ESCAPE',ctx);
   // Unknown code can mutate through any alias. Invalidating all known slots is conservative.
   for(const [k] of s.heap)s.heap.set(k,U);
   s.heap.set('!escaped',U);
  }
 }
 function enqueue(target,args,self,s,item,parent,node){
  if(!target)return U;
  const paramValues=target.node.parameters.map((p,i)=>args[i]||value(p,`${parent.key}:${p.name.getText()}:unknown`));
  const inherited=[...s.heap].filter(([k])=>k==='!escaped'||!k.startsWith('local:'));
  const signature=target.key+'|this='+brief(self)+'|'+paramValues.map(brief).join(',')+'|slots='+inherited.map(([k,v])=>k+'='+brief(v)).sort().join(',');
  const key=target.key+'#'+hash(signature).slice(0,20);
  if(contexts.size>2500){unknown(item,node,'CONTEXT_LIMIT_FAIL_CLOSED',parent);return U;}
  if(!contexts.has(key)){
   const trace=[...parent.trace,{...m.pos(node),caller:item.key,callee:target.key}];
   const c={key,item:target,args:paramValues,self,heap:inherited,trace};contexts.set(key,c);
  }
  edges.set(parent.key+'>'+key+':'+node.pos,{from:parent.key,to:key,site:place(item,node)});usedFiles.add(target.file);
  if(active.has(key)){unknown(item,node,'RECURSIVE_SUMMARY_NOT_PROVEN',parent);invalidate(s,item,node,parent,[self,...args]);return U;}
  if(!outputs.has(key))execute(contexts.get(key));
  const output=outputs.get(key);if(output)for(const [slot,v]of output.heap)s.heap.set(slot,v);
  return returns.get(key)||U;
 }
 function assign(left,v,s,item,ctx){
  if(ts.isIdentifier(left)){s.env.set(left.text,v);return;}
  if(ts.isPropertyAccessExpression(left)){
   const base=expr(left.expression,s,item,ctx),name=left.name.text;
   if(['poolProgress','diagnosticEnabled','fieldColumnEvidenceV2','diagnostic'].includes(name))unknown(item,left,'HOT_CONFIGURATION_MUTATION',ctx);
   if(base.tag==='REF'){
    const key=base.id+'.'+name;if(v.tag!=='UNKNOWN'||s.heap.has(key))s.heap.set(key,v);
   }else unknown(item,left,'UNKNOWN_ASSIGNMENT_RECEIVER',ctx);
   return;
  }
  if(ts.isElementAccessExpression(left)){
   const base=expr(left.expression,s,item,ctx);if(left.argumentExpression)expr(left.argumentExpression,s,item,ctx);
   if(!typed(base))unknown(item,left,'COMPUTED_WRITE_ALIAS_NOT_PROVEN_TYPED_STORAGE',ctx);return;
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
   if(s.heap.has('!escaped'))return value(n,'escaped:'+n.getText());
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
   const target=m.targets(ts.isPropertyAccessExpression(callee)?callee.name:callee);
   if(!target.length){unknown(item,n,'UNRESOLVED_CALL',ctx);invalidate(s,item,n,ctx,[...(receiver?[receiver]:[]),...args]);return U;}
   let out=null;
   for(const to of target){const v=enqueue(to,args,receiver||U,s,item,ctx,n);out=out?join(out,v):v;}
   return out||U;
  }
  if(ts.isNewExpression(n)){
   record(allocations,item,n,{kind:'constructor'},ctx);expr(n.expression,s,item,ctx);for(const a of n.arguments||[])expr(a,s,item,ctx);return value(n,'new:'+item.key+':'+n.pos);
  }
  if(ts.isObjectLiteralExpression(n)){
   record(allocations,item,n,{kind:'object-literal'},ctx);
   for(const p of n.properties){if(ts.isPropertyAssignment(p))expr(p.initializer,s,item,ctx);else if(ts.isShorthandPropertyAssignment(p))expr(p.name,s,item,ctx);else if(ts.isSpreadAssignment(p)){expr(p.expression,s,item,ctx);unknown(item,p,'SPREAD_OBJECT',ctx);}else unknown(item,p,'OBJECT_METHOD_OR_ACCESSOR',ctx);}
   return value(n,'literal:'+item.key+':'+n.pos);
  }
  if(ts.isArrayLiteralExpression(n)){
   record(allocations,item,n,{kind:'array-literal'},ctx);for(const e of n.elements)expr(e,s,item,ctx);return value(n,'array:'+n.pos);
  }
  if(ts.isArrowFunction(n)||ts.isFunctionExpression(n)){record(allocations,item,n,{kind:'closure'},ctx);unknown(item,n,'CLOSURE_BODY_NOT_MODELED',ctx);return U;}
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
  const self=ts.isMethodDeclaration(item.node)||ts.isGetAccessorDeclaration(item.node)?{tag:'REF',id:'root:'+item.key+':this',type:item.owner,proven:true}:U;
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
  const s={env:new Map(),heap:new Map(ctx.heap),completion:'normal',result:U};
  for(let i=0;i<item.node.parameters.length;i++){const p=item.node.parameters[i];if(!ts.isIdentifier(p.name))unknown(item,p,'PARAMETER_DESTRUCTURING',ctx);else s.env.set(p.name.text,ctx.args[i]||U);}
  const result=statement(item.node.body,s,item,ctx);let merged=result,returned=null;
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
 const remaining=allocations.size+unresolved.size+legacy.filter(s=>s.Reachability==='HOT_REACHABLE').length+issues.length;
 return {analysisRevision:'T168_CONTEXT_V1',scope:'Preserved original Hot roots/getters, actual local imports and resolved interface implementations; explicit source allocations only',
  profile,status:complete&&remaining===0?'ZERO_ALLOCATION':'NOT_ZERO_ALLOCATION',remainingSiteCount:remaining,productionHotPathZeroAllocation:complete&&remaining===0,
  profileFacts:{assertion:profile,valid:factsValid&&profile!=='UNKNOWN',defaults:defaultProof,coldBoundary: cold,writeInventory:facts,sourceContract:contractPath,sourceContractSha256:fs.existsSync(contractPath)?hash(fs.readFileSync(contractPath)):null,limitations:'Entry assertions only; operation must not reconfigure. Approved AST write/control-flow fingerprints plus actual defaults are pinned, not arbitrary cold-program theorem proving. Raw configuration changes/unknown escapes fail closed; does not prove runtime GC/SDK implicit allocation.'},
  analysisInputManifest:[__filename,path.join(repo,'test/probe_p011_particle_hot_graph.cjs'),contractPath,registryPath].map(file=>({file,sha256:fs.existsSync(file)?hash(fs.readFileSync(file)):null})),
  sourceManifest:{commit,dirty,sourceRoot:m.directory,files:sourceManifest},rootManifest:rootItems.map(n=>({key:n.key,...m.pos(n.node)})),
  legacyRegistry:legacy,legacySiteCount:legacy.length,registryComplete:complete,reachableContexts:[...contexts.values()].map(c=>({key:c.key,owner:c.item.key,parameters:c.args.map(brief),witness:c.trace})),
  callEdges:[...edges.values()],prunedBranches:[...pruned.values()],allocationSites:[...allocations.values()],unresolvedCalls:[...unresolved.values()],analysisIssues:issues,
  soundnessLimits:['External/unknown callbacks fail closed','Unsupported syntax fails closed','No unbounded numeric simulation; loop finite-domain fixpoint','Interface implementation set is local closed-source set; external implementations remain outside proof scope'],
  nativeRules:['Math numeric intrinsics (fixed original set)','Number validation intrinsics (fixed original set)','TypedArray.set/fill only after receiver checker type validation']};
}
module.exports={audit,model,factInventory,inventoryIdentity};
