// D2 tests execute the product ArkTS modules, without a device.
'use strict';
const fs=require('fs'),assert=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
 {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {WaterSandField:F}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const m=require('../entry/src/main/ets/physics/P011ParticleM2Field.ets');
const route='dual-field-particle-v1', results=[];
const near=(a,b,eps=1e-9)=>assert.ok(Math.abs(a-b)<=eps,`${a} != ${b}`);
function test(name,fn){try{fn();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:e.stack});}}
function field(nmax=512){const f=new F(128,512),p=m.defaultP011ParticleParams();p.nmax=nmax;
 assert.equal(f.setParticleRoute(route,p),0);f.setNaturalObservationRouteId(route);return f;}
function inject(f,h=.2,l=.1,id=1){const a=new Float64Array(128),b=new Float64Array(128);a[64]=h;b[64]=l;
 return f.injectGestureSand({perColumnHeavy:a,perColumnLight:b,sourceId:id,sequence:id,inputTime:id,x:64.5,y:100,seed:13});}
function tick(f,n){for(let i=0;i<n;i++){const r=f.step(1/60);assert.ok(r.ok,`step ${i} error ${r.errorCode}`);}}
test('01 canonical totals and external ledger include particles',()=>{const f=field(),a=f.getTotals(),r=inject(f);near(r.admittedH,.2);near(r.admittedL,.1);const b=f.getTotals();near(b.heavy-a.heavy,.2);near(b.light-a.light,.1);near(f.getDrift().heavy,0);});
test('02 gesture sink is exclusive with column suspension',()=>{const f=field(),h=f.suspH[64],l=f.suspL[64];inject(f);near(f.suspH[64],h);near(f.suspL[64],l);near(f.particleBinding.pool.totals().heavy,.2);});
test('03 route double gate and conflict preserve prior state',()=>{const f=new F(128,512),p=m.defaultP011ParticleParams();f.setNaturalObservationRouteId(route);assert.equal(f.particleRouteActive(),false);assert.equal(f.setParticleRoute('wrong',p),2);assert.equal(f.setParticleRoute(route,p),0);const c=new F(128,512);c.setNaturalObservationRouteId('other');assert.equal(c.setParticleRoute(route,p),4);assert.equal(c.particleBinding,null);});
test('04 real local clip truncates before scale and charges total once',()=>{const f=field();for(const scale of [0,.2,1])for(const local of [.01,.2,1])for(const total of [.03,.4,5]){f.beginMotionApplication(1/60,scale);f.substepMotionBudgetRemaining[4]=local;f.substepMotionBudgetRemaining[5]=local/2;f.totalMotionBudgetRemaining=total;const oldBottom=f.bottomCreepBudgetUsed,g=f.clipSubstepMotion(4,5,.6);near(g,Math.min(.6,local/2)*scale>total?total:Math.min(.6,local/2)*scale);near(f.substepMotionBudgetUsed[4],g);near(f.substepMotionBudgetUsed[5],g);near(f.totalMotionBudgetRemaining,total-g);near(f.bottomCreepBudgetUsed,oldBottom);}});
test('05 live inventory refuses unload without loss',()=>{const f=field();inject(f);const a=f.particleBinding.pool.snapshot();assert.equal(f.setParticleRoute(null,null),1);f.setNaturalObservationRouteId(null);assert.equal(f.particleRouteActive(),true);assert.deepEqual(f.particleBinding.pool.snapshot(),a);});
test('06 capacity failure leaves source ledger and pool intact',()=>{const f=field(1),a=f.getTotals(),r=inject(f,.2,.1);near(r.admittedH+r.admittedL,0);near(f.getTotals().heavy,a.heavy);assert.equal(f.particleBinding.liveCount,0);near(f.sourceEpoch,0);});
test('07 initialize clears pool and permits unload',()=>{const f=field();inject(f);f.initialize(f.defaultScene());assert.equal(f.particleBinding.liveCount,0);assert.equal(f.setParticleRoute(null,null),0);});
test('08 OFF armed empty pool matches legacy fields bit for bit',()=>{const a=new F(128,512),b=new F(128,512);assert.equal(b.setParticleRoute(route,m.defaultP011ParticleParams()),0);for(let i=0;i<20;i++){a.setTilt(i%3===0?4:0);b.setTilt(i%3===0?4:0);assert.equal(a.step(1/60).ok,b.step(1/60).ok);}for(const k of ['waterTop','bh','bl','mh','ml','suspH','suspL'])assert.deepEqual(a[k],b[k],k);});
test('09 live pool wakes dormant and committed pages drain each step',()=>{const f=field();inject(f);f.dormant=true;tick(f,3);assert.equal(f.dormant,false);assert.equal(f.particleBinding.work.pageBacklog,0);});
test('10 invert preserves mass and stays within continuous column domain',()=>{const f=field();inject(f);const a=f.getTotals(),before=f.particleBinding.pool.snapshot();f.invert180();const s=f.particleBinding.pool.snapshot();for(let i=0;i<s.nmax;i++)if(s.pState[i]){assert.ok(Number.isFinite(s.pX[i])&&s.pX[i]>=0&&s.pX[i]<128);near(s.pX[i],128-before.pX[i]);}near(f.getTotals().heavy,a.heavy);near(f.getTotals().light,a.light);f.invert180();near(f.getTotals().heavy,a.heavy);});
test('11 schema 4 roundtrip and schema 3 structural zero',()=>{const f=field();inject(f);const s=f.snapshot();assert.equal(s.schemaVersion,4);const g=field();g.restoreFromSnapshot(s);assert.deepEqual(g.particleBinding.pool.snapshot(),s.particleState);g.restoreFromSnapshot({...s,schemaVersion:3,particleState:undefined});assert.equal(g.particleBinding.liveCount,0);});
test('12 injection eta keeps free water invariant',()=>{const f=field(),water=f.getTotals().water;inject(f,.3,.1);near(f.getTotals().water,water,1e-6);});
test('13 admitted source above 0.1 percent survives real steps',()=>{const f=field(),r=inject(f,10,5);assert.ok(r.admittedH+r.admittedL>10);tick(f,8);assert.ok(f.getDrift().heavy<1e-9);assert.ok(f.getDrift().light<1e-9);});
test('P1 gesture plus shake closes both phase inventories',()=>{const f=field(1024),initial=f.getTotals();inject(f,.4,.2);f.setShakeInput(1);tick(f,30);const t=f.getTotals();near(t.heavy,initial.heavy+.4,1e-8);near(t.light,initial.light+.2,1e-8);const pool=f.particleBinding.pool;assert.ok(pool.sourceTotals(1).count>0);f.setShakeInput(0);tick(f,30);assert.ok(f.getDrift().heavy<1e-9);assert.ok(f.getDrift().light<1e-9);});
console.log(JSON.stringify({suite:'p011_particle_m2',passed:results.filter(x=>x.status==='PASS').length,failed:results.filter(x=>x.status==='FAIL').length,results},null,2));
if(results.some(x=>x.status==='FAIL'))process.exitCode=1;
