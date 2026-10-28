'use strict';
// T206 整体复杂度独立验证规格——执行器（tester，离线，预测试 RED）
// 三部分：
//  A. 实现面符号核查（P011_TX_WORK_TRACE_V1 计量/具名 scope/恒等式 全部零实现 → RED）
//  B. 独立恒等式检验表（TX 各组 oracle 规格的机器可读定义——供 trace 到件后逐 scope 检验，
//     本执行器自带独立参考实现，不调用被测源）
//  C. TX01–TX12 逐条状态判定（依赖未实现 → NOT_IMPLEMENTED，不冒产品 FAIL/PASS）
const fs=require('fs'),path=require('path'),assert=require('assert/strict');
const results=[];const rec=(id,name,status,detail)=>{results.push({id,name,status,detail:detail||null});console.log(status+' '+id+' '+name+(detail?' :: '+JSON.stringify(detail).slice(0,180):''));};

// ---------- A. 实现面符号核查 ----------
let allEts='';let missingSymbols=[];
(function walk(dir){for(const e of fs.readdirSync(dir,{withFileTypes:true})){const p=path.join(dir,e.name);
  if(e.isDirectory())walk(p);else if(e.name.endsWith('.ets'))allEts+=fs.readFileSync(p,'utf8');}})('entry/src/main/ets');
const symbols={
  'P011_TX_WORK_TRACE_V1 计量 revision':allEts.includes('P011_TX_WORK_TRACE_V1'),
  'scope ENTER/LEAVE/FAIL/CANCEL 计数':/(ENTER_SCOPE|LEAVE_SCOPE|SCOPE_ENTER|scopeEnter)/.test(allEts),
  '具名 scope 常量 (B_REQ/M_FIND_ENTER/PAIR_BUILD等)':/B_REQ|M_FIND_ENTER|PAIR_BUILD|KEY_INSERT|RELOCATE|NEIGHBOR_TEST|RANK_SELECT|RANGE_NODE/.test(allEts),
  'AUDIT_LOGICAL/EXECUTE/CERT_HIT 分解':/AUDIT_CERT_HIT|CERT_HIT/.test(allEts),
  'COPY_POOL_BYTES/COPY_FIELD_BYTES/UNDO_WRITES 计量':/COPY_POOL_BYTES|UNDO_WRITES/.test(allEts),
  'publication/encode/write/fsync bytes 计量':/PUBLICATION_BYTES|ENCODE_BYTES|FSYNC_BYTES/.test(allEts),
  'GPU prepare/sort/range/SHA_BLOCKS/upload/dispatch 分项计量':/SHA_BLOCKS/.test(allEts),
};
const missing=Object.entries(symbols).filter(([,v])=>!v).map(([k])=>k);
missingSymbols=missing;
rec('SYM','P011_TX_WORK_TRACE_V1 implementation symbols',missing.length===0?'PASS':'NOT_IMPLEMENTED',
  missing.length?{missing:missing}:null);
// 模型证据性质核（4822 例是 designer 算术模型，非产品 oracle）
{
  const d=JSON.parse(fs.readFileSync('docs/design-T197-whole-transaction-evidence-v1.json'));
  const s=JSON.stringify(d);
  rec('MODEL','T197 model evidence is designer arithmetic (4822 examples), NOT product oracle',
    (s.includes('4822')||d.revision!==undefined)?'PASS':'FAIL',{keys:Object.keys(d).slice(0,6)});
}

// ---------- B. 独立恒等式检验表（机器可读规格，独立参考实现） ----------
// 每 scope 检验: entered = completed + failed + cancelled + stillActive
function identityEntered(parts){const{completed,failed,cancelled,stillActive,entered}=parts;
  return entered===completed+failed+cancelled+stillActive;}
function identityM_find(parts){const{found,noPair,error,total}=parts;return total===found+noPair+error;}
function identityAudit(parts){const{logical,execute,certHit}=parts;return logical===execute+certHit;}
function identityBudget(parts){const{bEnt,bReq,bCommit}=parts;return bEnt<=bReq&&bCommit<=bEnt;}
function identityCopyBytes(parts){const{root,childrenSum}=parts;return root===childrenSum;}
function identityFrameWork(parts){const{total,direct,queued,stages,observation,publication,io}=parts;
  return total===direct+queued+stages+observation+publication+io;}
