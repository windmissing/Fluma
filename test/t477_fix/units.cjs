const fs = require('node:fs');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const ts = require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const project = process.argv[2];
const pages = fs.readFileSync(process.argv[3], 'utf8').trim().split(/\r?\n/).map(JSON.parse);
function load(source) {
  const m = {exports:{}};
  const text = ts.transpileModule(source, {compilerOptions:{
    module:ts.ModuleKind.CommonJS, target:ts.ScriptTarget.ES2020
  }}).outputText;
  new Function('exports','module',text)(m.exports,m);
  return m.exports;
}
const {GpuBridgeTimelineCold} = load(fs.readFileSync(project+'/entry/src/main/ets/physics/GpuBridgeTimelineCold.ets','utf8'));
const {GpuRecoveryPulse} = load(fs.readFileSync(project+'/entry/src/main/ets/physics/GpuRecoveryPulse.ets','utf8'));
const cases = [];
const identity = {module:'libgrsm1.so',route:'GpuResidentM1',packageSha256:'a'.repeat(64),pid:1};
async function collect(change=()=>{}) {
  const saved = new Map();
  const result = await new GpuBridgeTimelineCold().collect({
    status:()=> '{}',
    timeline:cursor=>{
      const page = structuredClone(pages[cursor/4]);
      change(page);
      return JSON.stringify(page);
    }
  }, {save:async(name,text)=>{
    saved.set(name,text);
    return {name,bytes:Buffer.byteLength(text),sha256:crypto.createHash('sha256').update(text).digest('hex')};
  }}, identity, async()=>{});
  return {result,saved};
}
async function check(name,body) {await body();cases.push(name);}
function pulseFixture() {
  let receipt={resetSerial:'1',epoch:'3',runLo:1,runHi:0,phase:'WAIT_NEW_SESSION',reason:'',windowAvailable:true};
  let calls=0,resets=0;
  const port={recoveryState:()=>JSON.stringify(receipt),
    newRun:()=>{resets++;return 'NEW_RUN_REQUESTED';},
    beginRecoverySession:()=> '11',
    pagePulse:()=>{calls++;return 'PAGE_PULSE_ACCEPTED';}};
  return {port,receipt,get calls(){return calls;},get resets(){return resets;}};
}
(async()=>{
  await check('native stage66/67 export crosses the former 120-row boundary',async()=>{
    const {result}=await collect();
    assert.equal(result.state,'COMPLETE');
    assert.equal(result.rows,124);assert.equal(result.pages,31);
    assert.deepEqual(result.stages.slice(66),['BatchFlushEnter','BatchFlushExit']);
    assert.equal(result.rejectedPage,null);
  });
  await check('old dictionary still rejects raw141 and preserves the exact rejected page',async()=>{
    const {result,saved}=await collect(p=>{
      p.stageRevision='T404_STAGE65_V1';
      if(p.cursor===120){p.rows[0].stage=141;p.rows[1].stage=142;}
    });
    assert.equal(result.rows,120);assert.equal(result.state,'INCOMPLETE');
    assert.match(result.reason,/INVALID_ROW/);assert(result.rejectedPage);
    const raw=saved.get(result.rejectedPage.name);
    assert.equal(JSON.parse(raw).cursor,120);assert.equal(JSON.parse(raw).rows[0].stage,141);
    assert.equal(crypto.createHash('sha256').update(raw).digest('hex'),result.rejectedPage.sha256);
  });
  await check('new dictionary rejects unknown68 and retains rejected bytes',async()=>{
    const {result}=await collect(p=>{if(p.cursor===120)p.rows[0].stage=68;});
    assert.equal(result.rows,120);assert.match(result.reason,/INVALID_ROW/);assert(result.rejectedPage);
  });
  await check('native dropped stays INCOMPLETE with the full readable prefix',async()=>{
    const {result}=await collect(p=>{p.dropped='1';p.incomplete=true;});
    assert.equal(result.rows,124);assert.match(result.reason,/NATIVE_DROPPED/);
  });
  await check('same-session native revoke reports original GAP and stops old pulses',()=>{
    const x=pulseFixture(),pulse=new GpuRecoveryPulse(x.port,'1');
    pulse.bindRequested();assert.equal(pulse.pulse(1),'PAGE_PULSE_ACCEPTED');
    x.receipt.phase='FAULTED_REVOKED';x.receipt.reason='V2_PULSE_CLOCK_GAP';
    for(let i=2;i<102;i++)assert.equal(pulse.pulse(i),'V2_PULSE_CLOCK_GAP');
    assert.equal(x.calls,1);assert.equal(x.resets,0);
  });
  await check('revoke between state read and pulse preserves native reason',()=>{
    const x=pulseFixture(),pulse=new GpuRecoveryPulse(x.port,'1');
    pulse.bindRequested();
    x.port.pagePulse=()=>{x.receipt.phase='FAULTED_REVOKED';x.receipt.reason='GPU_PROGRESS_TIMEOUT';return 'STALE_PAGE_SESSION';};
    assert.equal(pulse.pulse(1),'GPU_PROGRESS_TIMEOUT');assert.equal(pulse.pulse(2),'GPU_PROGRESS_TIMEOUT');
    assert.equal(x.resets,0);
  });
  await check('explicit new frame rebinds a new reset; an old frame cannot send',()=>{
    const x=pulseFixture(),old=new GpuRecoveryPulse(x.port,'1');
    old.bindRequested();assert.equal(old.pulse(1),'PAGE_PULSE_ACCEPTED');old.stop();
    x.receipt.resetSerial='2';x.receipt.epoch='4';x.receipt.runLo=2;
    const next=new GpuRecoveryPulse(x.port,'2');next.bindRequested();
    assert.equal(old.pulse(2),'STALE_FRAME');assert.equal(next.pulse(3),'PAGE_PULSE_ACCEPTED');
    assert.equal(x.calls,2);
  });
  await check('genuine native CLOCK_GAP is terminal without new session or reset',()=>{
    const x=pulseFixture(),pulse=new GpuRecoveryPulse(x.port,'1');pulse.bindRequested();
    x.port.pagePulse=()=> 'CLOCK_GAP';
    assert.equal(pulse.pulse(1),'CLOCK_GAP');
    x.port.pagePulse=()=>{throw Error('REVOKED_TOKEN_REUSED');};
    assert.equal(pulse.pulse(2),'CLOCK_GAP');assert.equal(x.resets,0);
  });
  await check('offline reader recognizes bounded enter event with no fabricated exit',()=>{
    const result=require('../t384/read.cjs').analyze(pages);
    assert(!result.faults.includes('DICTIONARY'));assert(!result.faults.includes('BAD_PROBE_FIELDS'));
  });
  console.log(JSON.stringify({schema:'T477_FIX_UNITS_V1',passed:cases.length,failed:0,cases,device:'NOT_RUN'}));
})().catch(error=>{console.error(error);process.exitCode=1;});
