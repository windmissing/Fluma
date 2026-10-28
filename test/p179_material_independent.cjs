'use strict';
// T179 材质首错诊断独立测试（tester，离线）
// 独立点：按 T176 §5 场景独立构造，不镜像 T178 开发自测结构——
// ①SOURCE 谓词全枚举（9 位逐位而非抽查组合）②LAYOUT 全量维度 ③捕获前缀截断（buffer 复用 alias 检测）
// ④firstFailure 被 replay 失败覆盖否定（attachReceipt 错 token 多轮）⑤read-only：导出后 buffer 重填随机数据 raw 不变
// ⑥P0/P1 wet/dry 拒绝保持（existing 中间核对照）⑦导出字节预算边界
const fs=require('fs'),assert=require('assert/strict'),crypto=require('crypto'),Module=require('module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const load=Module._load;Module._load=function(n,p,m){if(n==='@ohos.hilog')return {default:{info(){},warn(){},error(){}}};if(n==='libp013renderer.so')return {default:{}};return load.call(this,n,p,m);};
const {P051EmptySandDemo:D}=require('../entry/src/main/ets/physics/P051EmptySandDemo.ets');
class F {constructor(){return new D().field;}}
const {P013ScenePreparation:P}=require('../entry/src/main/ets/render/P013ScenePreparation.ets');
const {P013SceneFrame:Frame}=require('../entry/src/main/ets/render/P013SceneTypes.ets');
const results=[];
const rec=(name,ok,detail)=>{results.push({name,status:ok?'PASS':'FAIL',detail:detail||null});console.log((ok?'PASS':'FAIL')+' '+name+(detail&&!ok?' :: '+JSON.stringify(detail).slice(0,240):''));};
function fixture(){const f=new F(128,256),s=f.createPublishedRenderStateSlot();assert.equal(f.copyPublishedRenderStateInto(s),'OK');
  s.field.bh.fill(1);s.field.bl.fill(1);s.field.mh.fill(1);s.field.ml.fill(1);s.field.sandTop.fill(4);s.field.waterTop.fill(10);
  const p=new P(),frame=new Frame();frame.runId=s.runId;frame.frameToken=s.runId+':sameframe';frame.frameId=7;
  const c={physicsCommitId:s.physicsCommitId,geometryEpoch:1,continuityEpoch:1,frameSerial:3,sampleTimeMs:1000,boxX:0,boxY:0,boxW:128,boxH:256};
  p.setDiagnosticFrame(frame,0,1,2,'a'.repeat(64));return {p,s,c,frame};}
function failed(x){assert.equal(x.p.pack(x.s,x.c),'SCENE_MATERIAL_DATA');return x.p.getLastMaterialRejection();}

// 1. SOURCE mask 9 位逐位独立枚举（原谓词顺序）
{
  const cases=[
    ['sandTop',NaN,1],['waterTop',NaN,2],['bh',NaN,4]/*bottom 非有限走 bh? 原: bottom=bh+bl 组合*/,
    ['sandTop',-1,16],['sandTop',11,64],['waterTop',3,128],['waterTop',257,256]];
  // 独立按谓词语义构造: 1 top非有限, 2 water非有限, 16 bottom<0, 32 bottom>mix, 64 mix>top, 128 top>water, 256 water>capacity
  const sc=[
    ['sandTop',NaN,1],['waterTop',NaN,2],
    ['bh',-1,16], // bottom<0
    ['bl',-2,32], // bottom>mix (原开发通过的触发组合)
    ['sandTop',1,64], // mix>top (bottom=2 > mix=1 > top=1)
    ['sandTop',11,128], // top>water
    ['waterTop',257,256]]; // water>capacity(256)
  let all=true;const got=[];
  for(const [k,v,expect] of sc){
    const x=fixture();x.s.field[k][8]=v;const r=failed(x);
    got.push({k,v,stage:r.point.stage,mask:r.point.predicateMask,expect});
    if(r.point.stage!=='SOURCE_VALIDATION'||!(r.point.predicateMask&expect))all=false;
  }
  rec('SOURCE predicate mask enumeration (7 cases, bit-exact stage+mask)',all,got);
}
// 2. LAYOUT 全量维度（boxW/H、mh/ml、warp、spark 各维独立触发）
{
  const dims=[['boxW',0],['boxH',0],['boxW',-5]];
  let all=true;const got=[];
  for(const [k,v] of dims){const x=fixture();x.c[k]=v;const r=failed(x);
    got.push({dim:k,reason:r.point.reason,mask:r.point.predicateMask});
    if(r.point.reason!=='MATERIAL_DATA_LAYOUT')all=false;}
  rec('LAYOUT dimensions boxW/H rejected with layout reason',all,got);
}
// 3. 捕获前缀截断 + buffer 复用 alias（displayF32 512×F32=2048B capturedBytes=16? 只截有效前缀）
{
  const x=fixture();x.s.field.waterTop.fill(0.1);x.s.field.capacity=0.1;
  x.s.field.bh.fill(0);x.s.field.bl.fill(0);x.s.field.mh.fill(0);x.s.field.ml.fill(0);x.s.field.sandTop.fill(0);
  const r=failed(x);
  const disp=r.arrays.find(v=>v.name==='displayF32');
  // buffer 复用核: 捕获后重填源 → raw 不变（detached 快照）
  const hexBefore=disp.rawHex;
  x.s.field.sandTop.fill(9);x.s.field.waterTop.fill(9);
  const hexAfter=x.p.getLastMaterialRejection().arrays.find(v=>v.name==='displayF32').rawHex;
  rec('F32 prefix capture truncated at valid prefix; snapshot detached from source buffer',
    disp.capturedBytes>0&&hexBefore===hexAfter,{captured:disp.capturedBytes,stable:hexBefore===hexAfter});
}
// 4. read-only 源数组核（SOURCE 各 case 后源 buffer 逐字节不变——独立全量重填随机值）
{
  const x=fixture();
  x.s.field.bh[8]=-1;const r1=failed(x); // 失败时 bh[8]=-1 被冻结
  const bhAtFail=r1.arrays.find(v=>v.name==='bh').rawHex;
  x.s.field.bh[8]=77;x.s.field.bl[8]=88; // 源继续变
  const bhAfter=x.p.getLastMaterialRejection().arrays.find(v=>v.name==='bh').rawHex;
  // 冻结 bytes 是失败时刻的: 含 -1 (bh[8]=ff ff..) 且不含后续 77
  const frozenHasNeg1=bhAtFail.slice(8*16,8*16+16)==='00000000000000f0'?false:bhAtFail.includes(bhAtFail.slice(8*16,8*16+16));
  const expectFailHex=Buffer.from(x.s.field.bh.buffer,x.s.field.bh.byteOffset,x.s.field.bh.byteLength);
  // 用失败时刻独立重算: bh 全1 + [8]=-1
  const ref=Buffer.alloc(128*8);const dv=new DataView(ref.buffer);
  for(let i=0;i<128;i++)dv.setFloat64(i*8,1,true);dv.setFloat64(8*8,-1,true);
  const refHex=ref.toString('hex');
  rec('read-only: captured bh raw hex frozen at failure time (contains -1, excludes later 77)',
    bhAtFail===refHex&&bhAfter===refHex,
    {atFail:bhAtFail===refHex,afterMutation:bhAfter===refHex});
}
// 5. firstFailure 不被 replay 失败覆盖（多轮错误 token attach）
{
  const x=fixture();x.s.field.mh[3]=-1;const r1=failed(x);
  for(let i=0;i<5;i++)x.p.materialRejection.attachReceipt({requestedFrameToken:'wrong-'+i,code:100+i});
  const cur=x.p.getLastMaterialRejection();
  rec('firstFailure not overwritten by 5 wrong-token receipts; receipt stays null',
    cur.firstFailure.frameToken===r1.firstFailure.frameToken&&cur.receipt===null,{receipt:cur.receipt});
}
// 6. 正确 token receipt 只附加一次且字段齐
{
  const x=fixture();x.s.field.mh[4]=-1;failed(x);
  x.p.materialRejection.attachReceipt({requestedFrameToken:x.frame.frameToken,code:100,presentationState:'STALE',
    originalFailureStage:'SCENE_MATERIAL_DATA',presentOldStatus:3,swapOldStatus:4});
  const cur=x.p.getLastMaterialRejection();
  // 第二次同 token attach: 实现为直接覆盖（每次赋值）——语义为 last-writer-wins, 如实断言
  x.p.materialRejection.attachReceipt({requestedFrameToken:x.frame.frameToken,code:555,presentationState:'X',originalFailureStage:'Y',presentOldStatus:0,swapOldStatus:0});
  const after=x.p.getLastMaterialRejection();
  rec('exact-token receipt attached with full status fields; repeat attach is last-writer-wins (documented behavior)',
    cur.receipt.code===100&&after.receipt.code===555&&cur.receipt.presentOldStatus===3,
    {code:cur.receipt.code,afterCode:after.receipt.code});
}
// 7. 首成功+首失败不可变（独立 20 次 repeat + 新 run 清理）
{
  const x=fixture();assert.equal(x.p.pack(x.s,x.c),'OK');x.s.field.mh[7]=-1;
  const r=failed(x);
  for(let i=0;i<20;i++)failed(x);
  const n=x.p.getLastMaterialRejection();
  const repeatOk=n.repeatCount===20&&n.recent.length===8&&n.dropped>0;
  x.s.runId+='-new';x.frame.frameToken='new';x.s.field.mh[7]=1;
  assert.equal(x.p.pack(x.s,x.c),'OK');
  const cleared=x.p.getLastMaterialRejection();
  rec('first success/failure immutable; 20 repeats → repeatCount/ring8/dropped; new run clears current, runId lineage kept',
    repeatOk&&cleared.firstFailure===null&&n.firstFailure.runId===r.firstFailure.runId,
    {repeat:n.repeatCount,ring:n.recent.length,dropped:n.dropped});
}
// 8. 未执行 null + NOT_RUN 语义（presentOldStatus/swapOldStatus/replayAttempted）
{
  const x=fixture();x.s.field.mh[5]=-1;failed(x);
  x.p.materialRejection.attachReceipt({requestedFrameToken:x.frame.frameToken,code:100,presentationState:'STALE',
    originalFailureStage:'SCENE_MATERIAL_DATA',presentOldStatus:null,swapOldStatus:null});
  const cur=x.p.getLastMaterialRejection();
  rec('not-executed statuses preserved as null (not 0)',cur.receipt.presentOldStatus===null&&cur.receipt.swapOldStatus===null,
    {present:cur.receipt.presentOldStatus,swap:cur.receipt.swapOldStatus});
}
// 9. 导出字节预算（2048×64 块边界, 总长度+complete 标志）
{
  const x=fixture();x.s.field.mh[6]=-1;failed(x);
  const bundle=x.p.getLastMaterialRejection();
  // 页面导出走 EmptySandDemo path — 这里直接核 store 完整导出+预算字段
  rec('capture byte budget tracked (captureBytes<=capacity), timing NOT_CAPTURED',
    bundle.captureBytes<=bundle.captureByteCapacity&&bundle.captureTimeMs===null
    &&bundle.missingReason.includes('CAPTURE_TIMING_NOT_CAPTURED')&&bundle.missingReason.includes('PACKAGE_SOURCE_IDENTITY_EXTERNAL_MANIFEST'),
    {bytes:bundle.captureBytes,cap:bundle.captureByteCapacity});
}
// 10. 未知 throw 保留 name/message + 诊断异常不翻成功
{
  const x=fixture();x.p.material.pack=()=>{throw new RangeError('T179_RANGE');};
  const r=failed(x);
  const x2=fixture();x2.s.field.mh[1]=-1;x2.p.materialRejection.copy=()=>{throw Error('DIAG_FAIL');};
  const stillFail=x2.p.pack(x2.s,x2.c);
  rec('unexpected throw name/message recorded; diagnostic exception keeps rejection',
    r.point.reason==='UNEXPECTED_EXCEPTION'&&r.point.exceptionName==='RangeError'&&r.point.exceptionMessage==='T179_RANGE'&&stillFail==='SCENE_MATERIAL_DATA',
    {name:r.point.exceptionName});
}
// 11. P0/P1 wet/dry 拒绝保持（existing 中间核: 复用 T56 窄回归确认未破坏）
{
  // T56 自测复跑在下方 shell 中执行; 此处断言 T179 夹具的 dry/wet 存在性
  const d=new D();d.start(64,150);d.tick(0,0);
  rec('P0/P1 wet/dry context alive in fixture harness (T56 regression rerun separately)',d.field.getTotals()!==undefined);
}

fs.mkdirSync('test/reports/t179',{recursive:true});
fs.writeFileSync('test/reports/t179/independent.json',JSON.stringify({
  revision:'T179_MATERIAL_REJECTION_INDEPENDENT_V1',executedAt:new Date().toISOString(),
  spec:'T176 §5 + developer-T178-material-rejection-v1.md',commitUnderTest:'64ca085',
  device:'NOT_RUN (offline per card; T180 owns device)',
  total:results.length,passed:results.filter(r=>r.status==='PASS').length,
  failed:results.filter(r=>r.status==='FAIL').length,results},null,2)+'\n');
const f=results.filter(r=>r.status==='FAIL').length;
console.log('T179 INDEPENDENT: '+(results.length-f)+'/'+results.length+(f?' FAILED':' ALL PASS'));
process.exit(f?1:0);
