const fs=require('node:fs'),Module=require('node:module'),assert=require('node:assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const m=new Module(__filename);m._compile(ts.transpileModule(fs.readFileSync('test/t426/PrepareReadiness.ets','utf8')+'\n'+fs.readFileSync('test/t426/PrepareReadinessV4.ets','utf8').replace(/^import[^\n]*$/mg,''),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText,__filename);
const {PrepareAttemptOwner,copyPrepareOwner,samePrepareOwner,prepareOwnerKey,PrepareReadinessV4}=m.exports;
const fields=['prepareId','pageGeneration','branch','windowId','windowGeneration','controller','session','surface','context','run'];
const o=new PrepareAttemptOwner();for(const [i,k] of fields.entries())o[k]=k==='run'?'fixture':i+1;
const saved=copyPrepareOwner(o);assert(saved);assert(samePrepareOwner(saved,o));assert.equal(JSON.parse(prepareOwnerKey(saved)).length,10);
for(const k of fields){const changed={...o,[k]:k==='run'?'foreign':o[k]+1};assert(!samePrepareOwner(saved,changed),k);}
o.context++;assert.notEqual(saved.context,o.context);assert.equal(copyPrepareOwner({...o,controller:0}),null);assert.equal(copyPrepareOwner({...o,windowId:Number.MAX_SAFE_INTEGER+1}),null);
const gate=new PrepareReadinessV4(()=>{});gate.show();assert(gate.begin(0,true));const owner={...saved,prepareId:gate.prepareId,pageGeneration:gate.pageGeneration};assert(gate.bindOwner(owner));const serial=gate.ownerSerial;assert(gate.bindOwner({...owner}));assert.equal(gate.ownerSerial,serial);const snapshot=gate.ownerSnapshot();snapshot.run='mutated';assert.equal(gate.ownerSnapshot().run,owner.run);assert.equal(gate.consumeTicket({valid:false}),false);
console.log(JSON.stringify({result:'PASS',checks:'ten owner fields, immutable private copy, safe integers, fresh equal identity, one-shot invalid-ticket rejection',device:'NOT_RUN'}));
const {PrepareSnapshot,PrepareRect}=m.exports;
function readyGate(){
 const g=new PrepareReadinessV4(()=>{});g.show();for(let i=0;i<3;i++)g.inputCreated(i);assert(g.begin(100,true));
 const s=new PrepareSnapshot();Object.assign(s,{windowId:4,windowGeneration:1,windowActive:true,windowRegistered:true,keyboardConfiguration:'TESTER_CONFIRMED_FIXED_SYSTEM_KEYBOARD',deviceType:'phone',windowMode:1,keyboardHeight:0,geometry:1,nativeGeometry:1,controller:5,session:6,surface:7,context:8,run:'fixture',nativeReady:true,viewportWidth:800,viewportHeight:600,fullyVisible:true,componentPresent:true,transformIdentity:true,globalX:0,globalY:0,widthVp:400,heightVp:300,vp2px:2});
 s.avoid=Array.from({length:4},()=>new PrepareRect());for(const field of ['windowRect','component','screen','vpBoxWindowPx','vpBoxScreenPx'])Object.assign(s[field],{x:0,y:0,width:800,height:600});
 g.observe(s);assert(g.bindOwner({...s,prepareId:g.prepareId,pageGeneration:g.pageGeneration,branch:1}));g.frame(1,100);g.frame(2,116);assert.equal(g.state,'READY',g.reason);return {g,s};
}
{
 const {g,s}=readyGate();const t=g.checkForReserve(s,10000);assert(t.valid,'READY has no TTL');assert(g.consumeTicket(t));assert(!g.consumeTicket(t),'second consume must reject');
}
for(const field of ['prepareId','pageGeneration','observationSerial','inputSerial','ownerSerial','owner']){
 const {g,s}=readyGate();const t=g.checkForReserve(s,120);t[field]=typeof t[field]==='number'?t[field]+1:'foreign';assert(!g.consumeTicket(t),field);
}
{
 const {g,s}=readyGate();const t=g.checkForReserve(s,120);assert(t.valid);g.observe({...s,nativeReady:false});assert(!g.consumeTicket(t),'readiness fell');
}
console.log(JSON.stringify({actualGateTickets:'PASS',checks:['READY no TTL','one-shot','six ticket fields','readiness fall'],device:'NOT_RUN'}));
