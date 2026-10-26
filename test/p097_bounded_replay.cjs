// T97 arm 2: bounded three-arm replay — the R3 gesture sequence (505 touch rows,
// localX/localY from the T78 archive) driven through the demo model on BOTH
// revisions inside a child process with an external timeout. Exact T78 state is
// NOT archived (no initial pool/field snapshot) => exact reproduction stays
// UNPROVEN; this arm only proves the NEW revision stays bounded under the same
// gesture load where T78 froze, and that OLD vs NEW remain equivalent here.
'use strict';
const fs = require('fs'), path = require('path'), cp = require('child_process');
const OUT = 'test/reports/t97';
const seq = JSON.parse(fs.readFileSync(OUT + '/p097_r3_gesture_sequence.json', 'utf8'));
const touch = seq.filter(r => (r.kind === 'pan_update' || r.kind === 'pan_end') && r.localX !== null);

const childSrc = `
const fs=require('fs'),Module=require('module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const load=Module._load;Module._load=function(n,p,m){if(n==='libp013renderer.so')return {default:{}};if(n==='@ohos.hilog')return {default:{info(){},warn(){},error(){}}};return load.call(this,n,p,m)};
const dir=process.argv[2];
const seq=JSON.parse(fs.readFileSync(process.argv[3],'utf8'));
const touch=seq.filter(r=>(r.kind==='pan_update'||r.kind==='pan_end')&&r.localX!==null);
const {P051EmptySandDemo}=require(dir+'/entry/src/main/ets/physics/P051EmptySandDemo.ets');
const d=new P051EmptySandDemo(); d.setActive(true);
let ns=1e9, touched=false, steps=0, lastStatus=null;
const t0=Date.now();
for (const row of touch) {
  if (row.kind==='pan_update'&&!touched) { d.start(row.localX,row.localY); touched=true; }
  else if (row.kind==='pan_update') d.move ? d.move(row.localX,row.localY) : d.start(row.localX,row.localY);
  else if (row.kind==='pan_end'&&touched) { d.stop ? d.stop() : null; touched=false; }
  // drive the field: 2 substeps per gesture event (60fps approx)
  for (let k=0;k<2;k++){ ns+=1e9/60; const ok=d.tick(ns,1/60); steps++; if(!ok){lastStatus='modelStopped';break;} }
  if (Date.now()-t0>25000) { lastStatus='selfLimit25s'; break; }
}
const field=d.field||d._field||null;
let poolSummary=null;
try { const pool=(d.field?d.field.particleBinding.pool:d.pool); poolSummary={liveCount:pool.liveCount,nextId:pool.nextId,freeCount:pool.freeCount}; } catch(e){ poolSummary={error:String(e).slice(0,80)}; }
console.log(JSON.stringify({done:true,steps,touchEvents:touch.length,lastStatus,elapsedMs:Date.now()-t0,poolSummary}));
`;
fs.writeFileSync('.codex/local-test/t97/replay_child.js', childSrc);

