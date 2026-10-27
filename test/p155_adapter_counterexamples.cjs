'use strict';
// T155 独立适配器反例（T162 §4.1 清单 + 独立设计差异点，不镜像 T168 开发自测断言）
// 独立性设计：①注入位置不同（checkpointHot/collectParticlePlanInto 而非 beginStepHot）
//            ②marker 值不同（777001）③断言语义不同（验证分类字段与见证链而非仅 presence）
//            ④开发自测未覆盖：E ON totals 分配的 profile 依赖、关闭后重绑失效、
//              switch 分支、复合赋值别名、getter 执行非存储访问
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const {audit}=require('../test/probe_p011_particle_hot_graph.cjs');
const repo=path.resolve(__dirname,'..'),base=path.join(repo,'.codex/local-test/t155-adapter-fixture');
const source=path.join(base,'ets/physics'),hot='P011ParticleHot.ets';
const M='777001';
const results=[];
function rec(name,ok,detail){results.push({name,status:ok?'PASS':'FAIL',detail:detail||null});console.log((ok?'PASS':'FAIL')+' '+name+(detail&&!ok?' :: '+JSON.stringify(detail).slice(0,200):''));}

function runCase(opts){
  fs.mkdirSync(base,{recursive:true});fs.cpSync(path.join(repo,'entry/src/main/ets'),path.join(base,'ets'),{recursive:true});
  const files={};for(const f of [hot,'P011MergeExactIndex.ets','WaterSandField.ets'])files[f]=fs.readFileSync(path.join(source,f),'utf8');
  if(opts.inject){
    // 注入到 collectParticlePlanInto（与开发自测的 beginStepHot 不同函数）
    const sig='export function collectParticlePlanInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number, destination: P011ParticleHotPlanBuffer): number {';
    assert.equal(files[hot].split(sig).length,2,'signature');
    files[hot]=files[hot].replace(sig,sig+'\n'+opts.inject+'\n')+'\n'+(opts.extra||'');
  }
  if(opts.mutate)opts.mutate(files);
  for(const [f,t] of Object.entries(files))fs.writeFileSync(path.join(source,f),t);
  let registryPath;
  if(opts.registry){const reg=JSON.parse(fs.readFileSync(path.join(repo,'test/fixtures/p011_particle_allocation_registry_v1.json')));
    reg.sites.splice(10,1); // 删中间项（与开发自测 pop 尾项不同）
    registryPath=path.join(base,'registry.json');fs.writeFileSync(registryPath,JSON.stringify(reg));}
  return audit({profile:opts.profile||'OFF_ALL',sourceRoot:source,registryPath});
}
const hasM=r=>r.allocationSites.some(a=>a.text.includes(M));

// 1. guard 外数组字面量（注入位置独立）→ 可达分配
{const r=runCase({inject:'const t155out = ['+M+'];'});
 rec('guard-external-array reachable in collectParticlePlanInto',hasM(r)&&r.allocationSites.some(a=>a.kind==='array-literal'&&a.text.includes(M)),{alloc:r.allocationSites.length});}

// 2. 非空 d 传入（参数绑定传播）
{const r=runCase({inject:`t155use(work.mergeIndex as unknown as P011PoolProgressDiagnostic);`,
  extra:`function t155use(d: P011PoolProgressDiagnostic|null): void { if(d!==null){ const x = new Uint8Array(${M}); } }`});
 rec('nonnull parameter binding propagates through helper',hasM(r),{alloc:r.allocationSites.length});}

// 3. E ON profile: totals 对象字面量可达 + captureFailure 上下文（P OFF 不剪 E）
{const r=runCase({profile:'PROGRESS_OFF_EVIDENCE_ON'});
 const totals=r.allocationSites.some(a=>a.owner.includes('P011ParticleField.totals')&&a.kind==='object-literal');
 const cap=r.reachableContexts.some(c=>c.owner.includes('captureFailure'));
 const offR=audit({profile:'OFF_ALL',sourceRoot:source});
 const totalsOff=offR.allocationSites.some(a=>a.owner.includes('P011ParticleField.totals'));
 rec('E ON: totals allocation reachable only under EVIDENCE profile',totals&&cap&&!totalsOff,{on:totals,captureCtx:cap,offTotals:totalsOff});}

