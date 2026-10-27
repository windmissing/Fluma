'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P011StrictWriterKernels,P011KernelFrozen,P011KernelRequest}=
  require('../entry/src/main/ets/physics/P011WriterKernels.ets');
const {p011KernelStatus}=require('../entry/src/main/ets/physics/P011WriterRegistry.ets');
let passed=0;function test(name,fn){fn();passed++;console.log('PASS '+name);}
function fixture(kernel){const f=new P011KernelFrozen(3,2),r=new P011KernelRequest();
  f.ownerId=7;f.snapshotSerial=5;f.identity.runId='r';f.identity.attemptId='a';
  f.identity.snapshotSerial=5;f.identity.writerKernelRevision=kernel;
  f.capacity=100;f.hardMax=10;f.eps=1e-9;
  f.waterTop.fill(50);r.kernel=kernel;r.column=1;
  return {f,r,k:new P011StrictWriterKernels(3)};}
function preview(f,r,k){ r.requested=Math.max(r.requested,r.qGranted);f.ticketToken++;r.ticketToken=f.ticketToken;f.ticketState=1;f.ticketDebit=r.actualDebit;f.ticketRefund=0;
  if(r.kernel==='LAND_RN_GRAPH_V1'&&r.slot>=0&&f.liveCount===0){f.liveCount=1;f.freeCount=f.poolMass.length-1;f.freeSlots[0]=1;f.cacheH[r.column]=r.phase===0?f.poolMass[r.slot]:0;f.cacheL[r.column]=r.phase===1?f.poolMass[r.slot]:0;}
  return k.preview(f,r);}
test('registry excludes unproved writers and first airborne contact',()=>{
  assert.equal(p011KernelStatus('PERCOLATE','CONTACT_WAIT'),'STRICT_WRITER_UNSUPPORTED');
  assert.equal(p011KernelStatus('LAND_RN_GRAPH_V1','AIRBORNE'),'CONTACT_HOLD_UNSUPPORTED');});
test('settle uses actual RN phase sum, not qControl',()=>{
  const {f,r,k}=fixture('SETTLE_RN_GRAPH_V1');f.suspH[1]=1;f.suspL[1]=1;
  r.originalRequested=.3;r.qGranted=.15;r.actualDebit=.15;r.dt=.01;
  r.heavySpeed=500;r.lightSpeed=1250;
  const s=preview(f,r,k);assert.equal(s.decision,'RAW');
  assert.notEqual(s.phaseHeavy+s.phaseLight,r.qGranted);
  assert.equal(s.phaseHeavy+s.phaseLight,.175);assert.ok(s.ALo>1);
  const h=s.phaseHeavy,l=s.phaseLight;assert.equal(k.commit(f,s,7,f.snapshotSerial,s.token),'OK');
  assert.equal(f.mh[1],h);assert.equal(f.ml[1],l);
  assert.equal(f.suspH[1],1-h);assert.equal(f.suspL[1],1-l);});
test('top MIX preserves source split and receiver by phase',()=>{
  const {f,r,k}=fixture('TOP_SPLIT_RN_GRAPH_V1');f.mh[1]=.1;f.ml[1]=.2;
  f.surface[1]=.30000000000000004;r.receiver=2;r.qGranted=.1;r.actualDebit=.1;
  const s=preview(f,r,k);assert.equal(s.decision,'RAW');assert.equal(s.patch.branch,'MIX');
  assert.equal(s.patch.changedCount,2);const h=s.phaseHeavy,l=s.phaseLight;
  assert.equal(k.commit(f,s,7,f.snapshotSerial,s.token),'OK');assert.equal(f.mh[2],h);assert.equal(f.ml[2],l);});
test('LAND sub-ULP partial cannot increase bed without pool debit',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');f.poolMass[0]=1;f.poolId[0]=21;
  f.poolState[0]=2;f.poolColumn[0]=1;f.poolPhase[0]=0;
  r.slot=0;r.particleId=21;r.qGranted=2**-54;r.actualDebit=r.qGranted;
  const s=preview(f,r,k);assert.equal(s.decision,'ZERO');assert.equal(s.patch.changedCount,0);
  assert.equal(k.commit(f,s,7,f.snapshotSerial,s.token),'OK');assert.equal(f.poolMass[0],1);assert.equal(f.mh[1],0);});
