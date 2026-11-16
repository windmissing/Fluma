const {RasterGeometryController,RasterAllocation,rasterPredicate}=load('render/RasterGeometry');
const cases=[];
function check(name,fn){fn();cases.push(name);}
function page(){
 const w=windowFixture();Object.assign(w.props.windowRect,{left:47,top:29,width:1280,height:2832});PrepareMainWindow.set(w);
 nativeState={surfaceEpoch:3,contextEpoch:2,widthPx:1176,heightPx:1716,sizeSource:'EGL_QUERY_SURFACE'};
 const box=(width,height,x=51.20001220703125,y=990)=>({size:{width,height},windowOffset:{x,y},screenOffset:{x:x+47,y:y+29},scale:{x:1,y:1,z:1},translate:{x:0,y:0,z:0},rotate:{angle:0}});
 const outer=box(1177.5999755859375,1716),inner=box(1176,1716,52,990),touch=box(1176,1716,52,990),actual=box(1176,1716,52,990),action=box(4,4);
 const map={'t463-allocation':outer,'t463-raster':inner,'t463-touch':touch,'t463-xcomponent':actual};
 const observers=new Map(),p=new Page();let scale=3.5;
 const ui={vp2px:n=>scale*n,getFocusController:()=>({clearFocus(){}}),getComponentUtils:()=>({getRectangleById:id=>{if(id.startsWith('t449-action-'))return action;assert(id in map,id);return map[id];}}),postFrameCallback(){},getUIObserver:()=>({on:(n,f)=>observers.set(n,f),off:n=>observers.delete(n)})};
 p.getUIContext=()=>ui;p.wireInputs();p.fullyVisible=true;p.fixedKeyboardConfirmed=true;
 const c=p.singleSand,packet=c.preparation.packet;
 Object.assign(packet,{session:1,surface:3,context:2,width:1176,height:1716,geometry:0,run:'run',enabled:true,token:'token',frame:1});
 c.session=1;c.lastReceipt={presentationState:'CURRENT',requestedToken:'token',actualToken:'token',swapSucceeded:true};
 p.onPageShow();packet.geometry=p.layout.geometry;
 const x={p,c,w,packet,outer,inner,touch,actual,observers,setScale(v){scale=v;},
  layout(){observers.get('didLayout')?.();},frame(){p.prepareFrame?.onFrame(++frame);},draw(){observers.get('willDraw')?.();},
  flush(){this.frame();this.layout();this.draw();},sync(){p.refreshRaster();packet.geometry=p.layout.geometry;},
  ready(){p.prepare();this.flush();assert.equal(p.prepareGate.state,'PREPARING');this.flush();assert.equal(p.prepareGate.state,'READY',p.prepareGate.reason);}
 };return x;
}
function allocation(width){const a=new RasterAllocation();a.parent.width=width;a.parent.height=10;a.window.width=3000000000;a.window.height=2000;a.vp2px=3.5;a.windowId=1;a.windowGeneration=1;a.transformIdentity=true;return a;}
for(const [width,expected] of [[1-Number.EPSILON,0],[1,1],[1+Number.EPSILON,1],[1177.9999999999998,1177],[1178,1178],[1178.0000000000002,1178],[1177.5,1177],[0,0],[-1,0],[NaN,0],[Infinity,0],[2147483648,0]]){
 check('floor '+width,()=>{const c=new RasterGeometryController();c.allocate(allocation(width));assert.equal(c.plan.rasterW,expected);assert.equal(c.plan.valid,expected>0);});
}
check('actual two frames, exact integer sizes and both coordinate domains',()=>{
 const x=page();x.ready();const s=JSON.parse(x.p.prepareGate.report()).permit.snapshot;
 assert.equal(s.raster.allocation.parent.width,1177.5999755859375);assert.equal(s.raster.rasterW,1176);assert.equal(s.component.width,1176);assert.equal(s.viewportWidth,1176);
 assert.deepEqual([s.vpBoxWindowPx.x,s.vpBoxScreenPx.x],[52,99]);assert.equal(s.raster.native.geometry,s.geometry);
 assert.equal(x.c.capture.geometry().rasterWidth,1176);assert.equal(x.c.capture.geometry().raster.actual.x,52);
});
for(const delta of [-1,1])check('EGL '+delta,()=>{const x=page();nativeState.widthPx+=delta;x.sync();x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'RASTER_NATIVE_MISMATCH');});
check('old packet width',()=>{const x=page();x.packet.width=1178;x.sync();x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'RASTER_NATIVE_MISMATCH');});
for(const epoch of ['surfaceEpoch','contextEpoch'])check('stale packet after '+epoch,()=>{const x=page();nativeState[epoch]++;x.sync();x.p.prepare();x.flush();x.flush();assert.notEqual(x.p.prepareGate.state,'READY');assert.equal(x.p.readPrepareSnapshot().nativeReady,false);});
check('wrong actual component at 1178',()=>{const x=page();x.actual.size.width=1178;x.sync();x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'RASTER_COMPONENT_SIZE');});
check('touch layer must match actual node',()=>{const x=page();x.touch.windowOffset.x+=.25;x.sync();x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'RASTER_PLACEMENT_MISMATCH');});
check('transform unsupported',()=>{const x=page();x.inner.scale.x=1.01;x.sync();x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'RASTER_TRANSFORM_UNSUPPORTED');});
check('window origin mismatch',()=>{const x=page();x.actual.screenOffset.x+=.25;x.sync();x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'RASTER_PLACEMENT_MISMATCH');});
check('delayed layout rejects old frame, two distinct frames after layout',()=>{const x=page();x.p.prepare();x.frame();x.draw();assert.equal(x.p.prepareObservation.stages[4].count,0);x.layout();x.frame();x.draw();x.draw();assert.equal(x.p.prepareGate.state,'PREPARING');x.frame();x.draw();assert.equal(x.p.prepareGate.state,'READY');});
check('same floor movement invalidates permit and reserve has zero side effects',()=>{const x=page();x.ready();const before=x.p.rasterRevision,old=x.c.holdStatus(),seq=x.c.t394.sequence;x.outer.windowOffset.x-=.1;x.outer.screenOffset.x-=.1;
 const roi=new Int32Array([0,0,1,1,1,1,1,1,2,2,1,1]);assert.equal(x.c.reserveCapture(roi,'',1000).state,'REJECTED');assert.equal(x.c.t394.sequence,seq);assert.equal(x.c.holdStatus().requestId,old.requestId);assert(x.p.rasterRevision>before);assert.equal(x.p.rasterW,1176);});
