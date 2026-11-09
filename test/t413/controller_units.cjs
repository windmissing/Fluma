const fs=require('node:fs'),Module=require('node:module');
let source=fs.readFileSync('test/t394/units.cjs','utf8').split("ctl.beginT394('LOCKED',true);")[0];
source=source.replace("let surface=2",'let testNow=1000;let surface=2');
source=source.replace("getUptime:()=>Number(process.hrtime.bigint())",'getUptime:()=>testNow*1e6');
source=source.replace("captureStatus:nativeOK?",'frameSerial:packet.frame,session:packet.session,surfaceEpoch:packet.surface,contextEpoch:packet.context,geometryEpoch:packet.geometry,requestedRoiWords:Array.from(roi),presentationState:receipt.presentationState,swapSucceeded:nativeOK,unbindSucceeded:nativeOK,captureStatus:nativeOK?');
source=source.replace("captureStage:nativeOK?'COMPLETE':'SWAP'", "captureStage:nativeOK?'COMPLETE':'SWAP'");
source+=`
const {CaptureGeometry}=load('render/T413Capture');
let passed=0;const cases=[];
function test(name,f){f();passed++;cases.push(name);}
const hash='a'.repeat(64),hash2='b'.repeat(64);
function geometry(x=0){const g=new CaptureGeometry();g.globalX=x;g.globalY=100;g.widthVp=400;g.heightVp=350;g.vp2px=3;g.geometry=1+x;g.visible=true;g.configuration='fixture';ctl.captureGeometry(g);}
function ready(){nativeOK=true;prepareOK=true;geometry();model.startT394Fixture(true,testNow);ctl.beginT394('STATIC_NEUTRAL',true);assert(draw().currentFrameSucceeded);}
const roi=parseWindowRoi('20,20,16,16;100,100,16,16;200,200,16,16');
function reserve(){const r=ctl.reserveCapture(roi,'',300000);assert.equal(r.state,'RESERVED',r.reason);return r;}
function opened(){reserve();draw();const s=ctl.holdStatus();assert.equal(s.state,'HOLD_OPEN',s.reason);return s;}
function archive(s){assert.notEqual(ctl.ackArchive(s.attemptId,s.requestId,hash).state,'REJECTED');}
function failureRelease(){const s=ctl.holdStatus();if(s.archiveState!=='NATIVE_ARCHIVE_SAVED')archive(s);const text=ctl.capture.terminal();assert(text);ctl.capture.savedTerminal(s.attemptId,s.holdId,hash2);assert.equal(ctl.releaseCapture(s.attemptId,s.holdId).state,'RELEASED');}
ready();
test('invalid ROI and budget reject without phase/hold/evidence mutation',()=>{
 const before=JSON.stringify([ctl.capture.status(),ctl.t394.sequence,ctl.takeWindowProbe()]);
 assert.equal(ctl.reserveCapture(new Int32Array([0,0,65,64]),'',300000).state,'REJECTED');
 assert.equal(ctl.reserveCapture(roi,'',300001).state,'REJECTED');
 assert.equal(JSON.stringify([ctl.capture.status(),ctl.t394.sequence,ctl.takeWindowProbe()]),before);
});
const s=opened();
test('HOLD_OPEN freezes actual controller draw and rejects reserve/phase/optics/plan',()=>{
 const before=JSON.stringify([ctl.holdStatus(),ctl.t394.sequence,ctl.takeWindowProbe()]);const n=normal+readbacks;
 assert.equal(draw(),null);assert.equal(normal+readbacks,n);
 assert.equal(ctl.reserveCapture(roi,'',300000).reason,'HOLD_ACTIVE');ctl.beginT394('BAD',false);ctl.setProbeNeutral(true);
 assert.equal(ctl.setT406Plan('bad'),'HOLD_ACTIVE');assert.equal(JSON.stringify([ctl.holdStatus(),ctl.t394.sequence,ctl.takeWindowProbe()]),before);
});
test('failed export leaves evidence; wrong async ack rejected; correct ack does not end OPEN',()=>{
 const e=ctl.takeWindowProbe();assert(e);assert.equal(ctl.ackArchive('old',s.requestId,hash).state,'REJECTED');
 assert.equal(ctl.takeWindowProbe(),e);assert.equal(ctl.ackArchive(s.attemptId,s.requestId,'').state,'REJECTED');archive(s);
 assert.equal(ctl.holdStatus().state,'HOLD_OPEN');assert.equal(ctl.holdStatus().archiveState,'NATIVE_ARCHIVE_SAVED');
});
test('app original-domain clock fresh samples carry exact hold/session/serial',()=>{
 const a=ctl.sampleHoldClock(s.holdId);testNow+=2;const b=ctl.sampleHoldClock(s.holdId);assert(b.sampleSerial>a.sampleSerial);
 assert.equal(b.appMonoMs,testNow);assert.equal(b.holdId,s.holdId);assert.equal(ctl.sampleHoldClock('old'),null);
});
test('seal fixes first endpoints before async write; retries immutable and no release before saved',()=>{
 assert.equal(ctl.sealCapture(s.holdId).state,'SEALED');const seal=ctl.capture.terminal();testNow+=100;
 assert.equal(ctl.sealCapture(s.holdId).state,'SEALED');assert.equal(ctl.capture.terminal(),seal);
 assert.equal(ctl.releaseCapture(s.attemptId,s.holdId).reason,'ARCHIVE_OR_TERMINAL_PENDING');
 ctl.capture.savedTerminal(s.attemptId,s.holdId,hash2);assert.equal(ctl.capture.terminal(),seal);
 assert.equal(ctl.releaseCapture(s.attemptId,s.holdId).state,'RELEASED');
});
ready();const next=opened();
test('late old ack cannot clear new evidence',()=>{const e=ctl.takeWindowProbe();assert.equal(ctl.ackArchive(s.attemptId,s.requestId,hash).state,'REJECTED');assert.equal(ctl.takeWindowProbe(),e);});
test('real elapsed expiry rejects seal without timer and stays frozen',()=>{
 testNow+=300000;assert.equal(ctl.sealCapture(next.holdId).state,'REJECTED');assert.equal(ctl.holdStatus().state,'EXPIRED');
 const text=ctl.capture.terminal();testNow+=500;assert.equal(ctl.capture.terminal(),text);assert.equal(draw(),null);failureRelease();
});
ready();const geo=opened();
test('position change invalidates first cause, keeps failed frame frozen until archived release',()=>{
 geometry(1);assert.equal(ctl.holdStatus().state,'INVALID');assert.equal(ctl.holdStatus().reason,'GEOMETRY_CHANGED');
 ctl.capture.invalidate('SECOND');assert.equal(ctl.holdStatus().reason,'GEOMETRY_CHANGED');assert.equal(draw(),null);failureRelease();
});
ready();const own=opened();
test('owner change invalidates and never seals success',()=>{surface++;assert.equal(ctl.sealCapture(own.holdId).state,'REJECTED');assert.equal(ctl.holdStatus().reason,'DISPLAY_HOLD_OWNER_CHANGED');failureRelease();});
ready();reserve();
test('cancel RESERVED archives failed request with empty holdId, stale cancel zero effect',()=>{
 const r=ctl.holdStatus();assert.equal(ctl.cancelCapture('old','BAD').state,'REJECTED');assert.equal(ctl.holdStatus().state,'RESERVED');
 ctl.cancelCapture(r.attemptId,'PAGE_HIDDEN');assert.equal(ctl.holdStatus().holdId,'');assert.equal(ctl.holdStatus().state,'INVALID');failureRelease();
});
ready();opened();
test('background/hide invalidates and keeps first failure',()=>{ctl.rejectT394('PAGE_HIDDEN');assert.equal(ctl.holdStatus().reason,'PAGE_HIDDEN');assert.equal(draw(),null);failureRelease();});
console.log(JSON.stringify({revision:'T414_CONTROLLER_UNITS_V2',passed,failed:0,device:'NOT_RUN',cases}));
`;
const m=new Module(__filename);m.paths=module.paths;m._compile(source,__filename);
