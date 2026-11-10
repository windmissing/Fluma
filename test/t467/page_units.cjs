const fs=require('node:fs'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const project=process.argv[2];
function load(s,scope={}){const text=ts.transpileModule(s,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText;const m={exports:{}};new Function('exports','module',...Object.keys(scope),text)(m.exports,m,...Object.values(scope));return m.exports;}
const helper=fs.readFileSync(project+'/entry/src/main/ets/physics/GpuRecoveryPulse.ets','utf8');
const {GpuRecoveryPulse}=load(helper);let calls=[],receipt={resetSerial:'0',epoch:'1',runLo:0,runHi:0,phase:'FAULTED_REVOKED',windowAvailable:false};
const port={recoveryState:()=>JSON.stringify(receipt),newRun:()=>{receipt={...receipt,resetSerial:String(Number(receipt.resetSerial)+1),epoch:String(Number(receipt.epoch)+1),phase:'RESET_REQUESTED'};calls.push('reset');return 'NEW_RUN_REQUESTED';},beginRecoverySession:(...a)=>{calls.push(a);return a[0]==receipt.resetSerial&&a[1]==receipt.epoch?'11':'0';},pagePulse:(...a)=>{calls.push(a);return 'PAGE_PULSE_ACCEPTED';}};
let count=0;
const pulse=new GpuRecoveryPulse(port,'1');assert.equal(pulse.pulse(100),'WAIT_WINDOW');assert.equal(calls.length,0);++count;
receipt.windowAvailable=true;assert.equal(pulse.pulse(200),'RESET_REQUESTED');assert.equal(calls.length,1);++count;
assert.equal(pulse.pulse(39000000200),'RESET_REQUESTED');assert.equal(calls.length,1);++count;
receipt={...receipt,phase:'WAIT_NEW_SESSION',runLo:7};assert.equal(pulse.pulse(39000000300),'PAGE_PULSE_ACCEPTED');assert.deepEqual(calls[1],[receipt.resetSerial,receipt.epoch,'7','0','1']);assert.deepEqual(calls[2],['11','1','39000000300']);++count;
assert.equal(pulse.pulse(39000000400),'PAGE_PULSE_ACCEPTED');assert.deepEqual(calls[3],['11','2','39000000400']);++count;
receipt.resetSerial='9';const old=calls.length;assert.equal(pulse.pulse(39000000500),'STALE_RESET_RECEIPT');assert.equal(pulse.pulse(39000000600),'STALE_FRAME');assert.equal(calls.length,old);++count;
for(const page of ['GpuResidentM1','GpuSandWaterExperience']){
 const src=fs.readFileSync(project+'/entry/src/main/ets/pages/'+page+'.ets','utf8');
 const name=page==='GpuResidentM1'?'M1DemoFrame':'ExperienceFrame';
 const begin=src.indexOf('class '+name),end=src.indexOf('@Entry',begin);
 let body=src.slice(begin,end); // Includes only the ordinary frame class and interfaces.
 const {Frame}=load('let recoveryPageSerial=0;\n'+body+'\nexport const Frame='+name+';', {FrameCallback:class{},GpuRecoveryPulse,m1:port});
 const frame=new Frame();let ticks=0;frame.owner={tick:()=>++ticks};receipt.phase='WAIT_NEW_SESSION';frame.pulse.bindRequested();frame.onFrame(1);assert.equal(ticks,1);++count;
 frame.pulse.stop();frame.owner=null;const before=calls.length;frame.onFrame(2);assert.equal(calls.length,before);assert.equal(ticks,1);++count;
 assert.match(src,/pulse\.stop\(\)/);assert.doesNotMatch(src,/session\s*=\s*m1.beginPageSession\(\)/);++count;
}
console.log(JSON.stringify({schema:'T467_REAL_PAGE_FRAME_CONTROLS_V1',passed:count,failed:0}));
