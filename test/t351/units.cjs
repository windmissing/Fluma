'use strict';const assert=require('assert/strict'),fs=require('fs'),crypto=require('crypto'),{loader}=require('./load.cjs');
const roots=[require('./baseline.cjs'),'entry/src/main/ets'],loads=roots.map(loader),ctors=loads.map(load=>load('physics/derived/P051DerivedFacadeV2').P051DerivedFacadeV2);
const cfg=n=>({isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:n,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'});
const fixed=Date.now;Date.now=()=>1234567890;
const cases=[];function test(name,fn){try{fn();cases.push({name,status:'PASS'})}catch(e){cases.push({name,status:'FAIL',error:String(e.stack)})}}
function peers(n){return ctors.map(C=>new C(cfg(n)));}
function req(i,m=.125){return {revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:i,inputTimeMs:i*100,x:64.5,y:220,seed:51,requestedH:m,requestedL:0};}
function bytes(x){if(ArrayBuffer.isView(x))return {type:x.constructor.name,bytes:Buffer.from(x.buffer,x.byteOffset,x.byteLength).toString('hex')};if(x===null||typeof x!=='object')return x;const out={};for(const k of Object.keys(x)){if(k==='runId'||k==='run'||k==='owner'||k==='ownerId')continue;out[k]=bytes(x[k]);}return out;}
function comparable(f){return bytes(f.exportCheckpointCold());}
function compare(p){assert.deepEqual(comparable(p[0]),comparable(p[1]));const a=p[0].countersCold(),b=p[1].countersCold();for(let i=0;i<a.length;i++)if(i!==13)assert.equal(b[i],a[i],'counter '+i);}
test('N0 remains unsupported; raw zero-pool copy counts actual writes',()=>{
 for(const C of ctors)assert.throws(()=>new C(cfg(0)),/UNSUPPORTED/);
 const create=loads[1]('physics/derived/P011DerivedSnapshotV2');const a=create.createDerivedPayloadV2(0),b=create.createDerivedPayloadV2(0);assert.equal(create.copyDerivedPayloadV2(a,b),14*128*8);
});
for(const n of [2,16,64,192,512])test('bitwise/checkpoint/source/commits/counters parity N='+n+' Q0/1/2 A0/1',()=>{
 const p=peers(n);compare(p);const slots=p.map(f=>f.createPublishedSlotCold());
 const ops=[f=>f.step(0),f=>f.inject(req(1,Math.min(n*.125,2))),f=>f.step(1/60),f=>f.step(1/30),f=>f.inject(req(1)),f=>f.setTilt(10),f=>f.step(1/60),f=>f.step(-1)];
 for(const op of ops){const r=p.map(op);assert.deepEqual(bytes(r[0]),bytes(r[1]));for(let i=0;i<2;i++)assert.equal(p[i].copyPublishedSceneStateInto(slots[i]),'OK');compare(p);}
});
test('normal publication creates zero new byte views; every compared byte retained',()=>{
 const p=peers(512),c=p[1].field.derivedPublicationV2;assert.equal(c.byteViews.length,63);const identities=c.byteViews.slice();let slow=0;const old=c.byteView;c.byteView=function(b,i){if(this.byteBuffers[i]!==b)slow++;return old.call(this,b,i);};p.forEach(f=>assert(f.inject(req(1,1)).ok));compare(p);p.forEach(f=>assert(f.step(1/30).ok));compare(p);assert.equal(slow,0);assert(c.byteViews.every((v,i)=>v===identities[i]));
});
for(const live of [128,512])test('upper live bitwise payload/acc/cache parity N512 L'+live,()=>{
 const p=peers(512);let seq=0;
 for(let mass=live*.1;mass>1e-9;){const m=Math.min(mass,32);seq++;p.forEach(f=>assert(f.inject(req(seq,m)).ok));mass-=m;compare(p);}
 assert.equal(p[1].field.derivedV2.canonical.pool.liveCount,live);
 for(let i=0;i<4;i++){p.forEach(f=>assert(f.step(1/30).ok));compare(p);}
});
for(const n of [16,64,512])test('contact R/N request counts and publication bytes close N'+n,()=>{
 const p=peers(n);p.forEach(f=>assert(f.inject({...req(1,Math.min(n*.1,32)),y:.001}).ok));const b=p[1].budget;b.enabled=true;b.begin(1,33333333,1/30);
 p.forEach(f=>assert(f.step(1/30).ok));b.end();compare(p);const row=JSON.parse(b.exportCold()).rows[0];assert(row[9]>0);assert.equal(row[10],row[7]*(55*n+9*128*8));assert.equal(row[16],row[7]*(55*n+10*128*8));assert.equal(row[22],0);
});
for(const mutation of ['structure','mass','cache','duplicateId','binding','field','externalWriter','rollback'])test('first error/frozen evidence/order parity '+mutation,()=>{
 const p=peers(16);p.forEach(f=>assert(f.inject(req(1,1)).ok));const outcomes=[];
 for(const f of p){const d=f.field.derivedV2,q=d.canonical.pool;
  if(mutation==='structure'){q.freeSlots[0]=-1;q.pMass[0]=NaN;}
  if(mutation==='mass')q.pMass[0]=NaN;
  if(mutation==='cache')q.colParticleMassH[64]=-0;
  if(mutation==='duplicateId')q.pId[1]=q.pId[0];
  if(mutation==='binding')q.pMass=q.pMass.slice();
  if(mutation==='field')f.field.mh[0]=NaN;
  if(mutation==='externalWriter'){const verify=d.verifyPublicBoundary.bind(d);d.verifyPublicBoundary=(boundary)=>{const code=verify(boundary);if(boundary.startsWith('EXTERNAL_ADMISSION'))q.pMass[0]*=1.01;return code;};outcomes.push(bytes(f.inject(req(2))));}
  else if(mutation==='rollback'){f.field.audit=()=>5;outcomes.push(bytes(f.step(1/60)));}
  else outcomes.push(d.verifyPublicBoundary('T351_BAD'));
 }
 assert.deepEqual(outcomes[0],outcomes[1]);for(let i=0;i<2;i++){const raw=p[i].exportFailureCold();if(raw){raw.fault.owner=0;raw.scalarNames.forEach((key,j)=>{if(key==='renderCommit'){} });}outcomes[i]=bytes(raw);}assert.deepEqual(outcomes[0],outcomes[1]);
});
test('byte comparison mismatch/detach/alias/replacement preserves exact decisions and counted prefix',()=>{
 const p=peers(8),a=p[0].field.derivedPublicationV2,b=p[1].field.derivedPublicationV2;
 for(const mode of ['same','mismatch','length','alias','replacement','detach']){
  const x=new Uint8Array(8),y=new Uint8Array(8);if(mode==='mismatch')y[3]=1;if(mode==='detach')structuredClone(x.buffer,{transfer:[x.buffer]});const yb=mode==='length'?new ArrayBuffer(4):mode==='alias'?x.buffer:y.buffer;
  const run=c=>{const count=c.owner.counters.values[16];try{return [c.sameBuffer(x.buffer,yb),c.owner.counters.values[16]-count];}catch(e){return [e.constructor.name,c.owner.counters.values[16]-count];}};assert.deepEqual(run(a),run(b),mode);
 }
});
test('trace ON/OFF bitwise parity and exclusive sum = full frame wall; actual Q/A/bytes/requests',()=>{
 const p=[new ctors[1](cfg(64)),new ctors[1](cfg(64))],slots=p.map(f=>f.createPublishedSlotCold());p[1].budget.enabled=true;
 for(let frame=0;frame<5;frame++){const budget=p[1].budget;budget.begin(frame,16666667,frame===0?0:1/30);for(let i=0;i<2;i++){if(frame===1)assert(p[i].inject(req(1,.5)).ok);assert(p[i].step(frame===0?0:1/30).ok);assert.equal(p[i].copyPublishedSceneStateInto(slots[i]),'OK');}budget.end();assert.deepEqual(comparable(p[0]),comparable(p[1]));const data=JSON.parse(budget.exportCold()),row=data.rows.at(-1);assert.equal(row.slice(24,44).reduce((a,b)=>a+b,0),row[19]);assert.equal(row[22],0);assert.equal(row[8],frame===1?1:0);assert.equal(row[7],frame===0?0:2);assert.equal(row[10],(row[7]+row[8])*(55*64+9*128*8));assert.equal(row[11],55*64+14*128*8);assert.equal(row[17],0);const children=data.substeps.filter(s=>s[0]===frame);assert.equal(children.length,row[7]);for(const s of children){assert.equal(s[3],0);assert.equal(s[7],1);assert.equal(s.slice(24,44).reduce((a,b)=>a+b,0),s[6]);assert(s[14]>0&&s[15]>0);}}
});
test('bounded trace overflow/negative clock/disabled and metadata limits are explicit',()=>{
 const {FrameBudgetV2:B}=loads[1]('physics/derived/P011FrameBudgetV2'),b=new B();b.begin(0,0,0);assert.equal(b.enter(1),-1);assert.equal(b.count,0);b.enabled=true;for(let i=0;i<140;i++){b.begin(i,1e6,.001);const s=b.enter(3);b.leave(s);b.end();}assert.equal(b.count,128);assert.equal(b.overwritten,12);b.begin(200,1e6,.001);b.identity('x'.repeat(257),1,1,2,0);b.end();assert(b.invalid>0);assert.equal(b.rows.length,128*128);assert.equal(b.current.length,128);
 const before=Date.now;let t=100;Date.now=()=>t--;b.begin(201,0,0);b.end();Date.now=before;assert(b.invalid>1);const invalid=b.invalid;b.begin(202,0,0);b.add(9,NaN);b.end();assert(b.invalid>invalid);assert.equal(JSON.parse(b.exportCold()).incomplete,true);
});
Date.now=fixed;const report={revision:'T351_DIFFERENTIAL_V1',passed:cases.filter(x=>x.status==='PASS').length,failed:cases.filter(x=>x.status==='FAIL').length,device:'NOT_RUN',cases};fs.writeFileSync('test/reports/t351/units.json',JSON.stringify(report,null,2));console.log(JSON.stringify(report));process.exitCode=report.failed?1:0;
