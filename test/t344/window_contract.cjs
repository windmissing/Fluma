'use strict';
// T344 window-contract tests: the narrowed capture window bills ONLY T284Cold's
// own capture actions. Simulates the exact device page structure (t340-v1
// P051DerivedV2Demo tick): select() -> 20ms of caller business (authority
// capture + full render pipeline + T266_FRAME JSON build) -> enqueue -> finish.
// W1 zero-business-window: captureMs <= 2ms when only capture actions run.
// W2 device-shape window: 20ms of caller work before the first enqueue does
// NOT bill to captureMs (the T340 failure shape: 18-34ms, rawBytes=0).
// W3 in-window work still trips BUDGET_CAPTURE_MS (fail-closed unchanged).
// W4 no-capture-action anchor: captureMs=0, NOT_CAPTURED (no false over-budget).
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const Module=require('module'),load=Module._load;Module._load=function(name,parent,main){if(name==='@ohos.hilog')return {default:{info:()=>{}}};return load.call(this,name,parent,main);};
const path=require('path'),assert=require('assert/strict');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(require('fs').readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.resolve('.codex/local-test/t344-final/entry/src/main/ets');
const {T284Cold}=require(base+'/diagnostics/t248/T284Cold.ets');
let passed=0;
function cold(){const b=new T284Cold(),lines=[];let time=0;b.now=()=>time;b.emit=l=>lines.push(l);b.enabled=true;b.bind('r',1);return {b,lines,tick:d=>time+=d};}
// W1: zero business work inside the window
{const {b,lines,tick}=cold();b.signal('FIRST_BED',1);b.select(1,1);b.enqueue('FRAME','x'.repeat(2000));tick(0.01);b.enqueue('PACK','y'.repeat(500));b.finish();
 const idx=JSON.parse(b.summary()).indices[0];
 assert.equal(idx.status,'PENDING'); // COMPLETE requires drain(); capture itself succeeded
 assert.equal(idx.parts,10);assert.equal(idx.rawBytes,5000); // 2000+500 chars x2 bytes
 assert(idx.captureMs<=2,'W1 zero-business captureMs<=2, got '+idx.captureMs);
 passed++;}
// W2: the T340 device shape — 20ms of caller work between select and enqueue
{const {b,lines,tick}=cold();b.signal('FIRST_BED',1);b.select(1,1);
 tick(20); // authority capture + render pipeline + T266_FRAME JSON build
 b.enqueue('FRAME','x'.repeat(2000));b.finish();
 const idx=JSON.parse(b.summary()).indices[0];
 assert(idx.captureMs<5,'W2 caller work must not bill: got '+idx.captureMs);
 assert(!lines.some(l=>l.includes('BUDGET_CAPTURE_MS')),'W2 must not trip over the 20ms caller work');
 assert.equal(idx.status,'PENDING'); // no BUDGET_CAPTURE_MS rejection
 assert.equal(idx.parts,8);assert.equal(idx.rawBytes,4000);
 passed++;}
// W3: in-window delay still trips fail-closed
{const {b,lines,tick}=cold();b.signal('FIRST_BED',1);b.select(1,1);b.enqueue('FRAME','x'.repeat(100));tick(5);b.enqueue('PACK','y');b.finish();
 assert(lines.some(l=>l.includes('BUDGET_CAPTURE_MS')),'W3 in-window 5ms must trip');
 passed++;}
// W4: anchor selected but no capture action at all
{const {b,lines}=cold();b.signal('FIRST_BED',1);b.select(1,1);b.finish();
 const idx=JSON.parse(b.summary()).indices[0];
 assert.equal(idx.captureMs,0,'W4 no-action window is 0');
 assert.equal(idx.status,'INCOMPLETE');assert.equal(idx.reason,'NOT_CAPTURED');
 passed++;}
const report={revision:'T344_WINDOW_CONTRACT_V1',passed,failed:0,device:'NOT_RUN',windowSemantics:'arms-at-first-enqueue'};
require('fs').mkdirSync('test/reports/t344',{recursive:true});
require('fs').writeFileSync('test/reports/t344/window-contract.json',JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report));
