const fs=require('fs'),path=require('path'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{
 compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P013FrameAdapter,p013Reference}=require('../entry/src/main/ets/render/P013FrameAdapter.ets');
const {P013FrameInput,P013LightParams}=require('../entry/src/main/ets/render/P013RenderTypes.ets');
const Module=require('module');
const originalLoad=Module._load;
Module._load=function(name,parent,main){if(name==='@ohos.hilog')return {default:{info(){},error(){},warn(){},debug(){}}};return originalLoad.call(this,name,parent,main);};
const {P012WaterSandRenderer}=require('../entry/src/main/ets/render/P012WaterSandRenderer.ets');
const results=[];const expectedErrors=[];console.error=(...args)=>expectedErrors.push(args.join(' '));console.info=()=>{};
function test(name,fn){try{fn();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:e.stack});}}
function scene(){return {cols:128,capacity:192,sandTop:Float64Array.from({length:128},(_,i)=>80+Math.sin(i/10)*20),
 waterTop:new Float64Array(128).fill(160),bh:new Float64Array(128).fill(20),bl:new Float64Array(128).fill(20)};}
function frame(){let f=new P013FrameInput();Object.assign(f,{diagnosticEnabled:true,surfaceWidthPx:1024,surfaceHeightPx:768,
 boxXPx:50,boxYPx:60,boxWidthPx:800,boxHeightPx:600,capacity:192});return f;}
const a=new P013FrameAdapter();
test('profiles match actual Canvas DISPLAY_4X3_V1 for every channel/column',()=>{
 const s=scene(),before=JSON.stringify(s);assert.equal(a.pack(s),true);assert.equal(JSON.stringify(s),before);
 const r=new P012WaterSandRenderer(128,192);
 r.buildDisplayProfiles({bhAt:i=>s.bh[i],blAt:i=>s.bl[i],sandTopAt:i=>s.sandTop[i],waterTopAt:i=>s.waterTop[i]});
 for(let i=0;i<128;i++)for(const [k,key] of ['smoothSandTop','smoothWaterTop','smoothBh','smoothMixStart'].entries())
 assert.equal(a.data[4*i+k],Math.fround(r[key][i]));
});
test('invalid input leaves reusable 2048B packet unchanged',()=>{
 const before=a.data.slice();for(const mutate of [s=>s.cols=127,s=>s.capacity=NaN,s=>s.bh[1]=NaN,
 s=>s.bl[3]=-100,s=>s.sandTop[7]=170,s=>s.waterTop[9]=193]){
 const s=scene();mutate(s);assert.equal(a.pack(s),false);assert.deepEqual(a.data,before);}
});
test('Float32Array with nonzero byteOffset carries exactly 512 floats',()=>{
 const backing=new Float32Array(520);backing.fill(-99);backing.set(a.data,4);
 const view=new Float32Array(backing.buffer,16,512);assert.deepEqual(view,a.data);assert.equal(view.byteOffset,16);
});
test('flat fixture neutral reference and no sand outside mask',()=>{
 const s=scene();s.sandTop.fill(80);assert(a.pack(s));const f=frame(),l=new P013LightParams();
 const norm=Math.hypot(l.lightX,l.lightY,l.lightZ);l.lref=l.la+l.ld*l.lightZ/norm;
 assert.equal(p013Reference(a.data,f,l,450,500),1);assert.equal(p013Reference(a.data,f,l,0,500),null);
 assert.equal(p013Reference(a.data,f,l,450,100),null);f.diagnosticEnabled=false;assert.equal(p013Reference(a.data,f,l,450,500),null);
});
test('180 degree and non-fullscreen box maps the identical material point',()=>{
 a.pack(scene());const f=frame(),l=new P013LightParams();const v=p013Reference(a.data,f,l,280,500);
 f.visualInverted=true;assert.equal(p013Reference(a.data,f,l,50+800-(280-50),60+600-(500-60)),v);
});
test('depth approaches 1 across H/L/M without material-boundary jumps',()=>{
 a.pack(scene());const f=frame(),l=new P013LightParams();let previous=Infinity;
 for(let y=500;y<659;y+=1){const v=p013Reference(a.data,f,l,450,y);assert(v!==null);const delta=Math.abs(v-1);
 assert(delta<=previous+1e-14);previous=delta;}
});
test('mirrored light and mirrored two slopes exchange the sign about fixture reference',()=>{
 const s=scene();for(let i=0;i<128;i++)s.sandTop[i]=60+(i<64?i:127-i)*0.7;a.pack(s);
 const f=frame(),l=new P013LightParams();l.lightY=0;l.lightX=-0.3;l.lightZ=1;
 l.lref=l.la+l.ld*l.lightZ/Math.hypot(l.lightX,l.lightZ);
 const sample=(column)=>{const x=f.boxXPx+column*f.boxWidthPx/128;const top=a.data[4*column];
 return p013Reference(a.data,f,l,x,f.boxYPx+(1-(top-1)/f.capacity)*f.boxHeightPx)-1;};
 const left=sample(30),right=sample(97);assert(left*right<0);l.lightX*=-1;
 assert(Math.abs(sample(30)-right)<1e-7);assert(Math.abs(sample(97)-left)<1e-7);
});
test('empty sand and one-column edge ramp preserve continuous sampler mask',()=>{
 const s=scene();s.sandTop.fill(0);s.bh.fill(0);s.bl.fill(0);a.pack(s);
 assert.equal(p013Reference(a.data,frame(),new P013LightParams(),100,650),null);
 s.sandTop[0]=1;a.pack(s);assert(a.data[0]>0);assert(a.data[4]>0);assert.equal(a.data[4*5],0);
});


