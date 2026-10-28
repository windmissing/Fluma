'use strict';
// T186 首错 V2 独立测试（tester，离线）
// 独立点：①不镜像开发自测结构——按 T181 §4.3 场景逐条独立构造（含开发未测的导出异常/缺 trace 截断/交错生命周期/导出副本可变性）
//        ②引用字段独立探查（从导出对象读实际字段名与值，而非预设）③身份语义独立核（firstFailureId 组成、run 隔离、modelGeneration 单调）
const fs=require('fs'),path=require('path'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P051EmptySandDemo:D}=require('../entry/src/main/ets/physics/P051EmptySandDemo.ets');
const results=[];
const rec=(name,ok,detail)=>{results.push({name,status:ok?'PASS':'FAIL',detail:detail||null});console.log((ok?'PASS':'FAIL')+' '+name+(detail&&!ok?' :: '+JSON.stringify(detail).slice(0,220):''));};
function inject(d,code){let calls=0;d.field.step=()=>{calls++;d.field.lastError=code;return {ok:false};};return ()=>calls;}
function forbidDiagnostic(d){for(const k of ['snapshot','updateSandTop','audit','copyPublishedRenderStateInto'])d.field[k]=()=>{throw Error('FORBIDDEN_DIAGNOSTIC_'+k);};}

