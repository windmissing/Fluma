'use strict';
const fs=require('node:fs'),path=require('node:path'),Module=require('node:module'),cp=require('node:child_process'),assert=require('node:assert/strict'),crypto=require('node:crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const physics=path.resolve('entry/src/main/ets/physics'),frozen={};
// Test-only body-entry counter: the production first guard is a lexical local,
// so replacing the module export would not count its actual internal calls.
globalThis.__T174FirstGuardCalls=0;
const sinkFile=path.join(physics,'P011ParticleM2Sink.ets'),sinkText=fs.readFileSync(sinkFile,'utf8');
const sinkAst=ts.createSourceFile(sinkFile,sinkText,ts.ScriptTarget.Latest,true,ts.ScriptKind.TS);let guardStart=-1;
for(const n of sinkAst.statements)if(ts.isFunctionDeclaration(n)&&n.name?.text==='guardAdmitInto')guardStart=n.body.getStart(sinkAst)+1;
assert.ok(guardStart>0);const instrumented=new Module(sinkFile,module);instrumented.filename=sinkFile;instrumented.paths=Module._nodeModulePaths(physics);
instrumented._compile(ts.transpileModule(sinkText.slice(0,guardStart)+' globalThis.__T174FirstGuardCalls++; '+sinkText.slice(guardStart),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,sinkFile);require.cache[sinkFile]=instrumented;

function old(file){const source=cp.execFileSync('git',['show','961d837:entry/src/main/ets/physics/'+file],{encoding:'utf8'});frozen[file]=crypto.createHash('sha256').update(source).digest('hex');const fileName=path.join(physics,file),m=new Module(fileName,module);m.filename=fileName;m.paths=Module._nodeModulePaths(physics);m._compile(ts.transpileModule(source,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,fileName);return m.exports;}
const reference=old('P011MaterialGuard.ets'),{WaterSandField:OldField}=old('WaterSandField.ets');
const guard=require(path.join(physics,'P011MaterialGuard.ets')),{WaterSandField:Field}=require(path.join(physics,'WaterSandField.ets'));
const {P011NaturalObservationProbe:Probe}=require(path.join(physics,'P011NaturalObservation.ets')),{defaultP011ParticleParams}=require(path.join(physics,'P011ParticleM2Field.ets'));

const results=[],measurements=[];let numericCases=0;
const test=(name,fn)=>{fn();results.push({name,status:'PASS'});console.log('PASS '+name);};
const bits=x=>{const b=Buffer.alloc(8);b.writeDoubleLE(x);return b.toString('hex');};
function equalOut(a,b){assert.equal(bits(a.accepted),bits(b.accepted));assert.equal(bits(a.rejected),bits(b.rejected));assert.equal(a.allLegalBefore,b.allLegalBefore);assert.equal(a.limitingEdge,b.limitingEdge);}
function normalized(v){return JSON.parse(JSON.stringify(v,(k,v)=>k==='physicsTimeMs'||k==='ownerId'||k==='pageLease'?0:k==='runId'?'RUN':v));}
function physical(f){return {raw:['waterTop','bh','bl','mh','ml','suspH','suspL','sandTopScratch','substepMotionBudgetRemaining','substepMotionBudgetUsed'].map(k=>Buffer.from(f[k].buffer,f[k].byteOffset,f[k].byteLength).toString('hex')),pool:f.particleBinding?f.particleBinding.pool.snapshot():null,source:[f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch],budget:[f.totalMotionBudgetRemaining,f.totalMotionBudgetUsed,f.totalMotionBudgetClipped],events:f.particleBinding?(f.particleBinding.work.pageBacklog===1?normalized(f.particleBinding.exportCommittedEvidence()):{count:f.particleBinding.work.events.count,lineageCount:f.particleBinding.work.events.lineageCount,pageBacklog:f.particleBinding.work.pageBacklog}):null};}
test('numeric Into matches frozen old RN/tie/early exits, signed zero and nonfinite cases',()=>{
 const out=new guard.P011GuardNumericOutcome(),d=new Float64Array(3),k=new Float64Array(3),H=Math.tan(30*Math.PI/180);
 const values=[0,-0,H,-H,H+Number.EPSILON,H-Number.EPSILON,1,-1,NaN,Infinity,-Infinity,Number.MIN_VALUE];
 for(const hard of [H,0,-1,NaN,Infinity])for(const req of [0,-0,-1,NaN,Infinity,.1,1,Number.MIN_VALUE])for(const a of values)for(const b of [-1,-0,0,1,NaN,Infinity])for(const count of [0,1,2,3]){
  d[0]=a;d[1]=0-a;d[2]=a/2;k[0]=b;k[1]=0-b;k[2]=b;
  const constraints=[];for(let i=0;i<count;i++)constraints.push({residual:d[i],influence:k[i]});
  const expected=reference.p011GuardAdmit(req,constraints,hard);assert.equal(guard.p011GuardAdmitNumericInto(req,d,k,count,hard,out),0);equalOut(out,expected);numericCases++;
 }
});
test('single-column host keeps update first, original edge order and bitwise outcomes',()=>{
 const a=new OldField(128,512),b=new Field(128,512);const pa=new Probe(128,()=>{}),pb=new Probe(128,()=>{});a.setNaturalObservationProbe(pa);b.setNaturalObservationProbe(pb);
 let seed=13;for(let sample=0;sample<256;sample++){
  seed=(Math.imul(seed,1664525)+1013904223)>>>0;const column=sample%8===0?-0:sample%8===1?127:sample%8===2?64.5:sample%128;
  const sign=[-1,1,-0,NaN][sample%4],requested=[0,-0,-1,NaN,Infinity,.1,1,10][sample%8];
  for(const f of [a,b]){f.bh.fill(0);f.bl.fill(0);f.mh.fill(0);f.ml.fill(0);for(let c=0;c<128;c++)f.bh[c]=100+((seed+c*17)%1000)/1000;f.effectiveTilt=(sample%11)-5;}
  let calls=0;const original=b.updateSandTop;b.updateSandTop=function(){calls++;return original.call(this);};
  const expected=a.internalParticleGuard(column,sign,requested),actual=b.internalParticleGuard(column,sign,requested);b.updateSandTop=original;
  assert.equal(calls,1);assert.equal(bits(actual),bits(expected));pb.flushParticleColumnGuardEvidence();assert.deepEqual(pb.materialGuardEvidence,pa.materialGuardEvidence);assert.deepEqual(physical(b),physical(a));
 }
});
test('ordinary Q=2048 page preserves all evidence order without borrowing source slots',()=>{
 const p=new Probe(128,()=>{});for(let i=0;i<2048;i++)p.recordParticleColumnGuardInto(i%2+1,i%127,(i+1)%127,i%127,i+1,i,.5);
 assert.equal(p.particleGuardCount,2048);assert.equal(p.materialGuardEvidence.length,0);p.flushParticleColumnGuardEvidence();assert.equal(p.materialGuardEvidence.length,2048);assert.equal(p.particleGuardCount,0);
 for(let i=0;i<2048;i++){const row=p.materialGuardEvidence[i];assert.equal(row.clipped,1);assert.equal(row.rejected,.5);assert.equal(row.limitingEdge,i%127);assert.deepEqual(row.affectedEdgeIds,i%2?[i%127,(i+1)%127]:[i%127]);}
 p.recordParticleColumnGuardInto(1,9,100,null,2,1,1);p.recordMaterialGuardEvidence({channel:'settle',limitingEdge:null,affectedEdgeIds:[],rejected:1,clipped:1});assert.equal(p.materialGuardEvidence[2048].channel,'guard');assert.equal(p.materialGuardEvidence[2049].channel,'settle');
});
test('N probe rebinding and cold flush preserve target row ownership',()=>{
 const f=new Field(128,512),p=new Probe(128,()=>{}),next=new Probe(128,()=>{});f.setNaturalObservationProbe(p);p.recordParticleColumnGuardInto(2,3,4,4,2,1,1);f.setNaturalObservationProbe(next);
 assert.equal(p.materialGuardEvidence.length,1);assert.equal(next.materialGuardEvidence.length,0);assert.equal(p.particleGuardCount,0);
});
function landCase(C,mode,on){const f=new C(128,512),params=defaultP011ParticleParams();params.nmax=16;assert.ok(f.setParticleRoute(params).ok);f.setNaturalObservationRouteId('dual-field-particle-v1');
 const probe=new Probe(128,()=>{});if(on)f.setNaturalObservationProbe(probe);f.updateSandTop();
 const b=f.particleBinding;b.registerRun('T174_OFFLINE',1);const slot=b.pool.allocate({mass:1,x:64.5,y:f.sandTopScratch[64],vx:0,vy:0,phase:0,sourceKind:0,sourceId:17});assert.ok(slot>=0);b.pool.pState[slot]=2;f.sourceInjectedHeavy+=1;f.sourceEpoch++;
 assert.equal(b.beginCollect(1/60,0,1,13,f.sourceInjectedHeavy,f.sourceInjectedLight,f.sourceEpoch),0);f.totalMotionBudgetLimit=5;f.beginMotionApplication(1/60,1);f.effectiveTilt=20;
 let first=0,second=0;const firstStart=globalThis.__T174FirstGuardCalls,oldSecond=f.internalParticleGuard;
 f.internalParticleGuard=function(...args){second++;const r=oldSecond.apply(this,args);if(mode==='THROW')throw Error('T174_HOST_THROW');return r;};
 if(mode==='ROLLBACK')b.sink.failAfterPublish=1;let result,thrown='';
 try{result=f.applyMotionChannels(1/60);}catch(e){thrown=e.message;}finally{first=globalThis.__T174FirstGuardCalls-firstStart;f.internalParticleGuard=oldSecond;}
 if(mode==='THROW')b.rollback();if(on)probe.flushParticleColumnGuardEvidence();
 return {result,thrown,first,second,diag:{route:f.particleRouteActive(),active:b.active,rows:b.work.planA.requestCount,kind:b.work.planA.kind[0],hot:b.work.lastStatus,stage:b.work.errorStageCode},state:physical(f),evidence:probe.materialGuardEvidence,pending:probe.particleGuardCount};
}
for(const mode of ['NORMAL','ROLLBACK','THROW'])test('real LAND/host guard '+mode+' keeps both calls, refunds, state and failed evidence',()=>{
 const before=landCase(OldField,mode,true),after=landCase(Field,mode,true);assert.ok(before.first>0&&before.second>0);assert.ok(before.evidence.length>0,'must exercise rejected N evidence, not birthOnly');
 assert.deepEqual(after,before);measurements.push({mode,first:after.first,second:after.second,evidence:after.evidence.length,pending:after.pending,result:after.result,thrown:after.thrown});
});
test('N ON/OFF real LAND paths leave physical state bitwise equal',()=>{
 const off=landCase(Field,'NORMAL',false),on=landCase(Field,'NORMAL',true);assert.deepEqual(off.state,on.state);assert.equal(off.first,on.first);assert.equal(off.second,on.second);
});
test('Hot Into methods contain no heap constructors/literals/closures and both cold callers finally flush',()=>{
 const files=['P011MaterialGuard.ets','P011NaturalObservation.ets','WaterSandField.ets'];let into=0;
 for(const file of files){const source=fs.readFileSync(path.join(physics,file),'utf8'),ast=ts.createSourceFile(file,source,ts.ScriptTarget.Latest,true,ts.ScriptKind.TS);
  function visit(n){if((ts.isFunctionDeclaration(n)||ts.isMethodDeclaration(n))&&['p011GuardAdmitNumericInto','recordParticleColumnGuardInto','internalParticleGuard'].includes(n.name?.getText(ast))){into++;function scan(x){assert.ok(!ts.isNewExpression(x)&&!ts.isObjectLiteralExpression(x)&&!ts.isArrayLiteralExpression(x)&&!ts.isArrowFunction(x),x.getText(ast));ts.forEachChild(x,scan);}scan(n.body);}ts.forEachChild(n,visit);}visit(ast);
 }
 assert.equal(into,3);const field=fs.readFileSync(path.join(physics,'WaterSandField.ets'),'utf8');assert.equal((field.match(/finally \{ this.flushParticleGuardEvidence\(guardProbe\); \}/g)||[]).length,2);
 const binding=fs.readFileSync(path.join(physics,'P011ParticleM2Field.ets'),'utf8');assert.match(binding,/requestCapacity: 2048/);
});
fs.mkdirSync('test/reports/t174',{recursive:true});fs.writeFileSync('test/reports/t174/selftest.json',JSON.stringify({revision:'T174_DEVELOPER_V1',passed:results.length,failed:0,numericCases,frozenCommit:'961d837',frozenSourceSha:frozen,measurements,results,device:'NOT_RUN'},null,2)+'\n');console.log(JSON.stringify({passed:results.length,failed:0,numericCases}));
