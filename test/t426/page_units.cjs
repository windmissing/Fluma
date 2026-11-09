const fs=require('node:fs'),path=require('node:path'),Module=require('node:module'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const root=path.resolve(process.argv[2],'entry/src/main/ets'),source=fs.readFileSync(path.join(root,'pages/P051DerivedV2Demo.ets'),'utf8');
const inputLines=source.split('\n').filter(s=>s.includes('TextInput({controller:')).map(s=>s.slice(s.indexOf('TextInput(')));
assert.equal(inputLines.length,3);assert(!source.includes('hideTextInput'));assert(source.includes('prepareGate.consume()'));
let now=1000,frame=1,ctx=null,currentWindow=null,throwStop=false,throwSubscribe=false,timers=[],files=new Map(),lateWrites=[];let deferWrites=false;
const native={getState:()=>({surfaceEpoch:3,contextEpoch:2,widthPx:800,heightPx:600})};
class ColdFiles {async save(name,text){if(deferWrites)await new Promise((resolve,reject)=>lateWrites.push({resolve,reject}));files.set(name,text);return {name,bytes:text.length,sha256:'a'.repeat(64)};}}
class TextController {stopEditing(){if(throwStop)throw {code:55,name:'ComponentError',message:'stop failed',runStatus:9};this.events?.onEditChange?.(false);this.events?.onBlur?.();}}
global.TextInputController=TextController;global.getContext=()=>({filesDir:'fixture'});
global.TextInput=options=>{
 const events={};options.controller.events=events;
 let proxy;proxy=new Proxy({}, {get:(_,key)=>(...args)=>{if(key==='defaultFocus')assert.equal(args[0],false);if(key==='onAppear')args[0]();if(/^on/.test(key))events[key]=args[0];return proxy;}});return proxy;
};
const cache=new Map();
function load(name){const file=path.resolve(root,name+'.ets');if(cache.has(file))return cache.get(file).exports;const m=new Module(file);cache.set(file,m);m.filename=file;
 m.require=id=>{
  if(id==='@ohos.window')return {AvoidAreaType:{TYPE_KEYBOARD:3},WindowStatusType:{FULL_SCREEN:1},WindowEventType:{WINDOW_SHOWN:1,WINDOW_ACTIVE:2,WINDOW_INACTIVE:3,WINDOW_HIDDEN:4,WINDOW_DESTROYED:7}};
  if(id==='@ohos.deviceInfo')return {deviceType:'phone'};
  if(id==='@ohos.arkui.UIContext')return {FrameCallback:class {}};
  if(id==='@ohos.sensor')return {SensorId:{ACCELEROMETER:1},on(){},off(){}};
  if(id==='@ohos.hilog')return {info(){},warn(){},error(){}};
  if(id==='@ohos.systemDateTime')return {getUptime:()=>now*1e6,TimeType:{ACTIVE:0}};
  if(id==='libp013renderer.so')return native;
  if(id==='@ohos.file.fs'||id==='@ohos.util'||id==='@ohos.security.cryptoFramework'||id==='@kit.AbilityKit')return {};
  if(id.endsWith('/DiagnosticColdFiles'))return {DiagnosticColdFiles:ColdFiles};
  if(id.endsWith('/P051DerivedDemoV2'))return {P051DerivedDemoV2:class {budget={enabled:false};createPublicationCold(){return {};}stop(){}setActive(){}}};
  if(id.endsWith('/P013SceneController'))return {P013SceneController:class {setActive(){}release(){}}};
  if(id.startsWith('.'))return load(path.relative(root,path.resolve(path.dirname(file),id)));
  return require(id);
 };
 let text=fs.readFileSync(file,'utf8');
 if(name==='pages/P051DerivedV2Demo'){
  text=text.slice(0,text.indexOf(' build(){'))+' wireInputs():void {\n'+inputLines.join('\n')+'\n}\n}\nexport { P051DerivedV2Demo };';
  text=text.replace(/@Entry\s*/g,'').replace(/@Component\s*/g,'').replace(/@State\s*/g,'').replace('struct P051DerivedV2Demo','class P051DerivedV2Demo');
 }
 m._compile(ts.transpileModule(text,{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020,esModuleInterop:true}}).outputText,file);return m.exports;
}
const realTimeout=global.setTimeout,realClear=global.clearTimeout;
global.setTimeout=(fn,ms)=>{timers.push({fn,ms});return timers.length;};global.clearTimeout=id=>{if(timers[id-1])timers[id-1].cancel=true;};
const {P051DerivedV2Demo:Page}=load('pages/P051DerivedV2Demo'),{PrepareMainWindow}=load('render/PrepareMainWindow');
const keyboard=height=>({topRect:{left:0,top:0,width:0,height:0},rightRect:{left:0,top:0,width:0,height:0},
 bottomRect:{left:0,top:1600-height,width:height?1200:0,height},leftRect:{left:0,top:0,width:0,height:0}});
