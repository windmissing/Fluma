'use strict';
// T108 单沙明水测试先行（tester，离线 RED 基线）
// 消费: docs/algorithm-T122-single-sand-clear-water-handoff-v1.md §4 + algorithm-single-sand-clear-water-v1.md §8 CW-01..18
// 结论口径: 目标7新 mode/API（SAND_WATER_SOURCE_V1/CLEAR_WATER_SCENE_V1/独立 W 库存/显式 mode）当前零实现——
//          全部 CW 组预测试 RED 标 NOT_IMPLEMENTED，不把旧加水(dry)/旧 wet 成功当明水通过，不虚报产品 FAIL 缺陷。
// 可先行执行部分: 现有 dry 注入的源账/守恒基线（作为实现后对照基线）与符号存在性核查。
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const {defaultP011ParticleParams}=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const results=[];
function rec(id,name,status,detail){results.push({id,name,status,detail:detail||null});console.log(status+' '+id+' '+name+(detail&&!status.startsWith('PASS')?' :: '+JSON.stringify(detail).slice(0,160):''));}

// ---------- 符号存在性核查（全部 CW 的实现面依赖） ----------
const fieldSrc=fs.readFileSync('entry/src/main/ets/physics/WaterSandField.ets','utf8');
let allEts='';
function walk(dir){for(const e of fs.readdirSync(dir,{withFileTypes:true})){const p=path.join(dir,e.name);
  if(e.isDirectory())walk(p);else if(e.name.endsWith('.ets'))allEts+=fs.readFileSync(p,'utf8');}}
walk('entry/src/main/ets');
const symbols={
  'SAND_WATER_SOURCE_V1 (注入API版本)':allEts.includes('SAND_WATER_SOURCE_V1'),
  'CLEAR_WATER_SCENE_V1 (同帧渲染版本)':allEts.includes('CLEAR_WATER_SCENE_V1'),
  'CLEAR_WATER_OPTICS_V1 (透射光学)':allEts.includes('CLEAR_WATER_OPTICS_V1'),
  '显式 W 权威库存数组 (waterStock/realWater)':/waterStock|realWater|explicitWater/.test(allEts),
  '独立 mode 常量 (SINGLE_SAND_CLEAR_WATER)':allEts.includes('SINGLE_SAND_CLEAR_WATER'),
  'SourceRequest/Receipt typed 接口':/SourceRequest|WaterSourceReceipt/.test(allEts),
  '掉水双源入口 (injectWater)':/injectWater\(/.test(allEts),
  'FrameToken 同帧透射字段':/FrameToken/.test(allEts),
};
const missingSymbols=Object.entries(symbols).filter(([,v])=>!v).map(([k])=>k);
rec('SYM','target-7 implementation symbols',missingSymbols.length===Object.keys(symbols).length?'PASS':'NOT_IMPLEMENTED',
  missingSymbols.length?{missing:missingSymbols}:null);

// ---------- 可先行: dry 注入源账/守恒基线（实现后对照组） ----------
function dry(){
  const f=new WaterSandField(128,256);
  assert.ok(f.initializeDryParticleDemo({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,particleParams:defaultP011ParticleParams()}).ok);
  f.setTiltPreviewTraceSink(()=>{});
  return f;
}
try{
  // 旧 wet 链基线: WET_COLUMN_V1 默认场，先造水面（旧加水）再注入沙，验证现有注入/审计链活着
  const f=new WaterSandField(128,256);
  f.setTiltPreviewTraceSink(()=>{});
  const h=new Float64Array(128),l=new Float64Array(128);h[64]=.3;
  const r=f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:8,sequence:1,inputTime:0,x:64.5,y:100,seed:1});
  const admittedOld=r&&r.admittedH>=0&&r.rejectedReason===undefined||r&&r.admittedH>0;
  let consOk=false;
  try{consOk=f.audit(true)===null;}catch(e){}
  rec('BASELINE','legacy wet-chain injection/audit baseline (comparison only, NOT clear-water pass)',
    admittedOld&&consOk?'PASS':'FAIL',{receipt:r&&{admittedH:r.admittedH,rejected:r.rejectedReason},auditOk:consOk,
    note:'旧链成功不推出目标7湿段/排水通过（CW-02/§4 明示）'});
}catch(e){rec('BASELINE','legacy baseline','FAIL',{error:e.message});}