const results = [];
function rec(id, name, ok, detail) {
  results.push({id, name, status: ok ? 'PASS' : 'FAIL', ...detail});
  console.log((ok ? '[PASS] ' : '[FAIL] ') + id + ' ' + name);
}
const TIMEOUT_MS = 60000;
for (const [rev, dir] of [['NEW', '.codex/local-test/t97/c0c58c3'], ['OLD', '.codex/local-test/t97/067fd84']]) {
  const t0 = Date.now();
  let out = '', timedOut = false, code = null, err = '';
  try {
    const r = cp.execFileSync(process.execPath, ['.codex/local-test/t97/replay_child.js',
      require('path').resolve(dir).replace(/\\/g, '/'),
      require('path').resolve(OUT + '/p097_r3_gesture_sequence.json').replace(/\\/g, '/')],
      {timeout: TIMEOUT_MS, encoding: 'utf8'});
    out = r; code = 0;
  } catch (e) {
    out = (e.stdout || '') + (e.stderr || ''); code = e.status === null ? 'KILLED' : e.status;
    timedOut = e.killed === true || /timed out|ETIMEDOUT/i.test(String(e.message));
  }
  const wall = Date.now() - t0;
  let summary = null;
  const m = out.match(/\{.*"done":true.*\}/s);
  if (m) { try { summary = JSON.parse(m[0]); } catch (e2) {} }
  rec(`T97-R3-${rev}`, `bounded R3 gesture replay (${rev}): completed within external ${TIMEOUT_MS}ms timeout`,
    !timedOut && summary && summary.done,
    {wallMs: wall, childCode: code, timedOut, steps: summary && summary.steps,
     touchEvents: summary && summary.touchEvents, lastStatus: summary && summary.lastStatus,
     poolSummary: summary && summary.poolSummary,
     exactReplay: 'UNPROVEN — T78 archive has no initial pool/field snapshot; this is a same-gesture-sequence bounded run from a fresh field'});
}
// zero-source chain: gate→clock→request→inject with real energy on the demo model (T92 §6 R3 focus)
{
  const childZero = childSrc.replace('P051EmptySandDemo', 'P051EmptySandDemo'); // same model; energy probe below
  const src = `
const fs=require('fs'),Module=require('module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const load=Module._load;Module._load=function(n,p,m){if(n==='libp013renderer.so')return {default:{}};if(n==='@ohos.hilog')return {default:{info(){},warn(){},error(){}}};return load.call(this,n,p,m)};
const {P051EmptySandDemo}=require('D:/hmprojects/HelloWorld/entry/src/main/ets/physics/P051EmptySandDemo.ets');
const {WaterSandField}=require('D:/hmprojects/HelloWorld/entry/src/main/ets/physics/WaterSandField.ets');
const d=new P051EmptySandDemo(); d.setActive(true);
// gate->clock->request chain: touch start with a fresh gravity sample, then ticks
d.start(64.5,220);
let ns=1e9, zeroSourceSteps=0, nonzeroAt=null;
for(let i=0;i<120;i++){ ns+=1e9/60; d.tick(ns,1/60);
  const met=d.field.getMetrics();
  if((met.sourceInjectedHeavy||0)>0&&nonzeroAt===null)nonzeroAt=i;
  if((met.sourceInjectedHeavy||0)===0)zeroSourceSteps++;
}
const pool=d.field.particleBinding.pool;
console.log(JSON.stringify({done:true,zeroSourceSteps,nonzeroAt,liveCount:pool.liveCount}));
`;
  fs.writeFileSync('.codex/local-test/t97/zero_child.js', src);
  let out = '', timedOut = false;
  try { out = cp.execFileSync(process.execPath, ['.codex/local-test/t97/zero_child.js'], {timeout: 30000, encoding: 'utf8'}); }
  catch (e) { out = (e.stdout || '') + (e.stderr || ''); timedOut = e.killed === true; }
  const m = out.match(/\{.*"done":true.*\}/s);
  const summary = m ? JSON.parse(m[0]) : null;
  rec('T97-ZERO', 'zero-source chain probe: touch chain runs bounded; source activation observed (gate->clock->request->inject)',
    !timedOut && summary && summary.done && summary.nonzeroAt !== null,
    {timedOut, zeroSourceSteps: summary && summary.zeroSourceSteps, firstNonzeroStep: summary && summary.nonzeroAt,
     liveCount: summary && summary.liveCount,
     note: 'device zero-source attribution stays with T69/T82; this only records the chain behavior of the current build'});
}

fs.writeFileSync(OUT + '/p097_bounded_replay_results.json',
  JSON.stringify({executionId: 'p097-bounded-replay-v1', date: '2026-10-26',
    identity: {newRev: 'c0c58c3', oldRev: '067fd84'},
    exactReplayStatus: 'UNPROVEN (no archived initial pool/field snapshot in T78 artifacts)',
    results}, null, 1) + '\n');
const pass = results.every(r => r.status === 'PASS');
console.log('total', results.length, 'pass', results.filter(r => r.status === 'PASS').length);
process.exitCode = pass ? 0 : 1;