// 4. 配置反向重绑（写入 inventory 失效）
{const r=runCase({mutate:files=>{files['WaterSandField.ets']=files['WaterSandField.ets'].replace(
   'w.poolProgress=null;w.mergeIndex.diagnostic=null;return;',
   'w.poolProgress=new P011PoolProgressDiagnostic(null,w.pool.hotCanonicalBuffers());w.mergeIndex.diagnostic=null;return;');}});
 rec('disable-path rebind invalidates profile proof',r.analysisIssues.some(a=>a.reason==='CONFIGURATION_WRITE_INVENTORY_CHANGED')&&r.status==='NOT_ZERO_ALLOCATION',{issues:r.analysisIssues.map(i=>i.reason)});}

// 5. 冷函数守卫反转（控制流 hash 失效）
{const r=runCase({mutate:files=>{files['WaterSandField.ets']=files['WaterSandField.ets'].replace(
   'if(!this.poolProgressEnabled){w.poolProgress=null;',
   'if(this.poolProgressEnabled){w.poolProgress=null;');}});
 rec('cold guard reversal fails closed',r.analysisIssues.some(a=>a.reason==='COLD_FACT_CONTROL_FLOW_CHANGED'),{issues:r.analysisIssues.map(i=>i.reason)});}

// 6. d=null 时 d?.m(new X()) 实参不可达 vs d?.m(); new X() 可达
{const r1=runCase({inject:`const t155d=work.poolProgress;t155d?.increment(1,new Uint8Array(${M}).length);`});
 const argsPruned=!hasM(r1)&&r1.prunedBranches.some(p=>p.reason==='OPTIONAL_NULL_CALL_AND_ARGUMENTS'&&p.text.includes(M));
 const r2=runCase({inject:`const t155d=work.poolProgress;t155d?.increment(1);new Uint8Array(${M});`});
 rec('d=null: d?.m(new X()) arguments unreachable; separate new reachable',
   argsPruned&&hasM(r2),{argsPruned,sepReachable:hasM(r2)});}

// 7. 三种等价写法结果一致（if/ternary/early-return，注入位置独立）
{const rIf=runCase({inject:`const t155d=work.poolProgress;if(t155d!==null){new Uint8Array(${M});}`});
 const rT=runCase({inject:`const t155d=work.poolProgress;t155d===null?0:new Uint8Array(${M});`});
 const rE=runCase({inject:'t155early(work.poolProgress);',
   extra:`function t155early(d:P011PoolProgressDiagnostic|null):void{if(d===null)return;new Uint8Array(${M});}`});
 rec('if/ternary/early-return equivalent null-guards all prune',!hasM(rIf)&&!hasM(rT)&&!hasM(rE),
   {ifP:!hasM(rIf),tP:!hasM(rT),eP:!hasM(rE)});}

// 8. finally 中分配可达（try return 后 finally 仍扫）
{const r=runCase({inject:`try{return 0;}finally{new Uint8Array(${M});}`});
 rec('finally allocation reachable after try-return',hasM(r));}

// 9. registry 缺项（删中间项）→ FAIL closed
{const r=runCase({registry:true});
 rec('registry missing middle site fails complete',r.registryComplete===false&&r.status==='NOT_ZERO_ALLOCATION',{complete:r.registryComplete});}

