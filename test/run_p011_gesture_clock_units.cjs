const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P011GestureRateClock:Clock}=require('../entry/src/main/ets/physics/P011GestureRateClock.ets');
const results=[];function test(name,fn){try{fn();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:String(e)});}}
function mass(unit,segments){const c=new Clock();c.begin();let mass=0;const scale=unit==='ns'?1e6:1;c.sample(1000*scale,unit);for(let i=1;i<=segments;i++)mass+=8*c.sample((1000+1000*i/segments)*scale,unit);return mass;}
test('ns and explicit ms adapters integrate the same 8 units over one second',()=>{for(const u of ['ns','ms'])for(const n of [1,10,43,60,120])assert(Math.abs(mass(u,n)-8)<1e-10);});
test('first event never injects, including zero boot timestamp',()=>{const c=new Clock();c.begin();assert.equal(c.sample(0,'ns'),0);assert.equal(c.sample(2e7,'ns'),.02);});
test('long frame retains the existing one-second upper bound',()=>{const c=new Clock();c.begin();c.sample(1e9,'ns');assert.equal(c.sample(61e9,'ns'),1);assert.equal(c.sample(61e9+2e7,'ns'),.02);});
test('end/cancel/hide stop late callbacks and restart has no retained dt',()=>{const c=new Clock();for(let i=0;i<3;i++){c.begin();assert.equal(c.sample(1e9,'ns'),0);c.stop();assert.equal(c.sample(2e9,'ns'),0);c.begin();assert.equal(c.sample(100e9,'ns'),0);assert.equal(c.sample(100e9+1e7,'ns'),.01);}});
test('gate suspension does not accrue time outside canvas/panel',()=>{const c=new Clock();c.begin();c.sample(1e9,'ns');c.suspend();assert.equal(c.sample(99e9,'ns'),0);});
test('invalid and backward timestamps cannot emit negative or catch-up bursts',()=>{const c=new Clock();c.begin();c.sample(100,'ms');for(const x of [NaN,Infinity,-1]){assert.equal(c.sample(x,'ms'),0);assert.equal(c.sample(100,'ms'),0);}assert.equal(c.sample(90,'ms'),0);assert.equal(c.sample(90,'ms'),0);assert.equal(c.sample(100,'ms'),.01);});
const page=fs.readFileSync('entry/src/main/ets/pages/SandCanvas.ets','utf8');
test('page passes raw event timestamp once to ns adapter; all stop boundaries are wired',()=>{assert(page.includes("sample(timestampNs, 'ns')"));assert(!page.includes('timestampMs > 0 ? timestampMs : Date.now()'));for(const name of ['onPageHide','onActionEnd','onActionCancel']){const part=page.slice(page.indexOf(name),page.indexOf(name)+380);assert(part.includes('resetPanGestureBoundary()'),name);}assert(page.includes('this.gestureRateClock.begin()'));});
console.log(JSON.stringify({suite:'T36 gesture time/rate',results},null,2));process.exitCode=results.some(x=>x.status==='FAIL')?1:0;
