'use strict';
// Developer adapter tests. T155 must supply independent fixtures/verdicts.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const {audit}=require('./probe_p011_particle_hot_graph.cjs');
const repo=path.resolve(__dirname,'..'),base=path.join(repo,'.codex/local-test/t168-adapter-fixture');
const source=path.join(base,'ets/physics'),hot='P011ParticleHot.ets';
const marker='new Uint8Array(900168)';
const cases=[
 {id:'guard-external-array',inject:'const t168Outside = [];',expect:r=>assert.ok(r.allocationSites.some(a=>a.text==='[]'&&a.owner.endsWith('::beginStepHot')))},
 {id:'nonnull-parameter',inject:'t168Use(work.mergeIndex as unknown as P011PoolProgressDiagnostic);',extra:`function t168Use(d: P011PoolProgressDiagnostic|null): void { if(d!==null){ ${marker}; } }`,allocation:true},
 {id:'alias-reassignment',inject:`let d=work.poolProgress; d=work.mergeIndex as unknown as P011PoolProgressDiagnostic; if(d!==null){${marker};}`,allocation:true},
 {id:'two-distinct-workspaces',inject:'t168Second(work,work.field as unknown as P011ParticleHotWorkspace);',extra:`function t168Second(a: P011ParticleHotWorkspace,b: P011ParticleHotWorkspace): void { const d=b.poolProgress;if(d!==null){${marker};} }`,allocation:true},
 {id:'evidence-independent',profile:'PROGRESS_OFF_EVIDENCE_ON',expect:r=>{
  assert.ok(r.allocationSites.some(a=>a.owner==='P011ParticleField.ets::P011ParticleField.totals'&&a.kind==='object-literal'));
  assert.ok(r.reachableContexts.some(c=>c.owner.includes('P011FieldColumnEvidenceV2.captureFailure')));
 }},
 {id:'progress-unknown-clock',profile:'PROGRESS_ON',expect:r=>assert.ok(r.unresolvedCalls.some(a=>a.text==='this.clock()'))},
 {id:'same-method-different-class',inject:'const t168Good = new T168Good();t168Good.count();',extra:`class T168Good { count():void {} } class T168Bad { count():void{${marker};} }`,expect:r=>{
  assert.ok(r.reachableContexts.some(c=>c.owner.includes('T168Good.count')));assert.ok(!r.reachableContexts.some(c=>c.owner.includes('T168Bad.count')));
 }},
 {id:'finally-allocation',inject:`try{return 0;}finally{${marker};}`,allocation:true},
 {id:'finally-unknown-early-return',inject:`let t168d=work.poolProgress;try{if(input.seed>0){t168d=work.mergeIndex as unknown as P011PoolProgressDiagnostic;return 0;}}finally{t168d?.increment(1,${marker}.length);}`,allocation:true},
 {id:'unknown-branch-allocation',inject:`if(input.seed>0){${marker};}`,allocation:true},
 {id:'index-real-dependency',mutate:(files)=>{files['P011MergeExactIndex.ets']=files['P011MergeExactIndex.ets'].replace('private findBody(p: P011ParticleSnapshot,radius: number): boolean {',`private findBody(p: P011ParticleSnapshot,radius: number): boolean { ${marker};`);},allocation:true},
 {id:'configuration-rebind',mutate:files=>{files['WaterSandField.ets']=files['WaterSandField.ets'].replace('w.poolProgress=null;w.mergeIndex.diagnostic=null;return;', 'w.poolProgress=new P011PoolProgressDiagnostic(null,w.pool.hotCanonicalBuffers());w.mergeIndex.diagnostic=null;return;');},expect:r=>assert.ok(r.analysisIssues.some(a=>a.reason==='CONFIGURATION_WRITE_INVENTORY_CHANGED'))},
 {id:'cold-guard-reversed',mutate:files=>{files['WaterSandField.ets']=files['WaterSandField.ets'].replace('if(!this.poolProgressEnabled){w.poolProgress=null;', 'if(this.poolProgressEnabled){w.poolProgress=null;');},expect:r=>assert.ok(r.analysisIssues.some(a=>a.reason==='COLD_FACT_CONTROL_FLOW_CHANGED'))},
 {id:'optional-null-arguments-skipped',inject:`const t168d=work.poolProgress;t168d?.increment(1,${marker}.length);`,allocation:false,expect:r=>assert.ok(r.prunedBranches.some(p=>p.reason==='OPTIONAL_NULL_CALL_AND_ARGUMENTS'&&p.text.includes('900168')))},
 {id:'optional-chain-then-outside',inject:`const t168d=work.poolProgress;t168d?.increment(1);${marker};`,allocation:true},
 {id:'equivalent-if',inject:`const t168d=work.poolProgress;if(t168d!==null){${marker};}`,allocation:false},
 {id:'equivalent-ternary',inject:`const t168d=work.poolProgress;t168d===null?0:${marker};`,allocation:false},
 {id:'equivalent-early-return',inject:'t168Early(work.poolProgress);',extra:`function t168Early(d:P011PoolProgressDiagnostic|null):void{if(d===null)return;${marker};}`,allocation:false},
 {id:'returned-null-summary',inject:`const t168d=t168Return(work.poolProgress);t168d?.increment(1,${marker}.length);`,extra:'function t168Return(d:P011PoolProgressDiagnostic|null):P011PoolProgressDiagnostic|null{return d;}',allocation:false},
 {id:'callee-mutates-alias',inject:`let t168box={d:work.poolProgress};t168Mutate(t168box,work.mergeIndex as unknown as P011PoolProgressDiagnostic);t168box.d?.increment(1,${marker}.length);`,extra:'function t168Mutate(box:{d:P011PoolProgressDiagnostic|null},value:P011PoolProgressDiagnostic):void{box.d=value;}',allocation:true},
 {id:'loop-fixpoint-alias',inject:`let t168d=work.poolProgress;while(input.seed>0){t168d=work.mergeIndex as unknown as P011PoolProgressDiagnostic;}t168d?.increment(1,${marker}.length);`,allocation:true},
 {id:'unresolved-dependency',inject:'t168NotDefined(work);',expect:r=>assert.ok(r.unresolvedCalls.some(a=>a.text==='t168NotDefined(work)'))},
 {id:'registry-missing-site',registry:true,expect:r=>assert.equal(r.registryComplete,false)},
 {id:'unknown-profile',profile:'UNKNOWN',expect:r=>assert.equal(r.profileFacts.assertion,'UNKNOWN')},
 {id:'typedarray-spoof',inject:'t168Spoof(work);',extra:'function t168Spoof(work:P011ParticleHotWorkspace):void{ const fake={set(x: unknown):void{}};fake.set(work); }',expect:r=>assert.ok(r.unresolvedCalls.some(a=>a.reason==='OBJECT_METHOD_OR_ACCESSOR')||r.allocationSites.some(a=>a.kind==='object-literal'))},
];
function run(test){
 fs.mkdirSync(base,{recursive:true});fs.cpSync(path.join(repo,'entry/src/main/ets'),path.join(base,'ets'),{recursive:true});
 const files={};for(const file of [hot,'P011MergeExactIndex.ets','WaterSandField.ets'])files[file]=fs.readFileSync(path.join(source,file),'utf8');
 if(test.inject){const signature='export function beginStepHot(work: P011ParticleHotWorkspace, input: P011ParticleHotStepInput): number {';
  assert.equal(files[hot].split(signature).length,2);files[hot]=files[hot].replace(signature,signature+'\n'+test.inject+'\n')+'\n'+(test.extra||'');}
 if(test.mutate)test.mutate(files);for(const [file,text]of Object.entries(files))fs.writeFileSync(path.join(source,file),text);
 let registryPath;if(test.registry){const registry=JSON.parse(fs.readFileSync(path.join(repo,'test/fixtures/p011_particle_allocation_registry_v1.json')));registry.sites.pop();registryPath=path.join(base,'registry.json');fs.writeFileSync(registryPath,JSON.stringify(registry));}
 const r=audit({profile:test.profile||'OFF_ALL',sourceRoot:source,registryPath});
 // Baseline can legitimately FAIL. Check the injected site, not merely that verdict.
 if(test.allocation!==undefined)assert.equal(r.allocationSites.some(a=>a.text.includes('new Uint8Array(900168)')),test.allocation,test.id);
 if(test.expect)test.expect(r);
 if(test.allocation||test.registry||test.profile==='UNKNOWN')assert.equal(r.status,'NOT_ZERO_ALLOCATION');
 return {id:test.id,status:'PASS',profile:r.profile,graphVerdict:r.status,remaining:r.remainingSiteCount,allocationSites:r.allocationSites.length,unresolved:r.unresolvedCalls.length};
}
if(process.argv.includes('--case')){
 const c=cases.find(c=>c.id===process.argv[process.argv.indexOf('--case')+1]);if(!c)throw Error('UNKNOWN_CASE');console.log(JSON.stringify(run(c)));
}else{
 const results=[];for(const c of cases){const r=cp.spawnSync(process.execPath,[__filename,'--case',c.id],{cwd:repo,encoding:'utf8',maxBuffer:4*1024*1024});
  if(r.status!==0){results.push({id:c.id,status:'FAIL',output:r.stdout+r.stderr});console.error('FAIL '+c.id);}
  else {results.push(JSON.parse(r.stdout));console.log('PASS '+c.id);}
 }
 fs.mkdirSync(path.join(repo,'test/reports/t168-rev2'),{recursive:true});fs.writeFileSync(path.join(repo,'test/reports/t168-rev2/selftest.json'),JSON.stringify({revision:'T168_DEVELOPER_ADAPTER_TESTS_V1',passed:results.filter(r=>r.status==='PASS').length,failed:results.filter(r=>r.status==='FAIL').length,device:'NOT_RUN',independentTester:'NOT_RUN_T155_OWNS',results},null,2)+'\n');
 if(results.some(r=>r.status==='FAIL'))process.exitCode=1;
}