test('LAND partial/full retains then retires same ID',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');f.poolMass[0]=1;f.poolId[0]=21;
  f.poolState[0]=2;f.poolColumn[0]=1;f.poolPhase[0]=0;
  r.slot=0;r.particleId=21;r.qGranted=2**-53;r.actualDebit=r.qGranted;
  let s=preview(f,r,k);assert.equal(s.decision,'RAW');assert.ok(s.poolRemainder<1);
  assert.equal(k.commit(f,s,7,f.snapshotSerial,s.token),'OK');assert.equal(f.poolId[0],21);
  r.qGranted=f.poolMass[0];r.actualDebit=r.qGranted;s=preview(f,r,k);
  assert.equal(s.decision,'RAW');assert.equal(s.poolRetired,true);
  assert.equal(k.commit(f,s,7,f.snapshotSerial,s.token),'OK');assert.equal(f.poolState[0],0);
  assert.equal(f.poolId[0],21);});
test('owner mismatch blocks commit before field write',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');f.poolMass[0]=1;f.poolId[0]=21;
  f.poolState[0]=2;f.poolColumn[0]=1;r.slot=0;r.particleId=21;
  r.qGranted=.5;r.actualDebit=.5;const s=preview(f,r,k);assert.equal(s.decision,'RAW');
  assert.equal(k.commit(f,s,8,f.snapshotSerial,s.token),'IDENTITY_MISMATCH');assert.equal(f.mh[1],0);});
test('unsupported first contact does not mutate anything',()=>{
  const {f,r,k}=fixture('LAND_RN_GRAPH_V1');r.particleState='AIRBORNE';
  const s=preview(f,r,k);assert.equal(s.decision,'UNSUPPORTED');
  assert.equal(s.reason,'CONTACT_HOLD_UNSUPPORTED');assert.equal(s.patch.changedCount,0);});
function state(f){return JSON.stringify(f,(k,v)=>ArrayBuffer.isView(v)?Array.from(v,x=>Object.is(x,-0)?'-0':x):v);}
function landing(){const x=fixture('LAND_RN_GRAPH_V1'),{f,r}=x;
  f.poolMass[0]=1;f.poolId[0]=21;f.poolState[0]=2;f.poolColumn[0]=1;
  f.poolSource[0]=2;f.poolSourceId[0]=73;f.poolY[0]=3;f.poolVx[0]=.2;
  f.poolVy[0]=0;f.poolX[0]=1.25;r.slot=0;r.particleId=21;
  r.qGranted=.5;r.actualDebit=.5;return x;}