// Controller tests use a synchronous boundary double; the host suite compiles the actual native bridge.
const Module2=require('module');let time=0,drawCode=0,swapOk=true,uploads=0,releases=0,drains=0,inits=0,lastFrame,lastData,nativeBindingReads=0,waiting=false;
const native={getState(){return {code:0,sessionId:7,state:waiting?'WAIT_WINDOW':'WINDOW_AVAILABLE',surfaceEpoch:3,contextEpoch:1,widthPx:1024,heightPx:768,active:true,ownerBound:true};},
 init(id){assert.equal(id,'p013_lmat_surface');inits++;return {code:0,sessionId:7};},setActive(){return {code:0};},
 drawFrame(f,d,l){lastFrame={...f};lastData=d;uploads++;return {code:drawCode,sessionId:7,frameId:f.frameId,swapSucceeded:swapOk};},
 release(){releases++;return {code:0};},drainTelemetry(){drains++;return {records:[],droppedTelemetry:0,telemetryLiveCount:0};}};
const prior=Module2._load;Module2._load=function(name,parent,main){
 if(name==='libp013renderer.so')return Object.defineProperty({},'default',{get(){nativeBindingReads++;return native;}});
 if(name==='@ohos.systemDateTime')return {default:{TimeType:{ACTIVE:1},getUptime(){return ++time*10000;}}};
 return prior.call(this,name,parent,main);
};
const {P013RenderController}=require('../entry/src/main/ets/render/P013RenderController.ets');
const {P013Mode}=require('../entry/src/main/ets/render/P013RenderTypes.ets');
const ctx={fieldEpoch:2,geometryEpoch:3,continuityEpoch:4,configRevision:9,boxX:50,boxY:60,boxW:800,boxH:600,visualInverted:false};
function submit(c){c.submit(scene(),ctx,1024,768,123456789);}
test('binding is first accessed in XComponent onLoad, never during construction or pre-load controls',()=>{
 const before=nativeBindingReads,c=new P013RenderController();c.setActive(false);c.drainEvidence();c.drainTelemetry();c.release();
 assert.equal(nativeBindingReads,before);c.onLoad();assert.equal(nativeBindingReads,before+1);c.release();
});

test('onLoad may precede native window readiness; retry initializes only after window callback',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();
 const before=inits,count=uploads;waiting=true;submit(c);assert.equal(inits,before);assert.equal(uploads,count);
 waiting=false;submit(c);assert.equal(inits,before+1);assert.equal(uploads,count+1);c.release();
});

