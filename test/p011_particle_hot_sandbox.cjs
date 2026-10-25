'use strict';
const hot=require('../entry/src/main/ets/physics/P011ParticleHot.ets');
const {P011ParticleField}=require('../entry/src/main/ets/physics/P011ParticleField.ets');
const {P011ParticleHotPrototypeSink}=require('../entry/src/main/ets/physics/P011ParticleHotPrototype.ets');
const {registerParticleHotRun}=require('../entry/src/main/ets/physics/P011ParticleHotEvidence.ets');
function makeSandbox(nmax=512,eventCapacity=4096){
  const params={massPackage:.1,nmax,terminalSpeedHeavy:64,terminalSpeedLight:38,dragHeavy:2,dragLight:2,
    liftSpeedHeavy:8,liftSpeedLight:8,liftSpread:2,mergeColRadius:1,mergeHeightRadius:2,contactEps:1e-6,channelRateCap:0};
  const work=hot.configureHot({cols:128,nmax,requestCapacity:4096,motionCapacity:nmax,eventCapacity,lineageCapacity:8192,params});
  const sink=new P011ParticleHotPrototypeSink(work),fixtures=[];registerParticleHotRun(work,1,'D1-R3-HOT-SANDBOX');
  const add=(name,seed,action=0,mass=0,high=false,tilt=.2,bed=20)=>{
    const field={cols:128,capacity:512,sourceInjectedH:0,sourceInjectedL:0,sourceEpoch:0};
    for(const k of ['waterTop','bh','bl','mh','ml','suspH','suspL','surface'])field[k]=new Float64Array(128);
    field.waterTop.fill(400);field.bh.fill(bed);field.surface.fill(bed);if(high){field.bh[20]++;field.surface[20]++;}
    const pool=new P011ParticleField(128,nmax);seed(pool,field);
    fixtures.push({name,field,pool:pool.snapshot(),action,mass,tilt,scale:1,failAfterPublish:0});
  };
  const seed=(p,extra={})=>p.allocate({mass:.2,x:20.5,y:80,vx:0,vy:0,phase:0,sourceKind:0,sourceId:1,...extra});
  add('flight',p=>seed(p));
  add('partial-land',(p,f)=>{const slot=seed(p,{mass:1});p.retainAfterDeposit(slot,0,f.bh[20],1e-6);});
  add('cross',p=>seed(p,{x:20.99,vx:120}));
  add('capacity-cross',(p,f)=>{seed(p,{x:20.99,vx:120});f.waterTop[21]=f.bh[21];});
  add('spawn-tail',()=>{},1,.25);
  add('lift-two-refunds',()=>{},4,1,false,.2,.1);
  add('positive-tail',(p,f)=>{const slot=seed(p,{mass:28/60+1e-14});p.retainAfterDeposit(slot,0,f.bh[20],1e-6);},0,0,false,0);
  add('explicit-merge',(p,f)=>{for(let i=0;i<2;i++){const slot=seed(p,{mass:.1*(i+1),x:20.4+i*.2});p.retainAfterDeposit(slot,0,f.bh[20],1e-6);}},6,.1,true);
  add('explicit-degrade',(p,f)=>{const slot=seed(p,{mass:.1});p.retainAfterDeposit(slot,0,f.bh[20],1e-6);},5,.1,true);
  add('full-merge',(p,f)=>{for(let i=0;i<nmax;i++){const slot=seed(p,{mass:.1,sourceId:i<2?1:i+1});p.retainAfterDeposit(slot,0,f.bh[20],1e-6);}},1,.05,true);
  add('full-lift-degrade',(p,f)=>{for(let i=0;i<nmax;i++){const slot=seed(p,{mass:.1,sourceId:i+1});p.retainAfterDeposit(slot,0,f.bh[20],1e-6);}},4,.25,true);
  const field={cols:128,capacity:512,sourceInjectedH:0,sourceInjectedL:0,sourceEpoch:0};
  for(const k of ['waterTop','bh','bl','mh','ml','suspH','suspL','surface'])field[k]=new Float64Array(128);
  const input={runToken:1,substepId:1,attemptToken:1,dt:1/60,effectiveTiltRad:.2,seed:17,paramsRevision:1,inputRevision:1,field};
  hot.warmUpHot(work);return {work,sink,field,input,fixtures};
}
function appendRequestHotSandbox(work,p,fixture){
  if(fixture.action===0)return;const i=p.requestCount++,kind=fixture.action;
  p.kind[i]=kind;p.phase[i]=0;p.sourceKind[i]=kind===4?1:0;p.column[i]=20;p.targetColumn[i]=-1;
  p.particleId[i]=kind===1||kind===4?0:1;p.sourceId[i]=kind===1||kind===4?99:1;
  p.substep[i]=work.substepId;p.ordinal[i]=i+1;p.requested[i]=fixture.mass;p.totalWeight[i]=0;
  p.predictedX[i]=20.5;p.predictedY[i]=fixture.field.bh[20]+work.params.contactEps;
  p.predictedVx[i]=0;p.predictedVy[i]=0;p.mergeStart[i]=0;p.mergeCount[i]=kind===6?2:0;
  if(kind===6){p.lineage[0]=1;p.lineage[1]=2;}
}
function stepHotSandbox(s,fixture,ordinal){
  const w=s.work,f=s.field;let status=w.pool.transferHotState(fixture.pool,true);if(status!==0)return status;
  f.waterTop.set(fixture.field.waterTop);f.bh.set(fixture.field.bh);f.bl.set(fixture.field.bl);f.mh.set(fixture.field.mh);
  f.ml.set(fixture.field.ml);f.suspH.set(fixture.field.suspH);f.suspL.set(fixture.field.suspL);f.surface.set(fixture.field.surface);
  f.sourceInjectedH=0;f.sourceInjectedL=0;f.sourceEpoch=0;
  s.input.substepId=ordinal;s.input.attemptToken=ordinal;s.input.effectiveTiltRad=fixture.tilt;
  s.sink.failAfterPublish=fixture.failAfterPublish;s.sink.budget.scale=fixture.scale;
  status=hot.beginStepHot(w,s.input);if(status!==0)return status;const epoch=w.stepEpoch;
  status=hot.checkpointHot(w,epoch);if(status!==0)return status;
  status=hot.collectParticlePlanInto(w,epoch,w.planA);if(status!==0)return status;
  status=hot.collectParticlePlanInto(w,epoch,w.planB);if(status!==0)return status;
  appendRequestHotSandbox(w,w.planA,fixture);appendRequestHotSandbox(w,w.planB,fixture);
  status=hot.compareParticlePlansHot(w.planA,w.planB);if(status!==0)return status;
  status=hot.applyParticlePlanInto(w,epoch,w.planA,s.sink,w.results);if(status!==0)return status;
  return hot.finishStepHot(w,epoch);
}
module.exports={makeSandbox,stepHotSandbox,appendRequestHotSandbox};