function windowFixture(){const listeners=new Map();return {area:keyboard(0),focused:true,mode:1,
 props:{id:4,windowRect:{left:0,top:0,width:1200,height:1600}},
 getWindowProperties(){return this.props;},getWindowAvoidArea(){return this.area;},isFocused(){return this.focused;},getWindowStatus(){return this.mode;},
 on(key,fn){if(throwSubscribe&&key==='windowEvent')throw {code:8,message:'subscribe failed'};listeners.set(key,fn);},
 off(key,fn){assert.equal(listeners.get(key),fn);listeners.delete(key);},listeners,
 emit(key,value){listeners.get(key)?.(value);}};}
function page(){now+=10;throwStop=throwSubscribe=false;const w=windowFixture();currentWindow=w;PrepareMainWindow.set(w);
 const queued=[];let box={size:{width:800,height:600},windowOffset:{x:200,y:400},screenOffset:{x:200,y:400},scale:{x:1,y:1,z:1},translate:{x:0,y:0,z:0},rotate:{angle:0}};
 const p=new Page();p.getUIContext=()=>({vp2px:n=>2*n,getFocusController:()=>({clearFocus(){}}),getComponentUtils:()=>({getRectangleById:()=>box}),postFrameCallback:cb=>queued.push(cb)});
 p.wireInputs();p.layout.globalX=100;p.layout.globalY=200;p.layout.widthVp=400;p.layout.heightVp=300;p.layout.vp2px=2;p.layout.geometry=3;p.layout.visible=true;p.layout.configuration='FIXED_PANEL_XCOMPONENT_V2';
 p.fullyVisible=true;p.surfaceWidth=400;p.surfaceHeight=300;p.fixedKeyboardConfirmed=true;
 const c=p.singleSand,packet=c.preparation.packet;Object.assign(packet,{session:1,surface:3,context:2,width:800,height:600,geometry:3,run:'r',enabled:true,token:'token',frame:1});c.session=1;c.lastReceipt={presentationState:'CURRENT'};c.captureGeometry(p.layout);
 p.onPageShow();
 return {p,w,box,queued,flush(){const cb=queued.find(q=>typeof q.onIdle==='function');assert(cb);queued.splice(queued.indexOf(cb),1);cb.onFrame(++frame);cb.onIdle(2000000);},ready(){p.prepare();for(let i=0;i<4;i++)this.flush();assert.equal(p.prepareGate.state,'READY',p.prepareGate.reason);}};
}
let passed=0;const cases=[];let diagnosticSnapshot='';
async function test(name,f){await f();passed++;cases.push(name);}
(async()=>{
await test('cold no keyboard uses actual three input bindings and two different onIdle frames',()=>{const x=page();x.ready();const r=JSON.parse(x.p.prepareGate.report());assert(r.permit.secondFrame>r.permit.firstFrame);assert.equal(r.api.length,4);diagnosticSnapshot=x.p.prepareGate.report();});
await test('editing -> nonzero avoid -> exit editing -> zero avoid -> READY',()=>{const x=page();x.p.roiInput.events.onFocus();x.p.roiInput.events.onEditChange(true);x.w.area=keyboard(400);x.w.emit('avoidAreaChange',{type:3});x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'KEYBOARD_OCCLUDED');x.w.area=keyboard(0);x.w.emit('avoidAreaChange',{type:3});for(let i=0;i<3;i++)x.flush();assert.equal(x.p.prepareGate.state,'READY');});
await test('successful component command does not hide persistent keyboard; ratio1 cannot override',()=>{const x=page();x.w.area=keyboard(400);x.p.prepare();for(let i=0;i<4;i++)x.flush();assert.equal(x.p.prepareGate.state,'PREPARING');assert.equal(x.p.prepareGate.reason,'KEYBOARD_OCCLUDED');});
await test('previously editing without nonzero avoid is unknown',()=>{const x=page();x.p.planInput.events.onFocus();x.ready=()=>{};x.p.prepare();for(let i=0;i<4;i++)x.flush();assert.equal(x.p.prepareGate.reason,'KEYBOARD_VISIBILITY_UNKNOWN');});
await test('unconfirmed floating keyboard or non-fullscreen never READY',()=>{for(const change of [x=>x.p.fixedKeyboardConfirmed=false,x=>x.w.mode=2]){const x=page();change(x);x.p.prepare();for(let i=0;i<4;i++)x.flush();assert.equal(x.p.prepareGate.reason,'KEYBOARD_VISIBILITY_UNKNOWN');}});
await test('same-size position change resets frames, READY cannot survive immediate stale reserve',()=>{const x=page();x.ready();const before=x.p.singleSand.t394.sequence;x.box.windowOffset.x++;x.p.singleSand.reserveCapture(new Int32Array(12),'',1000);assert.equal(x.p.singleSand.t394.sequence,before);assert.equal(x.p.prepareGate.state,'INVALIDATED');});
await test('native old geometry and late action layout cannot produce permit',()=>{const x=page();x.p.singleSand.preparation.packet.geometry=2;x.p.prepare();for(let i=0;i<4;i++)x.flush();assert.equal(x.p.prepareGate.reason,'PREPARE_NATIVE_NOT_READY');x.p.singleSand.preparation.packet.geometry=3;x.flush();assert.equal(x.p.prepareGate.state,'PREPARING');x.flush();assert.equal(x.p.prepareGate.state,'READY');});
await test('actual reserveCapture has mandatory permit gate before phase/request mutation',async()=>{const x=page();const roi=new Int32Array([0,0,16,16,20,20,16,16,40,40,16,16]);const c=x.p.singleSand;assert.equal(c.reserveCapture(roi,'',1000).state,'REJECTED');assert.equal(c.t394.sequence,0);x.ready();x.p.probeRoi='0,0,16,16;20,20,16,16;40,40,16,16';await x.p.reserve();assert.equal(c.capture.status().state,'RESERVED');assert.equal(x.p.prepareGate.state,'INVALIDATED');assert.equal(x.p.prepareGate.reason,'PREPARE_PERMIT_CONSUMED');});
await test('READY edit invalidates permit; OPEN keyboard invalidates hold first cause',()=>{const x=page();x.ready();x.p.roiInput.events.onFocus();assert.equal(x.p.prepareGate.state,'INVALIDATED');const s=x.p.singleSand.capture;const roi=new Int32Array(12);s.reserve(roi,'',1000,1,1);s.open(1,1,'token',1,'r','owner',now);x.w.area=keyboard(400);x.w.emit('avoidAreaChange',{type:3});assert.equal(s.status().state,'INVALID');const why=s.status().reason;x.w.emit('keyboardHeightChange',400);assert.equal(s.status().reason,why);});
await test('double prepare no side effects; delayed timer/frame at >=3000 cannot READY',()=>{const x=page();x.p.prepare();const id=x.p.prepareGate.prepareId;x.p.prepare();assert.equal(x.p.prepareGate.prepareId,id);now+=3001;for(let i=0;i<3;i++)x.flush();assert.equal(x.p.prepareGate.state,'FAILED');assert(x.p.prepareGate.reason.startsWith('PREPARE_TIMEOUT'));});
await test('hide/show detaches exact listeners and stale callbacks cannot finish new prepare',()=>{const x=page();x.p.prepare();const old=x.p.prepareFrame,oldTimer=timers.at(-1).fn,oldListener=x.w.listeners.get('avoidAreaChange');x.p.onPageHide();assert.equal(x.w.listeners.size,0);x.p.onPageShow();x.p.prepare();const id=x.p.prepareGate.prepareId;old.onFrame(500);old.onIdle(2e6);oldTimer();oldListener({type:3});assert.equal(x.p.prepareGate.prepareId,id);assert.equal(x.p.prepareGate.state,'PREPARING');for(let i=0;i<4;i++)x.flush();assert.equal(x.p.prepareGate.state,'READY');});
await test('component error preserves raw55 but fails; window subscription error rejects',()=>{const x=page();throwStop=true;x.p.prepare();assert.equal(x.p.prepareGate.state,'FAILED');assert(JSON.parse(x.p.prepareGate.report()).api.some(r=>r.code===55&&r.runStatus==='9'));throwStop=false;const y=page();y.p.onPageHide();throwSubscribe=true;y.p.onPageShow();assert.equal(y.p.prepareGate.state,'FAILED');throwSubscribe=false;});
await test('late cold-write failure from hidden page cannot fail the next preparation or touch export busy',async()=>{
 const x=page();deferWrites=true;x.p.prepare();const old=lateWrites.splice(0);x.p.onPageHide();deferWrites=false;x.p.onPageShow();x.p.prepare();
 const id=x.p.prepareGate.prepareId;x.p.exporting=true;for(const p of old)p.reject(new Error('old disk error'));
 await Promise.resolve();await Promise.resolve();assert.equal(x.p.prepareGate.prepareId,id);assert.equal(x.p.exporting,true);assert.equal(x.p.prepareGate.state,'PREPARING');
});
await test('vp2px drift resets stability and a viewport outside the window rejects',()=>{
 const x=page();x.p.prepare();x.flush();x.flush();x.p.layout.globalX=110;x.box.windowOffset.x=220;x.box.screenOffset.x=220;
 x.flush();assert.equal(x.p.prepareGate.state,'PREPARING');x.flush();assert.equal(x.p.prepareGate.state,'READY');
 x.box.screenOffset.y=1500;x.p.observePrepare();assert.equal(x.p.prepareGate.state,'INVALIDATED');
});
await test('historical hide reject55 fixture cannot replace actual hidden/visible/unknown observations',async()=>{
 const raw=[];
 for(const mode of ['hidden','visible','unknown']){
  const x=page();
  try{await Promise.reject({code:55,message:'historical not bound',runStatus:9});}catch(error){raw.push(error);}
  if(mode==='visible')x.w.area=keyboard(400);
  if(mode==='unknown')x.p.fixedKeyboardConfirmed=false;
  x.p.prepare();for(let i=0;i<4;i++)x.flush();
  if(mode==='hidden')assert.equal(x.p.prepareGate.state,'READY');
  else assert.notEqual(x.p.prepareGate.state,'READY');
 }
 assert.equal(raw.length,3);
});
await test('window reconstruction terminates prepare; old generation cannot publish READY',()=>{const x=page();x.p.prepare();PrepareMainWindow.set(windowFixture());for(let i=0;i<3;i++)x.flush();assert.notEqual(x.p.prepareGate.state,'READY');});
global.setTimeout=realTimeout;global.clearTimeout=realClear;
fs.mkdirSync('test/reports/t428',{recursive:true});fs.writeFileSync('test/reports/t428/prepare-synthetic.json',JSON.stringify({evidenceSource:'SYNTHETIC_PAGE_HARNESS',device:'NOT_RUN',report:JSON.parse(diagnosticSnapshot)}));
console.log(JSON.stringify({revision:'T428_ACTUAL_PAGE_WIRING_V1',passed,failed:0,device:'NOT_RUN',cases}));
})().catch(error=>{global.setTimeout=realTimeout;global.clearTimeout=realClear;console.error(error);process.exitCode=1;});