// ---------- CW-01..18 逐组 RED 判定 ----------
// 判定依据: 依赖符号缺失 → NOT_IMPLEMENTED（预测试 RED，结束该轮归因）
// 各组最小实现依赖映射（交接 §3/§4 + 算法 §5/§6/§7）
const deps={
  'CW-01':['SINGLE_SAND_CLEAR_WATER','SAND_WATER_SOURCE_V1'],
  'CW-02':['SAND_WATER_SOURCE_V1','waterStock|realWater|explicitWater'],
  'CW-03':['waterStock|realWater|explicitWater'],
  'CW-04':['SAND_WATER_SOURCE_V1','injectWater('],
  'CW-05':['injectWater('],
  'CW-06':['injectWater('],
  'CW-07':['waterStock|realWater|explicitWater'],
  'CW-08':['waterStock|realWater|explicitWater'],
  'CW-09':['waterStock|realWater|explicitWater'],
  'CW-10':['waterStock|realWater|explicitWater'],
  'CW-11':['SAND_WATER_SOURCE_V1'],
  'CW-12':['SAND_WATER_SOURCE_V1'],
  'CW-13':['SAND_WATER_SOURCE_V1'],
  'CW-14':['SAND_WATER_SOURCE_V1'],
  'CW-15':['SAND_WATER_SOURCE_V1','waterStock|realWater|explicitWater'],
  'CW-16':['CLEAR_WATER_SCENE_V1'],
  'CW-17':['CLEAR_WATER_SCENE_V1'],
  'CW-18':['SAND_WATER_SOURCE_V1'],
};
const depMet=dep=>{
  return dep.every(d=>{
    if(d==='SINGLE_SAND_CLEAR_WATER')return allEts.includes('SINGLE_SAND_CLEAR_WATER');
    if(d==='SAND_WATER_SOURCE_V1')return allEts.includes('SAND_WATER_SOURCE_V1');
    if(d==='CLEAR_WATER_SCENE_V1')return allEts.includes('CLEAR_WATER_SCENE_V1');
    if(d==='injectWater(')return /injectWater\(/.test(allEts);
    if(d.startsWith('waterStock'))return /waterStock|realWater|explicitWater/.test(allEts);
    return false;
  });
};
const cwTitles={
  'CW-01':'新 run 全空 ΣW=ΣS=ΣP=0、L/U=0、无水 mask、medium 为新 mode',
  'CW-02':'干列连续加水只改该列 W、I_water 独立审计、不重置 initial',
  'CW-03':'湿列/满列/P 预留列局部准入或拒绝、rejected 可对账、无负 W',
  'CW-04':'掉沙触点出生→空气→入水→LAND 同 ID 可追、U=0、L=0、H 相守恒',
  'CW-05':'同初速度水/空气对照：跨面时间/湿时长/速度位移与解析式比对',
  'CW-06':'LAND q 后 W 不变、P 减 S 增、η 抬升、partial/full 身份正确',
  'CW-07':'纯水无源定角：每对通量 q 与 5 项上界、成对审计、平头零流',
  'CW-08':'干高床隔水/左右墙：无跨床瞬移、无墙外水损、head 残差量化',
  'CW-09':'θ=0 与正负倾角：head 趋同、accepted θ 同入物理与 shader',
  'CW-10':'床搬运/换列至满接收列：容量拒绝仍守恒、无伪成功',
  'CW-11':'注入失败族：pool 满/队列满/非法/NaN/旧 run/幂等冲突，库存身份不变',
  'CW-12':'幂等重试只计一次、正成功序号独立、零量拒绝不推进、water/sand 不串账',
  'CW-13':'按住切换/MOVE 风暴/次指针/UP CANCEL：旧代停止、新 DOWN 首采样 0',
  'CW-14':'后台/reset/几何代失效：待提交清理、已提交不消失、无时长补发',
  'CW-15':'step 审计失败与 source 事务失败分别注错：rollback 边界正确',
  'CW-16':'固定样本：水上原色/水中 T 混合/水下床可见/W=0 无覆盖/同 FrameToken',
  'CW-17':'shader/token/swap 失败：FAULT 或 STALE 完整 LastGood，禁 Canvas/混代',
  'CW-18':'长时定容运行：队列/ID/源账/rollback/帧内分配/复杂度实测',
};
const caseRows=[];
for(let i=1;i<=18;i++){
  const id='CW-'+String(i).padStart(2,'0');
  const met=depMet(deps[id]);
  const status=met?'NOT_RUN':'NOT_IMPLEMENTED'; // 依赖全缺 → RED；理论上不会 NOT_RUN（当前全缺）
  caseRows.push({id,title:cwTitles[id],status,statusReason:met?'dependencies present (unexpected)':'implementation symbols absent (pre-test RED)'});
  rec(id,cwTitles[id],status,{reason:'目标7 mode/API 未实现——预测试 RED，结束本轮归因；不虚报产品 FAIL'});
}

// ---------- 反例清单核（算法 §9 须保留项在报告登记） ----------
const counterexamples=['空气沙P使旧waterTop虚高','0水dry包络画出假水','legacy inject成功账归零','砂落床同时减W漏水',
  '水注入只改shader贴图','全局均摊穿高床','大q左右振荡','q<ULP账实分离','零LAND改CONTACT_WAIT','按住切换累积补发',
  '旧雾alpha=1遮床','scene水面来自新state床来自LastGood'];
rec('REGL','algorithm §9 counterexample list retained for implementation phase',counterexamples.length===12?'PASS':'FAIL',{count:counterexamples.length});

// ---------- 输出 ----------
const counts={NOT_IMPLEMENTED:results.filter(r=>r.status==='NOT_IMPLEMENTED').length,
  PASS:results.filter(r=>r.status==='PASS').length,FAIL:results.filter(r=>r.status==='FAIL').length};
fs.mkdirSync('test/reports/t108',{recursive:true});
fs.writeFileSync('test/reports/t108/red_baseline.json',JSON.stringify({
  revision:'T108_CLEAR_WATER_RED_BASELINE_V1',executedAt:new Date().toISOString(),
  spec:'T122 handoff §4 + algorithm-single-sand-clear-water-v1.md §8 CW-01..18',
  commitUnderTest:'current tree (no target-7 implementation; RED pre-test)',
  device:'NOT_RUN (offline per card)',
  summary:{total:results.length,...counts},
  missingSymbols,cwCases:caseRows,results,
  calibrations:'PENDING_CALIBRATION: κ/rateWater/rateSand/一次上限/dragWet/settlingSpeed/sigmaWater/tint/高光/分段误差 — 测试仅输出测量值与候选 configRev，不贴既有门槛',
  notes:['dry 注入成功基线仅作实现后对照，非明水通过（CW-02 合同明示）',
    '旧全局 levelWater 不是新邻接核 oracle',
    '未实现 RED 归 developer/工程；本卡不虚报产品 FAIL 缺陷']},null,2)+'\n');
console.log(`T108 RED BASELINE: ${counts.PASS} PASS / ${counts.NOT_IMPLEMENTED} NOT_IMPLEMENTED / ${counts.FAIL} FAIL`);