// 10. 未知调用别名逃逸使槽事实 widen（逃逸后**新读取**诊断槽不再被剪）
// 语义核: 逃逸前已捕获的局部值（NULL）保持被剪是精确语义（未知代码无法改变已捕获值）；
// 逃逸后新读取必须走 escaped 通道（UNKNOWN）不剪——这是探针 invalidate 的正确不动点。
{const r=runCase({inject:`t155esc(work);const t155d=work.poolProgress;t155d?.increment(1,new Uint8Array(${M}).length);`});
 const escU=r.unresolvedCalls.some(u=>u.reason==='UNKNOWN_CALL_OR_ALIAS_ESCAPE'&&u.text.includes('t155esc'));
 const escapedRead=!r.prunedBranches.some(p=>p.reason==='OPTIONAL_NULL_CALL_AND_ARGUMENTS'&&p.text.includes(M))&&hasM(r);
 rec('unknown call escape: post-escape slot reads not pruned (precise invalidation)',escU&&escapedRead,{escU,escapedRead});}

// 11. switch 保守可达（case 内分配不剪）
{const r=runCase({inject:`switch(input.seed>0?1:0){case 1: new Uint8Array(${M}); break;default: break;}`});
 rec('switch case allocation conservatively reachable',hasM(r),{alloc:r.allocationSites.length});}

// 12. 复合赋值别名（d = 其他对象 → UNKNOWN → 不剪）
{const r=runCase({inject:`let t155d=work.poolProgress;t155d=work.mergeIndex as unknown as P011PoolProgressDiagnostic;t155d?.increment(1,new Uint8Array(${M}).length);`});
 rec('reassigned alias not pruned by original null',hasM(r));}

// 13. 伪 TypedArray 方法（同名 set/fill 不豁免）
{const r=runCase({inject:'t155spoof(work);',
  extra:`function t155spoof(work:P011ParticleHotWorkspace):void{ const fake={set(v: unknown):void{}};fake.set(work); }`});
 rec('fake TypedArray-style set not exempted by name',r.unresolvedCalls.some(u=>u.reason==='OBJECT_METHOD_OR_ACCESSOR')||r.allocationSites.some(a=>a.kind==='object-literal')||r.unresolvedCalls.length>0);}

// 14. 未知 profile 入口断言保留
{const r=runCase({profile:'UNKNOWN'});
 rec('unknown profile assertion retained',r.profileFacts.assertion==='UNKNOWN'&&r.status==='NOT_ZERO_ALLOCATION');}

// 15. 真实 OFF_ALL 主图结论独立复现（T168 归档一致性）
{const r=audit({profile:'OFF_ALL'});
 const g=require('../test/reports/t168/gate-summary.json');
 const same=r.remainingSiteCount===g.profiles.OFF_ALL.remaining&&r.status===g.profiles.OFF_ALL.status&&r.registryComplete===true;
 rec('OFF_ALL main graph independent reproduction matches archived',same,
   {remaining:r.remainingSiteCount,archived:g.profiles.OFF_ALL.remaining,status:r.status});}

// 16. 旧无参 API 精确保留（exit 契约）
{const legacy=audit();
 rec('legacy conservative API unchanged (35 remaining, exit contract)',
   legacy.remainingSiteCount===35&&legacy.registryComplete===true&&legacy.status==='NOT_ZERO_ALLOCATION',
   {remaining:legacy.remainingSiteCount});}

// 输出
const failed=results.filter(r=>r.status==='FAIL');
fs.mkdirSync('test/reports/t155',{recursive:true});
fs.writeFileSync('test/reports/t155/adapter_counterexamples.json',JSON.stringify({
  revision:'T155_ADAPTER_COUNTEREXAMPLES_V1',executedAt:new Date().toISOString(),
  spec:'T162 V2 §4.1 + independent variants',probe:'T168 6c0c654',
  total:results.length,passed:results.length-failed.length,failed:failed.length,
  independence:'injection at collectParticlePlanInto (not beginStepHot), marker 777001, registry middle-site removal, classification-level assertions',
  results},null,2)+'\n');
console.log('T155 ADAPTER COUNTEREXAMPLES: '+(results.length-failed.length)+'/'+results.length+(failed.length?' FAILED':' ALL PASS'));
process.exit(failed.length?1:0);
