const Module=require('module');const prior=Module._load;Module._load=function(n,p,m){if(n==='libp013renderer.so')return {default:{}};if(n==='@ohos.systemDateTime')return {default:{TimeType:{ACTIVE:1},getUptime:()=>1e9}};return prior.call(this,n,p,m);};
const fs=require('fs'),assert=require('assert/strict');const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');const {P013DualFieldAdapter:Adapter,P013DualFieldUploadSlot:Upload}=require('../entry/src/main/ets/render/P013DualFieldAdapter.ets');
const params={massPackage:.1,nmax:16,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:2,dragLight:2,liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0};
function field(){let f=new WaterSandField(128,512);f.setParticleRoute(params);f.setNaturalObservationRouteId('dual-field-particle-v1');f.initialize({waterRegions:[{startCol:0,endColInclusive:127,bottom:0,height:160}],heavyDepositedRegions:[{startCol:0,endColInclusive:127,bottom:0,height:24}],lightDepositedRegions:[],heavySuspendedRegions:[],lightSuspendedRegions:[]});return f;}
function inject(f,m=.3){let h=new Float64Array(128),l=new Float64Array(128);h[64]=m;return f.injectGestureSand({perColumnHeavy:h,perColumnLight:l,sourceId:1,sequence:1,inputTime:10,x:64.5,y:100,seed:1});}
function pack(f){let s=f.createPublishedRenderStateSlot();assert.equal(f.copyPublishedRenderStateInto(s),'OK');let u=new Upload(16),c={physicsCommitId:s.physicsCommitId,fieldEpoch:1,geometryEpoch:1,continuityEpoch:1,configRevision:1,visualInverted:false,particleLayerEnabled:true};return {s,u,c,a:new Adapter()};}

const {P013RenderController}=require('../entry/src/main/ets/render/P013RenderController.ets');
const {p013ProjectX,P013_COLUMN_PROJECTION:REV}=require('../entry/src/main/ets/render/P013ParticleProjection.ets');
const results=[];function test(name,fn){try{fn();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:e.stack});}}
test('same legal T61 boundary inputs: raw rejects, opt-in projects inside original column; source immutable',()=>{
 for(let col=0;col<128;col++){for(const x of [col, col+.5,col+1-1e-9]){const q=p013ProjectX(x);assert.equal(Math.floor(q),col);assert.ok(Math.abs(q-x)<=2**-17);}}
 for(const x of [127.99999999,127.999999999,127.9999999999,127.99999618530273]){
 const f=field();inject(f);const p=pack(f),pool=p.s.field.particleState;pool.pX[0]=x;
 const source=Buffer.from(pool.pX.buffer).toString('hex');assert.equal(p.a.packInto(p.s,p.c,p.u),'INVALID_PARTICLE');
 assert.equal(p.a.packInto(p.s,p.c,p.u,REV),'OK');assert.ok(p.u.particleXY[0]<128);assert.ok(p.u.projection.correctedCount>0);
 assert.equal(Buffer.from(pool.pX.buffer).toString('hex'),source);assert.ok(p.u.projection.maxXError<=2**-17);
 }});
test('all illegal authoritative predicates still reject atomically under opt-in',()=>{
 for(const mutate of [p=>p.pX[0]=128,p=>p.pX[0]=-1,p=>p.pX[0]=NaN,p=>p.pY[0]=513,p=>p.pY[0]=-1,p=>p.pMass[0]=0,p=>p.pState[0]=3,p=>p.pPhase[0]=2,p=>p.pId[0]=0,p=>p.pId[1]=p.pId[0]]){
 const f=field();inject(f);const p=pack(f);assert.equal(p.a.packInto(p.s,p.c,p.u,REV),'OK');
 const before=Buffer.from(p.u.particleXY.buffer).toString('hex');mutate(p.s.field.particleState);
 assert.notEqual(p.a.packInto(p.s,p.c,p.u,REV),'OK');assert.equal(Buffer.from(p.u.particleXY.buffer).toString('hex'),before);
 }});