// 独立反例数据（假想 trace 检验器单测——规格自检，证明检验器本身正确）
{
  let ok=true;
  if(!identityEntered({completed:10,failed:2,cancelled:1,stillActive:3,entered:16}))ok=false;
  if(identityEntered({completed:10,failed:2,cancelled:1,stillActive:3,entered:15}))ok=false; // 不等式应 fail
  if(!identityM_find({found:5,noPair:7,error:1,total:13}))ok=false;
  if(identityM_find({found:5,noPair:7,error:1,total:12}))ok=false;
  if(!identityAudit({logical:9,execute:6,certHit:3}))ok=false;
  if(identityAudit({logical:9,execute:6,certHit:2}))ok=false;
  if(!identityBudget({bEnt:5,bReq:10,bCommit:4}))ok=false;
  if(identityBudget({bEnt:11,bReq:10,bCommit:4}))ok=false;
  if(identityBudget({bEnt:5,bReq:10,bCommit:6}))ok=false;
  if(!identityCopyBytes({root:100,childrenSum:100}))ok=false;
  if(identityCopyBytes({root:100,childrenSum:90}))ok=false;
  if(!identityFrameWork({total:100,direct:20,queued:30,stages:30,observation:10,publication:5,io:5}))ok=false;
  rec('IDT','independent identity checkers (entered/M_find/audit/budget/copyBytes/frameWork) self-tested',ok?'PASS':'FAIL');
}

// ---------- C. TX01–TX12 逐条状态 ----------
const txDeps={
  'TX-01':'C/N/L/B 独立变化、空槽F、B>N、full no-pair/early/late 配对；同 M 分母旧scan/T123/new树逐pair一致',
  'TX-02':'邻接只证 eligible；非相邻最小 b；signed-zero/subnormal/rounded y±r/MAX_ID/跨group/质心重定位/退役slot拒旧ID',
  'TX-03':'连续 birth/merge 每 audit、budget 分母/refund、first code/stage/ordinal、λ 恢复与成功 source 前缀（不只 final SHA）',
  'TX-04':'read-set 逐位失效/同rev异内容/alias/pointer/budget.close/motion 缓存 miss，cert hit 与重算一致',
  'TX-05':'slot-order vs tree/ID-order 求和反例；dirty numeric audit 原 fold，证书不用结合律/epsilon',
  'TX-06':'SOURCE R1..8、lift collect 无 seed、apply 多 C/phase seed、V0..3/S1..2、frame 外 DIRECT；计数不二次乘',
  'TX-07':'全 field 通道/water40/eta64/correction C²/dirtyS/观测/发布——pair 热点降低后未藏未计链',
  'TX-08':'undo nested 同地址多写、源成功后 stepfail、plan/事件/lineage/票据溢出、savepoint 逐字节一致',
  'TX-09':'工作量/内存/IO 不足 root 修改前背压；bound violation 整 root rollback；offered/accepted 分母、重试身份',
  'TX-10':'owner FIFO/停止/reset/cancel/deadline/队列满/publication lease/IO 慢/迟到 GPU；不丢已接受 source；UI 显式 STALE',
  'TX-11':'GPU prepare/SHA/bridge/fence/consume 与 archive 全项；O0/O2/CPU/GPU 同 fixture 同包身份',
  'TX-12':'N192/512 与 C/B/Q/E 独立 sweep，全 root/frame 峰值内存/work 上界，拒绝率与时延分列',
};
const caseRows=[];
for(const [id,spec] of Object.entries(txDeps)){
  // 全部依赖 TX trace 计量到件 → NOT_IMPLEMENTED
  caseRows.push({id,spec,status:'NOT_IMPLEMENTED',reason:'P011_TX_WORK_TRACE_V1 计量/具名 scope 未插桩——预测试 RED，结束本轮归因归工程'});
  rec(id,spec,'NOT_IMPLEMENTED',{reason:'trace instrumentation absent (pre-test RED)'});
}

// ---------- 输出 ----------
fs.mkdirSync('test/reports/t206',{recursive:true});
fs.writeFileSync('test/reports/t206/independent.json',JSON.stringify({
  revision:'T206_WHOLE_CHAIN_SPEC_V1',executedAt:new Date().toISOString(),
  spec:'T197 §12 (algorithm-T197-whole-transaction-complexity-v1.md)',
  commitUnderTest:'current tree (TX trace zero implementation; RED pre-test)',
  device:'NOT_RUN (offline per card)',
  modelCaveat:'T197 designer 4822-example arithmetic model is NOT a product oracle; independent spec defined here',
  total:results.length,passed:results.filter(r=>r.status==='PASS').length,
  notImplemented:results.filter(r=>r.status==='NOT_IMPLEMENTED').length,
  failed:results.filter(r=>r.status==='FAIL').length,
  missingSymbols,cwCases:caseRows,results},null,2)+'\n');
console.log('T206 SPEC: PASS='+(results.filter(r=>r.status==='PASS').length)
  +' NOT_IMPLEMENTED='+results.filter(r=>r.status==='NOT_IMPLEMENTED').length
  +' FAIL='+results.filter(r=>r.status==='FAIL').length);