test('controller baseline never calls bridge; overlay uses one buffer and new IDs',()=>{
 const c=new P013RenderController(),before=uploads;submit(c);assert.equal(uploads,before);
 c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();submit(c);const buffer=lastData;
 assert.equal(lastFrame.frameId,1);assert.equal(lastFrame.fieldEpoch,2);assert.equal(lastFrame.sessionId,7);
 submit(c);assert.equal(lastFrame.frameId,2);assert.equal(lastData,buffer);assert.equal(c.diagnosticVisible,true);
});
test('ON->OFF and ON->baseline submit transparent clear before removing layer',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();submit(c);
 c.setMode(P013Mode.GLES_LMAT_DIAG_OFF);assert.equal(c.mode,P013Mode.GLES_LMAT_DIAG_ON);submit(c);
 assert.equal(lastFrame.diagnosticEnabled,false);assert.equal(c.mode,P013Mode.GLES_LMAT_DIAG_OFF);
 c.setMode(P013Mode.CANVAS_BASELINE);const before=releases;submit(c);
 assert.equal(lastFrame.diagnosticEnabled,false);assert.equal(c.mode,P013Mode.CANVAS_BASELINE);assert.equal(releases,before+1);
});
test('swap failure preserves fault telemetry and removes diagnostic layer',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();drawCode=303;swapOk=false;
 const before=drains;submit(c);assert.equal(c.mode,P013Mode.CANVAS_BASELINE);assert.equal(c.diagnosticVisible,false);
 assert.equal(drains,before+2);assert(c.failureTelemetry);assert.equal(c.fallbackReason,'DRAW_303');drawCode=0;swapOk=true;
});
test('stale geometry hides old heatmap, consumes frame ID and retries with a new one',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();submit(c);drawCode=103;
 submit(c);assert.equal(c.diagnosticVisible,false);const id=lastFrame.frameId;drawCode=0;submit(c);
 assert.equal(lastFrame.frameId,id+1);assert.equal(c.diagnosticVisible,true);
});
test('background gates calls; resume reuses owner; disposal is idempotent',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();submit(c);
 const before=uploads;c.setActive(false);submit(c);assert.equal(uploads,before);c.setActive(true);submit(c);
 assert.equal(uploads,before+1);const old=releases;c.release();c.release();assert.equal(releases,old+1);submit(c);assert.equal(uploads,before+1);
});
test('same native session remount keeps increasing IDs; canonical evidence resets mode',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();submit(c);const before=lastFrame.frameId;
 c.onLoad();submit(c);assert.equal(lastFrame.frameId,before+1);
 c.forceBaseline('CANONICAL_EVIDENCE');assert.equal(c.mode,P013Mode.CANVAS_BASELINE);assert.equal(c.diagnosticVisible,false);
});
test('controller ring is bounded, reports loss, and drains immutable copies',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();
 for(let i=0;i<2051;i++)c.submit(scene(),ctx,1024,768,1000000*(i+1));
 const batch=c.drainEvidence();assert.equal(batch.controller.length,2048);assert.equal(batch.droppedControllerTelemetry,3);
 const first=JSON.stringify(batch.controller[0]);submit(c);assert.equal(JSON.stringify(batch.controller[0]),first);
 assert.equal(c.drainEvidence().controller.length,1);
});
test('unrecoverable bridge/drain/release exceptions never escape into the Canvas ticker',()=>{
 const c=new P013RenderController();c.setMode(P013Mode.GLES_LMAT_DIAG_ON);c.onLoad();
 const methods=[native.getState,native.drainTelemetry,native.release];
 native.getState=()=>{throw Error('bridge allocation failure');};
 native.drainTelemetry=()=>{throw Error('cannot allocate drain result');};
 native.release=()=>{throw Error('cannot allocate release result');};
 try {assert.doesNotThrow(()=>submit(c));assert.equal(c.mode,P013Mode.CANVAS_BASELINE);assert.equal(c.diagnosticVisible,false);}
 finally {[native.getState,native.drainTelemetry,native.release]=methods;}
});
test('nearest-rank summaries keep null for unavailable metrics',()=>{
 const {p013Stats}=require('../entry/src/main/ets/render/P013RenderController.ets');
 assert.deepEqual(p013Stats([],2),{n:0,p50:null,p95:null,max:null,failCount:2});
 assert.deepEqual(p013Stats(Array.from({length:20},(_,i)=>20-i),1),{n:20,p50:10,p95:19,max:20,failCount:1});
});
test('native telemetry staging is bounded and joins native plus controller loss',()=>{
 const c=new P013RenderController();c.onLoad();const prior=native.drainTelemetry;
 native.drainTelemetry=()=>({records:Array.from({length:2051},(_,i)=>({sessionId:7,frameId:i+1})),
 droppedTelemetry:2,telemetryLiveCount:2051});
 try {const batch=c.drainEvidence();assert.equal(batch.native.records.length,2048);
 assert.equal(batch.native.records[0].frameId,4);assert.equal(batch.native.droppedTelemetry,5);}
 finally {native.drainTelemetry=prior;}
});
console.log(JSON.stringify({suite:'P013 P0 product adapter/CPU/controller',results},null,2));
process.exitCode=results.some(r=>r.status==='FAIL')?1:0;
