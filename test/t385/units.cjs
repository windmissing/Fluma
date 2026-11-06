'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),Module=require('node:module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const render=path.resolve(process.argv[2],'entry/src/main/ets/render');
const modules=new Map();
const mockNative={};
function load(name){
 if(modules.has(name))return modules.get(name).exports;
 const file=path.join(render,name+'.ets'),m=new Module(file);
 modules.set(name,m);
 m.require=(id)=>{
  if(id==='libp013renderer.so')return mockNative;
  if(id==='./P013SingleSandPreparation')return {P013SingleSandPreparation:class{
    packet=new (load('P013SingleSandTypes').SingleSandPacketV1)();
    profile=new Float32Array(256);slot={particleXY:new Float32Array(4),particleMeta:new Uint32Array(4)};
  }};
  if(id.includes('DiagnosticColdFiles'))return {DiagnosticColdFiles:class{async save(){throw Error('FSYNC_FAIL');}}};
  if(id.startsWith('./')&&!id.includes('/preparation/'))return load(id.slice(2));
  return {};
 };
 m._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020,esModuleInterop:true}}).outputText,file);
 return m.exports;
}
const {P013SingleSandController}=load('P013SingleSandController');
const {parseWindowRoi,saveWindowProbe}=load('SingleSandWindowCold');
const {chooseWindowFixture,windowFixtureKey}=load('RoiFixture');
const native={draws:[],evidence:null,takes:0,drawSingleSandScene(packet,_p,_x,_m,roi){
 this.draws.push(arguments.length);
 if(roi){this.evidence={manifest:{requestId:packet.probeRequestId,jsElements:packet.probeElements,
  jsByteLength:packet.probeBytes,session:packet.session,frameSerial:packet.frame,captureStatus:'CAPTURE_FAILED'}};}
 return {reason:'OK',actualToken:packet.token,currentFrameSucceeded:true};
},takeSingleSandWindowProbe(){this.takes++;const e=this.evidence;this.evidence=null;return e;}};
const ctl=new P013SingleSandController(native),{packet,profile,slot}=ctl.preparation;
Object.assign(packet,{run:'run',epoch:1,session:1,geometry:1,surface:1,context:1,token:'current',
 width:200,height:200,x:0,y:0,w:200,h:200,logicalW:200,logicalH:200,capacity:256,count:1,enabled:true});
profile.fill(64,0,128);slot.particleXY.set([64,150]);slot.particleMeta[0]=256;
let ready=false;
ctl.prepareSingleSandFrame=()=>ready;
const draw=()=>ctl.drawSingleSandScene({}, {}, 1);
(async()=>{
 const roi=parseWindowRoi('0,0,16,16');
 assert.equal(roi.byteLength,16);
 assert(ctl.armWindowProbe(roi,''));assert(!ctl.armWindowProbe(roi,''));
 assert.equal(draw(),null);assert.equal(ctl.takeWindowProbe(),null);
 ready=true;draw();draw();
 assert.deepEqual(native.draws,[5,4]);assert.equal(native.takes,1);
 const evidence=ctl.takeWindowProbe();assert.equal(evidence.manifest.jsByteLength,16);
 await assert.rejects(saveWindowProbe('unused',evidence),/FSYNC_FAIL/);
 assert.equal(ctl.takeWindowProbe(),evidence);assert(!ctl.armWindowProbe(roi,''));
 ctl.ackWindowProbe();assert(ctl.armWindowProbe(roi,''));
 packet.surface++;draw();assert.equal(ctl.takeWindowProbe().manifest.captureStage,'RESERVATION_IDENTITY_CHANGED');
 assert.equal(native.draws.at(-1),4);ctl.ackWindowProbe();
 assert(ctl.armWindowProbe(roi,''));ctl.cancelWindowProbe('PAGE_HIDDEN');
 assert.equal(ctl.takeWindowProbe().manifest.captureStage,'PAGE_HIDDEN');ctl.ackWindowProbe();
 const fixture=chooseWindowFixture(packet,profile,slot.particleXY,slot.particleMeta);
 const coords=parseWindowRoi(fixture);assert.equal(coords.length,12);
 assert(coords[1]+coords[3]<50&&coords[5]<117.1875&&coords[5]+coords[7]>117.1875&&coords[9]>50);
 const k=windowFixtureKey(packet);assert(ctl.armWindowProbe(coords,k));packet.displayHash='changed';draw();
 assert.equal(ctl.takeWindowProbe().manifest.captureStage,'RESERVATION_IDENTITY_CHANGED');
 profile.fill(0);assert.equal(chooseWindowFixture(packet,profile,slot.particleXY,slot.particleMeta),'');
 assert.equal(parseWindowRoi('0,0,0,16'),null);
 console.log('T385 generated controller: wait, reserve once, four-arg ordinary, export retry, epoch/fixture/page cancellation, BED/AIR/BG support PASS');
})().catch(e=>{console.error(e);process.exitCode=1;});