function native(){let epoch=1,ctx=0,w=0,h=0;const calls=[];return {calls,getState:()=>({code:0,state:'WINDOW_AVAILABLE',surfaceEpoch:epoch,contextEpoch:ctx,widthPx:w,heightPx:h}),init:()=>({code:0,sessionId:1}),
 prepareDualFieldSurface:(id,expected)=>{calls.push('prepare');assert.equal(expected,epoch);epoch=2;ctx=1;w=800;h=480;return {code:0,sessionId:1,surfaceEpoch:epoch,contextEpoch:ctx,widthPx:w,heightPx:h,sizeSource:'EGL_QUERY_SURFACE',geometrySpace:'SURFACE_PX_V1',errorStage:'NONE'};},
 drawDualFieldFrame:(frame,data,xy,meta,visual)=>{calls.push('draw');assert.equal(frame.surfaceEpoch,2);assert.equal(frame.contextEpoch,1);assert.equal(frame.boxWidthPx,800);assert.equal(frame.boxHeightPx,480);assert.equal(visual.radiusPx,3);return {code:0,swapSucceeded:true,errorStage:'NONE'};}};}
function controller(n){const c=new P013RenderController();c.native=n;c.loaded=true;return c;}
test('T61 fractional zero-surface startup old path remains RED; explicit prepare path GREEN same publication',()=>{
 const p=pack(field());Object.assign(p.c,{boxX:0,boxY:0,boxW:1177.6,boxH:2442.8});
 const n=native(),c=controller(n);assert.equal(c.submitDualField(p.s,p.c,1177.6,2442.8,1),null);assert.equal(c.dualFailure,'NONUNIFORM_SCALE');assert.deepEqual(n.calls,[]);
 assert.equal(c.submitDualFieldSurfacePx(p.s,p.c,2,3).code,0);assert.deepEqual(n.calls,['prepare','draw']);assert.equal(c.lastPresentedCommit,p.s.physicsCommitId);
});
test('prepare failure and lifecycle epoch race never draw or claim a new commit',()=>{
 for(const race of [false,true]){const p=pack(field()),n=native(),c=controller(n);n.prepareDualFieldSurface=()=>({code:race?0:1,surfaceEpoch:9,contextEpoch:9,widthPx:800,heightPx:480,sizeSource:'EGL_QUERY_SURFACE',geometrySpace:'SURFACE_PX_V1'});
 assert.equal(c.submitDualFieldSurfacePx(p.s,p.c,1,3),null);assert.equal(c.lastPresentedCommit,0);assert.ok(!n.calls.includes('draw'));}
});
test('true nonuniform legacy transform still refuses native draw',()=>{const p=pack(field()),n=native(),c=controller(n);n.getState=()=>({state:'WINDOW_AVAILABLE',surfaceEpoch:1,contextEpoch:1,widthPx:800,heightPx:480});assert.equal(c.submitDualField(p.s,p.c,800,300,1),null);assert.equal(c.dualFailure,'NONUNIFORM_SCALE');assert.deepEqual(n.calls,[]);});
test('DEMO opts in; measurement and local touch share XComponent; density radius explicit',()=>{const s=fs.readFileSync('entry/src/main/ets/pages/EmptySandDemo.ets','utf8');assert.ok(s.includes('submitDualFieldSurfacePx'));assert.ok(s.includes('vp2px(1)'));const x=s.slice(s.indexOf('XComponent({'));assert.ok(x.indexOf('.onAreaChange')<x.indexOf("}.width('92%')"));assert.ok(x.indexOf('.onTouch')<x.indexOf("}.width('92%')"));assert.ok(!x.includes('.border('));});
console.log(JSON.stringify({suite:'T62 offline repair (native boundary synthetic)',results},null,2));process.exitCode=results.some(r=>r.status==='FAIL')?1:0;
