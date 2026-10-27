'use strict';
// Adversarial source fixtures; never modify business sources or archived V1 evidence.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const {audit}=require('./probe_p011_particle_hot_graph.cjs');
const repo=path.resolve(__dirname,'..'),base=path.join(repo,'.codex/local-test/t168-rev2-fixture-'+process.pid),source=path.join(base,'ets/physics');
const marker='new Uint8Array(900268)',has=r=>r.allocationSites.some(a=>a.kind==='constructor'&&a.text===marker);
const effects=(r,kind)=>r.allocationEffects.some(a=>a.kind===kind);
const cases=[
 {id:'baseline-fixed-array',check:r=>{assert.equal(r.status,'ZERO_ALLOCATION');assert.equal(r.rootManifest.length,24);assert.equal(r.legacySiteCount,60);assert.ok(r.storageProofs.some(p=>p.ownerStorage?.length===8));assert.ok(r.reachableContexts.some(c=>c.owner.includes('recordParticleColumnGuardInto')));}},
 {id:'fixed-oob',inject:'work.fieldBinding[8]=work.fieldBinding[0];',check:r=>assert.ok(r.unresolvedCalls.some(x=>x.reason==='COMPUTED_WRITE_ALIAS_NOT_PROVEN_FIXED_STORAGE'))},
 {id:'fixed-unknown-index',inject:'work.fieldBinding[input.seed]=work.fieldBinding[0];',check:r=>assert.ok(r.unresolvedCalls.some(x=>x.reason==='COMPUTED_WRITE_ALIAS_NOT_PROVEN_FIXED_STORAGE'))},
 {id:'fixed-push',inject:'work.fieldBinding.push(work.fieldBinding[0]);',check:r=>assert.ok(effects(r,'Array_POTENTIAL_GROWTH'))},
 {id:'fixed-length',inject:'work.fieldBinding.length=999;',check:r=>assert.ok(r.unresolvedCalls.some(x=>x.reason==='FIXED_ARRAY_LENGTH_MUTATION'))},
 {id:'fixed-escape',inject:`opaque168(work.fieldBinding);work.fieldBinding[0]=work.fieldBinding[1];`,check:r=>assert.ok(r.havocProofs.some(x=>x.affected.some(v=>v.endsWith('.fieldBinding'))))},
 {id:'super-virtual-derived',inject:'const d=new R2Derived();d.run();',extra:`class R2Base { run():void { this.virtual(); } virtual():void {} } class R2Derived extends R2Base { run():void{super.run();} virtual():void{${marker};} }`,check:r=>assert.ok(has(r))},
 {id:'unknown-base-virtual-set',inject:'r2Invoke(work.field as unknown as R2Base);',extra:`class R2Base { virtual():void{} } class R2Derived extends R2Base { virtual():void{${marker};} } function r2Invoke(d:R2Base):void{d.virtual();}`,check:r=>assert.ok(has(r))},
 {id:'super-same-storage',inject:'const d=new R2Derived();d.run(work);',extra:`class R2Base { w:P011ParticleHotWorkspace|null=null; store(w:P011ParticleHotWorkspace):void{this.w=w;} } class R2Derived extends R2Base { run(w:P011ParticleHotWorkspace):void{super.store(w);if(this.w!==null&&this.w.poolProgress!==null){${marker};}} }`,check:r=>assert.equal(has(r),false)},
 {id:'local-map-primitive-escape',inject:`const m=new Map<number,number>();m.set(1,2);opaque168(m);if(work.poolProgress!==null){${marker};}`,check:r=>{assert.equal(has(r),false);assert.ok(effects(r,'Map_POTENTIAL_GROWTH'));assert.ok(r.havocProofs.some(x=>x.preservedUnreachableEntryObjects));}},
 {id:'local-map-workspace-escape',inject:`const m=new Map<number,P011ParticleHotWorkspace>();m.set(1,work);opaque168(m);if(work.poolProgress!==null){${marker};}`,check:r=>assert.ok(has(r))},
 {id:'local-set-growth',inject:'const s=new Set<number>();s.add(1);',check:r=>assert.ok(effects(r,'Set_POTENTIAL_GROWTH'))},
 {id:'map-iterator-effect',inject:'const m=new Map<number,number>();m.keys();',check:r=>assert.ok(effects(r,'Map_ITERATOR_CREATION'))},
 {id:'spoof-get',inject:'const f=new R2Fake();f.get(1);',extra:`class R2Fake {get(k:number):number{${marker};return k;}}`,check:r=>assert.ok(has(r))},
 {id:'spoof-set',inject:'const f=new R2Fake();f.set(1);',extra:`class R2Fake {set(k:number):void{${marker};}}`,check:r=>assert.ok(has(r))},
 {id:'closure-not-called',inject:`const f=()=>{${marker};};`,check:r=>{assert.equal(has(r),false);assert.ok(r.allocationSites.some(x=>x.kind==='closure'));}},
 {id:'closure-called',inject:`const f=()=>{${marker};};f();`,check:r=>assert.ok(has(r))},
 {id:'local-callback',inject:`const f=()=>{${marker};};r2Apply(f);`,extra:'function r2Apply(f:()=>void):void{f();}',check:r=>assert.ok(has(r))},
 {id:'closure-scalar-escape',inject:`const n=input.seed;const f=()=>n;opaque168(f);if(work.poolProgress!==null){${marker};}`,check:r=>assert.equal(has(r),false)},
 {id:'closure-workspace-escape',inject:`const f=()=>work;opaque168(f);if(work.poolProgress!==null){${marker};}`,check:r=>assert.ok(has(r))},
 {id:'closure-workspace-write',inject:`const f=()=>{work.poolProgress=work.mergeIndex as unknown as P011PoolProgressDiagnostic;};f();if(work.poolProgress!==null){${marker};}`,check:r=>assert.ok(has(r))},
 {id:'unknown-callback',inject:'r2UnknownCallback();',extra:'declare const r2UnknownCallback:()=>void;',check:r=>assert.ok(r.unresolvedCalls.some(x=>x.text==='r2UnknownCallback()'))},
 {id:'nested-string-result',inject:'input.seed.toString(16).padStart(8,"0");',check:r=>assert.equal(r.allocationEffects.filter(x=>x.kind==='STRING_RESULT'&&x.owner.endsWith('::beginStepHot')).length,2)},
 {id:'nested-arguments',inject:`input.seed.toString(${marker}.length).padStart(8,"0");`,check:r=>assert.ok(has(r))},
 ...['break;','return 0;','throw 1;',''].map((exit,i)=>({id:'iterator-close-'+i,inject:`for(const n of new R2Iterator()){${exit}}`,extra:`class R2Iterator { [Symbol.iterator]():R2Iterator{return this;} next():{done:boolean,value:number}{return {done:false,value:1};} return():{done:boolean,value:number}{${marker};return {done:true,value:0};} }`,check:r=>assert.ok(has(r))})),
 {id:'iterator-zero',inject:`for(const n of new R2Iterator()){${marker};}`,extra:'class R2Iterator { [Symbol.iterator]():R2Iterator{return this;} next():{done:boolean,value:number}{return {done:true,value:0};} }',check:r=>assert.equal(has(r),false)},
 {id:'builtin-iteration',inject:'for(const n of [1,2]){if(n>0){break;}}',check:r=>{assert.ok(effects(r,'BUILTIN_FOR_OF_ITERATOR_CREATION'));assert.ok(effects(r,'BUILTIN_ITERATOR_RESULT'));}},
 {id:'unknown-iteration',inject:'for(const n of input as unknown as Iterable<number>){}',check:r=>assert.ok(r.layers.NOT_IMPLEMENTED_ANALYZER.some(x=>x.reason.includes('ITERATOR_PROTOCOL')))},
 {id:'inherited-iterator-close',inject:'for(const n of new R2DerivedIterator()){break;}',extra:`class R2Iterator { [Symbol.iterator]():R2Iterator{return this;} next():{done:boolean,value:number}{return {done:false,value:1};} return():{done:boolean,value:number}{${marker};return {done:true,value:0};} } class R2DerivedIterator extends R2Iterator {}`,check:r=>assert.ok(has(r))},
 {id:'closure-this-write',inject:'const d=new R2Writer();d.run(work);',extra:`class R2Writer { run(work:P011ParticleHotWorkspace):void{const f=()=>{this.write(work);};f();if(work.poolProgress!==null){${marker};}} write(work:P011ParticleHotWorkspace):void{work.poolProgress=work.mergeIndex as unknown as P011PoolProgressDiagnostic;} }`,check:r=>assert.ok(has(r))},
];
function run(c){
 fs.mkdirSync(base,{recursive:true});fs.cpSync(path.join(repo,'entry/src/main/ets'),path.join(base,'ets'),{recursive:true});
 if(c.inject){const p=path.join(source,'P011ParticleHot.ets'),sig='export function beginStepHot(work: P011ParticleHotWorkspace, input: P011ParticleHotStepInput): number {';
  const text=fs.readFileSync(p,'utf8');assert.equal(text.split(sig).length,2);fs.writeFileSync(p,text.replace(sig,sig+'\n'+c.inject+'\n')+'\n'+(c.extra||''));}
 const r=audit({profile:'OFF_ALL',sourceRoot:source});c.check(r);
 if(c.inject)assert.equal(r.status,'NOT_ZERO_ALLOCATION');
 return {id:c.id,status:'PASS',remaining:r.remainingSiteCount,source:r.allocationSites.length,effects:r.allocationEffects.length,proofGaps:r.layers.PROOF_GAPS.length,notImplemented:r.layers.NOT_IMPLEMENTED_ANALYZER.length};
}
if(process.argv.includes('--case')){console.log(JSON.stringify(run(cases.find(c=>c.id===process.argv.at(-1)))));}
else{
 const results=cases.map(c=>{const p=cp.spawnSync(process.execPath,[__filename,'--case',c.id],{encoding:'utf8',maxBuffer:8*1024*1024,timeout:120000});
  const r=p.status===0?JSON.parse(p.stdout):{id:c.id,status:'FAIL',output:p.stdout+p.stderr,error:p.error?.message};console.log(r.status+' '+c.id);return r;});
 const out=path.join(repo,'test/reports/t168-rev2');fs.mkdirSync(out,{recursive:true});
 fs.writeFileSync(path.join(out,'effect-selftest.json'),JSON.stringify({device:'NOT_RUN',passed:results.filter(r=>r.status==='PASS').length,failed:results.filter(r=>r.status==='FAIL').length,results},null,2)+'\n');
 if(results.some(r=>r.status==='FAIL'))process.exitCode=1;
}