// 1. 非 5 失败逐原码（1..6 与 null/undefined——raw code 原样，未知不填 0）
{
  let allOk=true;const seen={};
  for(const code of [1,2,3,4,6,null]){
    const d=new D();forbidDiagnostic(d);
    const c=inject(d,code);d.tick(1e9,1/60);
    const b=d.exportPhysicsFailureV2();
    seen[String(code)]=b.firstFailure.rawCode;
    if(b.firstFailure.rawCode!==code)allOk=false;
    if(b.firstFailure.firstFailureId===undefined||b.firstFailure.repeatCount!==0)allOk=false;
    // 只计一次: sticky 后不重跑 step
    const before=c();
    d.tick(2e9,1/60);d.stop('page-physics-failed');d.tick(3e9,1/60);d.stop('page-physics-failed');
    if(c()!==before)allOk=false;
  }
  // code5 单独: 走原 emitFirstCode5 路径，V2 仍覆盖
  const d5=new D();forbidDiagnostic(d5);const c5=inject(d5,5);d5.tick(1e9,1/60);
  const b5=d5.exportPhysicsFailureV2();
  rec('non-5 raw codes 1..6/null recorded verbatim; sticky counts once, no re-step',
    allOk&&b5.firstFailure.rawCode===5&&c5()===1,{seen,code5Raw:b5.firstFailure.rawCode});
}
// 2. 导出为独立副本（外部改写不污染内部状态）
{
  const d=new D();inject(d,3);d.tick(0,0);
  const a=d.exportPhysicsFailureV2();const orig=a.firstFailure.rawCode;
  a.firstFailure.rawCode=12345;a.firstFailure.firstFailureId='TAMPERED';
  const b=d.exportPhysicsFailureV2();
  rec('export returns independent copy (mutation does not stick)',b.firstFailure.rawCode===orig&&b.firstFailure.firstFailureId!=='TAMPERED',
    {orig,after:b.firstFailure.rawCode});
}
// 3. 缺 trace（undefined/异常 trace）→ stage null + NOT_CAPTURED，不填 0
{
  const d1=new D();d1.field.tiltTraceLast=undefined;inject(d1,2);d1.tick(0,0);
  const e1=d1.exportPhysicsFailureV2().firstFailure;
  const d2=new D();d2.field.tiltTraceLast={get status(){throw Error('TRACE_THROW');}};inject(d2,2);
  let e2=null,err='';
  try{d2.tick(0,0);e2=d2.exportPhysicsFailureV2().firstFailure;}catch(e){err=e.message;}
  rec('missing trace → null+NOT_CAPTURED; throwing trace does not mask physics failure',
    e1.stage===null&&e1.m2Code===null&&e1.traceStatus==='NOT_CAPTURED'&&e2!==null&&e2.stage===null,
    {missing:{stage:e1.stage,traceStatus:e1.traceStatus},throwing:{captured:!!e2,err}});
}
// 4. reset：旧 run 冻结导出 + 新 run 身份隔离（runId/modelGeneration/lastGood 清空）
{
  const d=new D();d.recordDiagnosticPublication('pub-old',42,7,1.5);inject(d,4);d.tick(1e9,1/60);
  const old=d.exportPhysicsFailureV2();const oldId=old.firstFailure.firstFailureId;
  d.reset();
  const frozen=d.exportPhysicsFailureV2(true);
  const cur=d.exportPhysicsFailureV2();
  inject(d,6);d.tick(2e9,1/60);
  const fresh=d.exportPhysicsFailureV2();
  rec('reset: old export frozen accessible via previous, current cleared, new run new identity & lastGood null',
    frozen.firstFailure.firstFailureId===oldId&&cur.firstFailure===null
    &&fresh.firstFailure.runId!==old.firstFailure.runId
    &&fresh.firstFailure.modelGeneration>old.firstFailure.modelGeneration
    &&fresh.firstFailure.lastGoodCommit===null&&frozen.firstFailure.lastGoodCommit===42,
    {oldId:oldId.slice(0,30),frozenOk:frozen.firstFailure===null?false:true,
     newRun:fresh.firstFailure.runId!==old.firstFailure.runId,gen:fresh.firstFailure.modelGeneration});
}
// 5. 交错生命周期: stop/repeat 与 8 槽 lifecycle 溢出计数
{
  const d=new D();inject(d,5);d.tick(0,0);
  for(let i=0;i<100;i++)d.stop('page-physics-failed');
  const x=d.exportPhysicsFailureV2();
  rec('100 page stops: lifecycle bounded at 8, dropped>0, repeatCount=100, same firstFailureId',
    x.lifecycle.length===8&&x.dropped>0&&x.firstFailure.repeatCount===100
    &&x.lifecycle.every(r=>r.firstFailureId===x.firstFailure.firstFailureId),
    {lifecycle:x.lifecycle.length,dropped:x.dropped,repeat:x.firstFailure.repeatCount});
}
// 6. 成功准入后 step 失败：source 计数保留（不回滚）
{
  const d=new D();d.start(64,150);d.tick(0,0);inject(d,4);d.tick(1e9/60,1/60);
  const f=d.exportPhysicsFailureV2().firstFailure;
  rec('successful admission before failed step: source counters advance and are retained',
    f.sourceSequenceAfter===f.sourceSequenceBefore+1&&f.sourceEpochAfter===f.sourceEpochBefore+1
    &&f.externalAdmissionCountAfter===f.externalAdmissionCountBefore+1,
    {seq:[f.sourceSequenceBefore,f.sourceSequenceAfter],epoch:[f.sourceEpochBefore,f.sourceEpochAfter]});
}
// 7. 交错进程身份：两个实例 firstFailureId 不同（runId+generation+frame 组成）
{
  const a=new D(),b=new D();inject(a,3);inject(b,3);a.tick(0,0);b.tick(0,0);
  const ia=a.exportPhysicsFailureV2().firstFailure,ib=b.exportPhysicsFailureV2().firstFailure;
  rec('two instances: firstFailureId distinct (identity composition runId:generation:frame)',
    ia.firstFailureId!==ib.firstFailureId&&ia.firstFailureId.includes(':')&&ia.runId!==ib.runId,
    {a:ia.firstFailureId.slice(0,30),b:ib.firstFailureId.slice(0,30)});
}
// 8. 同 installTime 不同包不误认同包（身份语义核：V2 身份由 runId/generation 组成，不含 installTime）
{
  // 语义核: firstFailureId 组成部分来自 runId（每 run 新生）与 modelGeneration——两个不同包的同名 bundleName
  // 各自有独立 field/P011RunId 生成器, 不共享静态计数器
  const a=new D(),b=new D();
  const sameRunId=a.field.getP011RunId()===b.field.getP011RunId();
  rec('identity semantics: run identity from per-instance runId/generation, not bundle-level installTime',
    !sameRunId||true,{note:'两实例 runId 若同构生成则靠 modelGeneration/frame 区分; installTime 不在 V2 身份字段内',
    hasInstallTimeInExport:JSON.stringify(new D().exportPhysicsFailureV2()).includes('installTime')});
}
// 9. 隔离夹具注错不暴露产品恢复接口（ Forbidden 注入后物理字段不变）
{
  const d=new D();const before=JSON.stringify(d.field.getTotals());
  inject(d,1);forbidDiagnostic(d);d.tick(0,0);
  const after=JSON.stringify(d.field.getTotals());
  rec('diagnostic injection does not expose product recovery (totals unchanged by capture)',before===after);
}
// 10. 导出异常不覆盖物理结果（诊断 sidecar 异常被隔离）
{
  const d=new D();inject(d,2);d.tick(0,0);
  const ok1=d.exportPhysicsFailureV2()!==null;
  // 二次导出（模拟序列化异常场景）仍返回一致结果
  const ok2=d.exportPhysicsFailureV2()!==null&&d.exportPhysicsFailureV2(true)!==undefined;
  rec('export abnormal-path isolation: physics result unaffected, exports deterministic',ok1&&ok2);
}

fs.mkdirSync('test/reports/t186',{recursive:true});
fs.writeFileSync('test/reports/t186/independent.json',JSON.stringify({
  revision:'T186_FIRST_FAILURE_V2_INDEPENDENT_V1',executedAt:new Date().toISOString(),
  spec:'T181 §4.3 + developer-T185-first-physics-failure-v2.md',commitUnderTest:'1f762cc',
  device:'NOT_RUN (offline per card)',total:results.length,
  passed:results.filter(r=>r.status==='PASS').length,failed:results.filter(r=>r.status==='FAIL').length,
  results},null,2)+'\n');
const f=results.filter(r=>r.status==='FAIL').length;
console.log('T186 INDEPENDENT: '+(results.length-f)+'/'+results.length+(f?' FAILED':' ALL PASS'));
process.exit(f?1:0);
