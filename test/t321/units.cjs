'use strict';
const fs=require('fs'),path=require('path'),cp=require('child_process'),Module=require('module'),assert=require('assert/strict'),crypto=require('crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const base=path.resolve('entry/src/main/ets/physics/clearwater'),load=n=>require(base+'/P011ClearWater'+n+'.ets');
const {P011ClearWaterEngine:E}=load('Engine'),{clearWaterCConfig:cfg,clearWaterCBConfig:bc}=load('CConfig'),{ClearWaterSourceController:Source}=load('SourceController'),{ClearWaterBAttachResult:Attach}=load('BTypes');
const original=cp.execFileSync('git',['show','eee20b3:entry/src/main/ets/physics/clearwater/P011ClearWaterSourceController.ets'],{encoding:'utf8'});
const baselineFile=path.join(base,'SourceBaseline.ets'),baseline=new Module(baselineFile);baseline.filename=baselineFile;baseline.paths=module.paths;baseline._compile(ts.transpileModule(String(original),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,baselineFile);const OldSource=baseline.exports.ClearWaterSourceController;
const cases=[];function test(name,fn){try{fn();cases.push({name,status:'PASS'})}catch(e){cases.push({name,status:'FAIL',error:String(e.stack)})}}
function engine(initial=null){const e=new E(cfg(),initial);assert.equal(e.attachBInto(bc(),new Attach()),0);return e;}
function make(Type=Source,initial=null){const e=engine(initial),s=new Type(e,1,1,1);return {e,s};}
function cold(s){return JSON.parse(s.trace.exportCold());}
function authority(e){const values={};for(const [k,v] of Object.entries(e.canonical)){if(k==='bHeader')continue;if(ArrayBuffer.isView(v))values[k]=Buffer.from(v.buffer,v.byteOffset,v.byteLength).toString('hex');else if(typeof v==='number'||typeof v==='boolean'||v===null)values[k]=v;}
 for(const [k,v] of Object.entries(e.canonical.pool))if(ArrayBuffer.isView(v))values['pool.'+k]=Buffer.from(v.buffer,v.byteOffset,v.byteLength).toString('hex');else if(typeof v==='number')values['pool.'+k]=v;
 values.input=e.currentInputGeneration;values.pending=e.pendingSourceCount;values.geometry=e.geometryEpoch;
 return crypto.createHash('sha256').update(JSON.stringify(values)).digest('hex');}
function tick(s,n,unit='NS',domain='ARKUI_FRAME_MONOTONIC'){const valid=s.tick(n,unit,domain);if(valid)assert.equal(s.sample(),0);return valid;}
test('T315 original NS windows produce exact rate1 quantity and GAP terminal',()=>{
 const {e,s}=make();s.select(1);assert(s.down(7,64.5,100));
 const a=2734444379322947,b=2734444390443514,c=2734444412682284;
 assert.equal(tick(s,a),false);assert(tick(s,b));assert(tick(s,c));assert.equal(e.canonical.sourceCommit,2);assert(Object.is(e.canonical.I_water,(b-a)/1e9+(c-b)/1e9));
 assert.equal(s.gestureValidSeconds,.033359337);assert.equal(s.gestureSampledSeconds,.033359337);assert.equal(s.configuredWaterRate,1);
 assert.equal(tick(s,c+100000000),false);assert.equal(s.stopReason,'GAP');assert.equal(s.sourceState,2);assert.match(s.feedbackMessage,/重新按下/);
 const data=cold(s);const row=data.data.find((r,i)=>r[1]===3&&data.reasons[i]==='GAP');assert(row);assert.equal(row[2],c);assert.equal(row[3],c+100000000);assert.equal(row[4],100000000);assert.equal(row[9],7);assert.equal(row[10],-1);assert.equal(row[16],2);assert.equal(row[17],2);assert.equal(row[18],2);assert.equal(row[27],1);assert.equal(row[13],row[12]+1);
});
test('no catch-up or auto-resume after GAP; new DOWN requires first zero tick',()=>{
 const {e,s}=make();s.select(1);s.down(1,64,100);tick(s,0);tick(s,16000000);tick(s,1000000000);const before=authority(e),commit=e.currentSourceCommit;
 s.move(1,65,100);s.up(1);for(let i=0;i<30;i++)tick(s,1016000000+i*16000000);assert.equal(e.currentSourceCommit,commit);assert.equal(authority(e),before);assert.equal(s.stopReason,'GAP');
 s.stop('GRAVITY_STALE');assert.equal(s.stopReason,'GAP');assert(s.down(1,65,100));assert.equal(s.stopReason,'');assert.equal(tick(s,2000000000),false);assert.equal(e.currentSourceCommit,commit);tick(s,2016000000);assert.equal(e.currentSourceCommit,commit+1);assert.equal(s.gestureValidSeconds,.016);
});
test('continuous pointer identity, MOVE no quantity, wrong UP ignored, correct UP explicit',()=>{
 const {e,s}=make();s.down(11,64,100);tick(s,1000000);assert.equal(s.move(99,1,1),false);s.up(99);assert.equal(s.sourceState,1);assert(s.move(11,65,100));assert.equal(e.currentSourceCommit,0);tick(s,17000000);const n=e.currentSourceCommit;s.up(11);assert.equal(s.stopReason,'UP');assert.equal(s.sourceState,2);tick(s,33000000);tick(s,49000000);assert.equal(e.currentSourceCommit,n);
});
for(const [name,raw,unit,domain,reason] of [['domain',32000000,'NS','OTHER','CLOCK_DOMAIN_CHANGE'],['unit',32,'MS','ARKUI_FRAME_MONOTONIC','CLOCK_DOMAIN_CHANGE'],['backward',15000000,'NS','ARKUI_FRAME_MONOTONIC','BACKWARD_CLOCK'],['nonfinite',NaN,'NS','ARKUI_FRAME_MONOTONIC','INVALID_CLOCK']])test(name+' closes pointer, preserves raw/domain and needs DOWN',()=>{
 const {e,s}=make();s.down(1,64,100);tick(s,0);tick(s,16000000);assert.equal(s.tick(raw,unit,domain),false);assert.equal(s.stopReason,reason);assert.equal(s.sourceState,2);const data=cold(s);if(name==='nonfinite'){const i=data.reasons.findIndex((v,i)=>v==='INVALID_CLOCK'&&data.data[i][1]===4);assert(i>=0);assert.equal(data.data[i][3],null);assert.equal(data.data[i][29],0);}else assert(data.domains.includes(domain));assert.equal(e.currentSourceCommit,1);
});
test('raw threshold remains 1/30; >1/30 GAP and no clamp',()=>{
 const {s}=make();s.down(1,64,100);assert.equal(tick(s,0),false);assert(tick(s,33333333));assert.equal(s.clock.seconds,.033333333);assert.equal(tick(s,66666667),false);const data=cold(s);assert(data.data.some((v,i)=>v[1]===4&&data.reasons[i]==='GAP'&&v[4]===33333334));
});
test('terminal records actual admitted/rejected and success counts, not wall duration',()=>{
 const {e,s}=make(Source,{W:new Float64Array(128).fill(256),bh:new Float64Array(128),mh:new Float64Array(128)});s.select(1);s.down(1,64,100);s.tick(0,'NS','TEST');s.tick(16000000,'NS','TEST');s.sample();const data=cold(s),row=data.data.find(r=>r[1]===6);assert(row);assert.equal(row[20],.016);assert.equal(row[21],0);assert(row[22]>0);assert.equal(row[19],0);assert.equal(row[17],1);assert.equal(e.canonical.I_water,0);
});
test('duplicate sample no request, STOP/CANCEL and rate config remain exact',()=>{
 const {e,s}=make();s.select(1);s.down(1,64,100);tick(s,0);tick(s,16000000);s.sample();s.sample();assert.equal(s.sampleCount,1);assert.equal(s.terminalCount,1);assert.equal(e.currentSourceCommit,1);s.stop('CANCEL');assert.equal(s.stopReason,'CANCEL');assert.equal(s.sourceState,2);assert.match(s.feedbackMessage,/取消/);assert.equal(cfg().rateWater,1);
});
test('observation ON/OFF and original controller bitwise authority parity after every operation',()=>{
 const original=make(OldSource),on=make(),off=make();off.s.trace.enabled=false;
 const ops=[s=>s.select(1),s=>s.down(2,64.5,100),s=>s.tick(0,'NS','TEST')];for(let i=1;i<=60;i++){ops.push(s=>s.tick(i*16000000,'NS','TEST'),s=>s.sample());}ops.push(s=>s.move(99,2,2),s=>s.move(2,65.5,100),s=>s.tick(2000000000,'NS','TEST'),s=>s.sample(),s=>s.tick(2016000000,'NS','TEST'),s=>s.sample(),s=>s.up(2),s=>s.select(0),s=>s.down(3,66,100),s=>s.tick(3000000000,'NS','TEST'),s=>s.tick(3016000000,'NS','TEST'),s=>s.sample(),s=>s.up(3));
 for(const op of ops){const values=[original,on,off].map(x=>op(x.s));assert.deepEqual(values,[values[0],values[0],values[0]]);assert.equal(authority(on.e),authority(original.e));assert.equal(authority(off.e),authority(original.e));assert.equal(on.s.feedbackCount,original.s.feedbackCount);}
 assert(on.s.trace.sequence>0);assert.equal(off.s.trace.sequence,0);
});
test('fixed recent ring and first boundaries explicitly report loss; GAP is preserved',()=>{
 const {s}=make();s.down(1,64,100);s.tick(0,'NS','TEST');for(let i=1;i<=700;i++)s.tick(i*16000000,'NS','TEST');s.tick(20000000000,'NS','TEST');const before=s.trace.sequence,data=cold(s);assert.equal(data.recentCount,512);assert(data.overwritten>0&&data.incomplete);assert.equal(s.trace.rows.length,576*32);assert.equal(data.typedPayloadBytes,147712);assert(data.data.some((r,i)=>r[1]===3&&data.reasons[i]==='GAP'&&r[9]===1&&r[10]===-1));assert.equal(s.trace.sequence,before);assert.equal(data.coverage,'RECENT_WINDOW_PLUS_FIRST_BOUNDARIES_NOT_FULL_RUN');
 for(let i=0;i<70;i++){s.down(i,64,100);s.up(i);}assert(s.trace.boundaryDropped>0);assert.equal(s.trace.boundaries,64);
});
test('oversized metadata is explicit lost evidence, cold export does not change authority',()=>{
 const {e,s}=make();s.tick(1,'NS','x'.repeat(129));assert(s.trace.metadataDropped>0);const before=authority(e);const data=cold(s);assert(data.incomplete);assert.equal(authority(e),before);assert.equal(s.trace.rows.length,18432);
});
test('page shows terminal separately from CURRENT and offers explicit checksummed cold export',()=>{
 const p=fs.readFileSync('entry/src/main/ets/pages/ClearWaterDemo.ets','utf8');assert(p.includes('Text(this.sourceFeedback)'));assert(p.includes('configuredWaterRate'));assert(p.includes("Button('导出源时钟收据')"));assert(p.includes('DiagnosticColdFiles'));assert(p.includes('this.syncSourceFeedback();this.schedule()'));assert(!p.includes('Canvas('));
});
const report={revision:'T321_SOURCE_CLOCK_UNITS_V1',passed:cases.filter(x=>x.status==='PASS').length,failed:cases.filter(x=>x.status==='FAIL').length,device:'NOT_RUN',baselineCommit:'eee20b3',cases};fs.writeFileSync('test/reports/t321/units.json',JSON.stringify(report,null,2));console.log(JSON.stringify(report));process.exitCode=report.failed?1:0;