check('same floor scale change invalidates an open hold before clock and keeps first reason',()=>{const x=page();x.ready();x.c.capture.reserve(new Int32Array(12),'k',1000,12,1);x.c.capture.open(12,1,x.packet.token,x.packet.frame,x.packet.run,x.c.ownerKey(),now);assert.equal(x.c.holdStatus().state,'HOLD_OPEN');x.setScale(3.51);x.p.clockCold();const old=x.c.holdStatus();assert.equal(old.state,'INVALID');assert.equal(x.p.probeStatus,'CLOCK_HOLD_NOT_OPEN');x.outer.size.width-=.1;x.sync();assert.equal(x.c.holdStatus().reason,old.reason);assert.equal(x.c.holdStatus().holdId,old.holdId);});
check('generated draw packet uses allocated integer size and rejects mismatching EGL',()=>{
 const x=page(),frameContext={boxX:0,boxY:0,boxW:x.p.surfaceWidth,boxH:x.p.surfaceHeight,geometryEpoch:x.p.layout.geometry};
 x.c.preparation.pack=()=> 'OK';x.c.prepareSingleSandOptics=()=>true;
 native.prepareDualFieldSurface=()=>({...nativeState,code:0});
 assert.equal(x.c.prepareSingleSandFrame({},frameContext,3.5),true);
 assert.deepEqual([x.packet.w,x.packet.h,x.packet.width,x.packet.height],[1176,1716,1176,1716]);
 nativeState.widthPx=1178;assert.equal(x.c.prepareSingleSandFrame({},frameContext,3.5),false);
 assert.equal(x.c.failure,'RASTER_NATIVE_SIZE_MISMATCH');
});
check('rotation and native generation need new layout',()=>{const x=page();x.ready();const rev=x.p.rasterRevision;x.w.props.windowRect.width=2100;nativeState.surfaceEpoch++;x.sync();assert(x.p.rasterRevision>rev);assert.equal(x.p.prepareGate.state,'INVALIDATED');assert.equal(x.p.raster.plan.layoutAcknowledged,0);});
check('old observer cannot consume new plan',()=>{const x=page();x.p.prepare();const old=x.observers.get('didLayout');x.p.onPageHide();x.p.onPageShow();x.sync();x.p.prepare();const current=x.p.prepareObservation;old();assert.equal(current.layoutGeneration,0);});
check('keyboard occlusion stays strict',()=>{const x=page();x.w.area=keyboard(400);x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'KEYBOARD_OCCLUDED');});
check('keyboard configuration unknown stays strict',()=>{const x=page();x.p.fixedKeyboardConfirmed=false;x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'KEYBOARD_VISIBILITY_UNKNOWN');});
check('exact inward target and position state',()=>{const x=page();assert.deepEqual([x.p.raster.plan.desiredWindowRect.x,x.p.raster.plan.desiredWindowRect.y,x.p.rasterW,x.p.rasterH],[52,990,1176,1716]);assert.equal(x.p.rasterLocalX,0.79998779296875);assert(source.includes(".position({x:this.rasterLocalX+'px',y:this.rasterLocalY+'px'})"));});
for(const [left,width] of [[51,1177],[52,1177]])check('original failed placement '+left,()=>{const x=page();x.actual.windowOffset.x=left;x.actual.screenOffset.x=left+47;x.actual.size.width=width;x.sync();x.p.prepare();x.flush();x.flush();assert.notEqual(x.p.prepareGate.state,'READY');});
check('all screen frames are checked',()=>{const x=page();x.touch.screenOffset.x++;x.sync();x.p.prepare();x.flush();x.flush();assert.equal(x.p.prepareGate.reason,'RASTER_PLACEMENT_MISMATCH');});
check('stale observation generation does not change plan',()=>{const x=page(),p=x.p.raster.plan,old=p.generation;x.outer.windowOffset.x+=.01;x.outer.screenOffset.x+=.01;x.sync();const before=JSON.stringify(p);assert.equal(x.p.raster.observe(p.actual,p.screen,p.inner,p.touch,p.native,true,p.innerScreen,p.touchScreen,old),false);assert.equal(JSON.stringify(p),before);});
for(const scale of [2.625,3.5])for(const origin of [9.999999999999998,10,10.000000000000002,10.5])check('adjacent boundary '+origin+' scale '+scale,()=>{const a=allocation(10.75);a.vp2px=scale;a.parent.x=a.screen.x=origin;a.parent.y=a.screen.y=origin;const c=new RasterGeometryController();c.allocate(a);assert.equal(c.plan.desiredWindowRect.x,Math.ceil(origin));assert.equal(c.plan.rasterW,Math.floor(origin+10.75)-Math.ceil(origin));assert.equal(c.plan.desiredWindowRect.y,Math.ceil(origin));assert(c.plan.desiredWindowRect.x>=origin);});
check('pixel lengths on real inner nodes',()=>{for(const id of ['t463-xcomponent','t463-raster','t463-touch'])assert(source.includes(".id('"+id+"').width(this.rasterW+'px').height(this.rasterH+'px')"));assert(source.includes(".id('t463-allocation').width('92%').layoutWeight(1)"));});
global.setTimeout=realTimeout;global.clearTimeout=realClear;
console.log(JSON.stringify({schema:'T516_GENERATED_INWARD_RASTER_V2',passed:cases.length,failed:0,cases,device:'NOT_RUN'}));