test('LAND INWARD solves exact three-edge bound, refunds once and retires no slot',()=>{
  const {f,r,k}=landing();f.hardMax=.2;f.dry=true;
  const s=preview(f,r,k),token=s.token;
  assert.equal(s.decision,'INWARD');assert.equal(s.previewCount,2);
  assert.ok(s.qControl>0&&s.qControl<.2);assert.equal(s.qControl,s.selectedDomainHi);
  const q=s.qControl;assert.equal(k.commit(f,s,7,5,token),'OK');
  assert.equal(f.poolMass[0],1-q);assert.equal(f.cacheH[1],1-q);
  assert.equal(f.poolId[0],21);assert.equal(f.poolSource[0],2);assert.equal(f.poolSourceId[0],73);
  assert.equal(f.poolY[0],3);assert.equal(f.poolVx[0],.2);assert.equal(f.nextId,1);
  assert.equal(f.eventCount,1);assert.equal(f.lineageCount,1);assert.equal(f.freeCount,1);
  assert.equal(f.totalRemaining,.5-q);assert.equal(f.ticketRefund,.5-q);
  assert.equal(f.waterTop[1],f.surface[1]+f.cacheH[1]);
  const snapshot=state(f);assert.equal(k.commit(f,s,7,6,token),'STALE_TOKEN');assert.equal(state(f),snapshot);
});
test('settle INWARD and top MIX INWARD use the same frozen input',()=>{
  {const {f,r,k}=fixture('SETTLE_RN_GRAPH_V1');f.hardMax=.05;f.suspH[1]=1;
    r.originalRequested=.1;r.qGranted=.1;r.actualDebit=.1;r.heavySpeed=500;r.dt=.01;
    const s=preview(f,r,k);assert.equal(s.decision,'INWARD');assert.ok(s.qControl<.05);
    assert.equal(s.previewCount,2);assert.ok(s.ALo<=1&&s.AHi>=1);}
  {const {f,r,k}=fixture('TOP_SPLIT_RN_GRAPH_V1');f.hardMax=.2;f.mh.fill(1);f.surface.fill(1);
    r.receiver=2;r.qGranted=.5;r.actualDebit=.5;const s=preview(f,r,k);
    assert.equal(s.decision,'INWARD');assert.ok(s.qControl>0&&s.qControl<.1);
    assert.equal(s.patch.branch,'MIX');assert.equal(s.previewCount,2);}
});
test('top later branches only RAW/ZERO and retain their RN bias',()=>{
  for(const [bl,q,branch] of [[.3,.4,'MIX_LIGHT'],[.05,.45,'MIX_LIGHT_HEAVY']]){
    const {f,r,k}=fixture('TOP_SPLIT_RN_GRAPH_V1');f.bh[1]=.4;f.bl[1]=bl;
    f.mh[1]=.1;f.ml[1]=.2;f.surface[1]=((.4+bl)+.1)+.2;
    r.receiver=2;r.qGranted=q;r.actualDebit=q;const s=preview(f,r,k);
    assert.equal(s.decision,'RAW');assert.equal(s.patch.branch,branch);
    assert.ok(s.bLo<0||s.bHi>0);assert.ok(s.envelope.nodeCount>20);
  }
});
test('all participants are checked before commit, stale token and forged public patch cannot write',()=>{
  for(const mutate of [f=>f.bh[0]=1,f=>f.sourceH[1]=.01,f=>f.poolSourceId[0]++,
    f=>f.ticketDebit+=1,f=>f.identity.configRevision++,f=>f.freeSlots[1]=19]){
    const {f,r,k}=landing();const s=preview(f,r,k),token=s.token;mutate(f);
    const before=state(f);assert.equal(k.commit(f,s,7,5,token),'IDENTITY_MISMATCH');assert.equal(state(f),before);
  }
  const {f,r,k}=landing();let s=preview(f,r,k),old=s.token;s=k.preview(f,r);
  assert.equal(k.commit(f,s,7,5,old),'STALE_TOKEN');
  const token=s.token;s.patch.afterBits.fill(99);s.qControl=99;
  assert.equal(k.commit(f,s,7,5,token),'OK');assert.equal(f.mh[1],.5);
});
test('pool/capacity audit rejects without writes and ZERO preserves signed material bits',()=>{
  const {f,r,k}=landing();f.liveCount=1;f.cacheH[1]=.25;f.freeCount=1;f.freeSlots[0]=1;
  f.ticketToken=r.ticketToken=1;f.ticketDebit=.5;f.ticketState=1;r.requested=.5;
  let before=state(f);let s=k.preview(f,r);assert.equal(s.decision,'UNSUPPORTED');assert.equal(state(f),before);
  const z=landing();z.f.hardMax=0;z.f.ml[1]=-0;s=preview(z.f,z.r,z.k);
  assert.equal(s.decision,'ZERO');const bits=Array.from(z.f.ml,x=>Object.is(x,-0));
  assert.equal(z.k.commit(z.f,s,7,5,s.token),'OK');assert.deepEqual(Array.from(z.f.ml,x=>Object.is(x,-0)),bits);
  assert.equal(z.f.poolMass[0],1);assert.equal(z.f.eventCount,0);assert.equal(z.f.ticketState,3);
});
// Independent exact rational reconstruction of each recorded RN node.
const bitBuffer=Buffer.alloc(8);
function gcd(a,b){while(b){const t=a%b;a=b;b=t;}return a<0n?-a:a;}
function rat(n,d=1n){if(d<0n){n=-n;d=-d;}const g=gcd(n,d);return [n/g,d/g];}
function dy(v){bitBuffer.writeDoubleBE(v);const bits=bitBuffer.readBigUInt64BE();
  const e=Number((bits>>52n)&2047n),m=(bits&((1n<<52n)-1n))+(e?1n<<52n:0n),p=(e?e-1075:-1074);
  const sign=bits>>63n?-1n:1n;return p>=0?rat(sign*m*(1n<<BigInt(p))):rat(sign*m,1n<<BigInt(-p));}
function rop(op,a,b){if(op===1)return rat(a[0]*b[1]+b[0]*a[1],a[1]*b[1]);
  if(op===2)return rat(a[0]*b[1]-b[0]*a[1],a[1]*b[1]);
  if(op===3)return rat(a[0]*b[0],a[1]*b[1]);return rat(a[0]*b[1],a[1]*b[0]);}
function le(a,b){return a[0]*b[1]<=b[0]*a[1];}
test('recorded RN node ideal/error/eta contains independent exact expression',()=>{
  const {f,r,k}=fixture('SETTLE_RN_GRAPH_V1');f.suspH[1]=1;f.suspL[1]=.75;
  f.sourceH[1]=.3;f.sourceL[1]=.2;r.sourceActive=true;
  r.originalRequested=.3;r.qGranted=.15;r.actualDebit=.15;r.dt=.01;r.heavySpeed=10;r.lightSpeed=25;
  const s=preview(f,r,k);assert.equal(s.decision,'RAW');const e=s.envelope,ideal=[];
  assert.ok(e.nodeCount>40);assert.equal(e.nodeCount,s.nodeCount);
  for(let i=0;i<e.nodeCount;i++){
    ideal[i]=e.op[i]===7?dy(e.actual[i]):rop(e.op[i],ideal[e.inputA[i]],ideal[e.inputB[i]]);
    const error=rop(2,dy(e.actual[i]),ideal[i]);
    assert.ok(le(dy(e.errorLo[i]),error)&&le(error,dy(e.errorHi[i])),`node ${i} error`);
    assert.ok(le(dy(e.idealLo[i]),ideal[i])&&le(ideal[i],dy(e.idealHi[i])),`node ${i} ideal`);
  }
});
test('top and LAND node certificates reconstruct exactly, and delta0 is explicit',()=>{
  const cases=[];
  {const x=fixture('TOP_SPLIT_RN_GRAPH_V1'),{f,r}=x;
    f.bh[1]=.1;f.bl[1]=.2;f.mh[1]=.3;f.ml[1]=.4;f.surface[1]=((.1+.2)+.3)+.4;
    r.receiver=2;r.qGranted=.2;r.actualDebit=.2;cases.push(x);}
  cases.push(landing());
  for(const {f,r,k} of cases){const s=preview(f,r,k),e=s.envelope,ideal=[];
    assert.equal(s.decision,'RAW');
    for(let i=0;i<e.nodeCount;i++){
      ideal[i]=e.op[i]===7?dy(e.actual[i]):rop(e.op[i],ideal[e.inputA[i]],ideal[e.inputB[i]]);
      const error=rop(2,dy(e.actual[i]),ideal[i]);
      assert.ok(le(dy(e.errorLo[i]),error)&&le(error,dy(e.errorHi[i])),`${r.kernel} node ${i}`);
    }
    if(r.kernel==='TOP_SPLIT_RN_GRAPH_V1')assert.ok(s.delta0Lo[0]!==0||s.delta0Hi[0]!==0);
  }
});
console.log(`T135 ${passed}/${passed} PASS`);
