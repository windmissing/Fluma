'use strict';
// T342 mainchain verification: exact70 V2 envelope through the DEMO main chain.
// Part 1: full-run strict-domain audit — facade (isolated V2 owner) drives
// inject+tilt for 300 steps at N192/N512; after EVERY step the published scene
// state must satisfy the renderer's strict domain exactly:
//   0<=bh,bl,mh,mL (beds), susp==0 (dry), 0<=top<=waterTop<=capacity
// with NO epsilon and no negative values anywhere (the code100 failure shape
// was waterTop=-3*2^-54 < top=0).
// Part 2: code100 reproduction counterexample — the exact T180 0a6fe16 failure
// snapshot input (column 57 negative denormal waterTop/colParticleMassH) is
// fed through the same renderer strict pack (P013ScenePreparation); under V2
// the main chain can no longer produce that state, and the reconstructed
// inputs the chain CAN produce all pass. The original snapshot's failing
// predicate (mask128 top>water) must be reproducible from the archived input
// as the counterexample control.
const assert=require('assert/strict'),fs=require('fs'),path=require('path'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText,f);
const {P051DerivedFacadeV2:F}=require(path.resolve('entry/src/main/ets/physics/derived/P051DerivedFacadeV2.ets'));
const {P013ScenePreparation:P}=require(path.resolve('entry/src/main/ets/render/P013ScenePreparation.ets'));
const strictCheck=(f,label)=>{
  const s=f.field;
  for(let c=0;c<128;c++){
    const bh=s.bh[c],bl=s.bl[c],mh=s.mh[c],ml=s.ml[c],st=s.sandTopScratch[c],wt=s.waterTop[c],sh=s.suspH[c],sl=s.suspL[c];
    assert(Number.isFinite(bh)&&bh>=0,`${label} col${c} bh=${bh}`);
    assert(Number.isFinite(bl)&&bl>=0,`${label} col${c} bl=${bl}`);
    assert(Number.isFinite(mh)&&mh>=0,`${label} col${c} mh=${mh}`);
    assert(Number.isFinite(ml)&&ml>=0,`${label} col${c} ml=${ml}`);
    assert(sh===0&&sl===0,`${label} col${c} susp not zero dry`);
    const mix=bh+bl,top=(mix+mh)+ml;
    assert(top>=mix&&mix>=bh,`${label} col${c} order bottom`);
    assert(Number.isFinite(wt)&&wt>=top,`${label} col${c} top=${top} > waterTop=${wt} (mask128 shape)`);
    assert(wt<=256,`${label} col${c} waterTop=${wt} > capacity`);
  }
};
(async()=>{
 const rows=[];
 // Part 1: mainchain 300-step strict audit at both tiers
 for(const n of [192,512]){
   const f=new F({isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:n,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'});
   const slot=f.createPublishedSlotCold();
   for(let k=0;k<300;k++){
     if(k<10)assert(f.inject({revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:k+1,inputTimeMs:k+1,x:64.5,y:20,seed:51,requestedH:.3,requestedL:.1}).ok);
     f.setTilt(k<100?0:10);
     const r=f.step(1/60);assert(r.ok,'step '+k+' failed');
     strictCheck(f,'N'+n+' step'+k);
     if([0,123,299].includes(k)){
       assert.equal(f.copyPublishedSceneStateInto(slot),'OK');
       const s=slot.state,c={physicsCommitId:s.physicsCommitId,particleLayerEnabled:true,fieldEpoch:s.stateEpoch,continuityEpoch:1,geometryEpoch:1,frameSerial:k+1,sampleTimeMs:(k+1)*1000/60,frameDeltaMs:1000/60,inputMode:'EXPERIENCE',sourceKind:'gesture',physicalRouteId:'dual-field-particle-v1',runIdentity:s.runId,boxX:0,boxY:0,boxW:320,boxH:480,pixelRatio:1,visualInverted:false,appearanceTier:0,renderSeed:51,configRevision:1,ablation:'off',fillBox:'tight',canonicalEvidence:false,gravityDirX:0,gravityDirY:1,sandTracerEnabled:false};
       const pack=new P();
       const status=pack.pack(s,c);
       assert.equal(status,'OK','renderer strict pack rejected at step '+k+': '+JSON.stringify(pack.material.failurePoint));
       rows.push({n,step:k,status,commit:s.physicsCommitId});
     }
   }
 }
 // Part 2: code100 counterexample control — the T180 snapshot input MUST still
 // fail the renderer strict pack (proving the pack still rejects the archived
 // shape; the fix is in the chain, not in a relaxed renderer), and the V2
 // chain (Part 1) never produces it.
 const snap=JSON.parse(fs.readFileSync(path.resolve('test/t342/code100_input.json'),'utf8'));
 const buildState=()=>{const n=128,cap=256;const mk=()=>new Float64Array(n);
   const st={cols:n,capacity:cap,waterTop:mk(),bh:mk(),bl:mk(),mh:mk(),ml:mk(),suspH:mk(),suspL:mk(),sandTopScratch:mk(),surface:mk(),colParticleMassH:mk(),colParticleMassL:mk(),particleState:null,mediumMode:'DRY_AIR_V1',airDynamicsRevision:'P051_DRY_GRAVITY_V1',birthPlacementRevision:'DRY_POINT_V1'};
   return st;};
 // Reconstruct the failing array shapes from the archived snapshot
 const mkScene=(arrays)=>{
   // WaterSandSnapshot surface consumed by P013FrameAdapter: sandTop (not the
   // field-internal sandTopScratch), waterTop, bh, bl.
   const f={cols:128,capacity:256,physicsCommitId:1,waterTop:Float64Array.from(arrays.waterTop),bh:Float64Array.from(arrays.bh),bl:Float64Array.from(arrays.bl),mh:Float64Array.from(arrays.mh),ml:Float64Array.from(arrays.ml),suspH:Float64Array.from(arrays.suspH),suspL:Float64Array.from(arrays.suspL),sandTop:Float64Array.from(arrays.sandTop),sandTopScratch:Float64Array.from(arrays.sandTop),surface:Float64Array.from(arrays.sandTop),colParticleMassH:Float64Array.from(arrays.colParticleMassH),colParticleMassL:Float64Array.from(arrays.colParticleMassL),particleState:null,mediumMode:'DRY_AIR_V1',airDynamicsRevision:'P051_DRY_GRAVITY_V1',birthPlacementRevision:'DRY_POINT_V1'};
   return f;};
 // The renderer pack consumes P011PublishedRenderState — emulate enough for the
 // strict SOURCE_VALIDATION stage by calling pack on a minimal state carrying
 // the archived arrays.
 const {P011PublishedRenderState}=require(path.resolve('entry/src/main/ets/physics/P011PublishedRenderState.ets'));
 const snapState=new P011PublishedRenderState(mkScene(snap.arrays));
 // The T180 snapshot predates the policy fields: numericPolicy undefined +
 // derivedSeal null is exactly the archived V1 shape the gate accepts.
 snapState.runId='code100-repro';snapState.physicsCommitId=1;snapState.stateEpoch=1;snapState.publicationKind='REPRO';snapState.ledgerCommitKey='k';
 assert.equal(snapState.numericPolicy===undefined||snapState.numericPolicy==='P011_NUMERIC_V1',true,'snapshot must look V1 to the gate');
 const c2={physicsCommitId:1,particleLayerEnabled:false,fieldEpoch:1,continuityEpoch:1,geometryEpoch:1,frameSerial:1,sampleTimeMs:16.7,frameDeltaMs:16.7,inputMode:'EXPERIENCE',sourceKind:'gesture',physicalRouteId:'dual-field-particle-v1',runIdentity:'code100-repro',boxX:0,boxY:0,boxW:320,boxH:480,pixelRatio:1,visualInverted:false,appearanceTier:0,renderSeed:51,configRevision:1,ablation:'off',fillBox:'tight',canonicalEvidence:false,gravityDirX:0,gravityDirY:1,sandTracerEnabled:false};
 const pack2=new P();
 // The production SceneController arms the diagnostic frame before pack; the
 // failure-point capture (column/predicateMask) requires it. Use a minimal stub.
 pack2.setDiagnosticFrame({},0,1,'repro-binding','repro-digest');
 const snapStatus=pack2.pack(snapState,c2);
 // The archived input must be rejected with the same predicate (counterexample
 // control: renderer strictness intact).
 assert(snapStatus!=='OK','archived code100 input unexpectedly PASSED the strict pack — renderer strictness was relaxed somewhere!');
 const fp=pack2.material.failurePoint;
 assert(fp&&fp.column===57&&fp.predicateMask===128,'counterexample predicate mismatch: '+JSON.stringify(fp));
 rows.push({counterexample:'T180-0a6fe16-archived-input',status:snapStatus,column:fp.column,predicateMask:fp.predicateMask,stage:fp.stage});
 const report={revision:'T342_MAINCHAIN_V2_VERIFICATION_V1',device:'NOT_RUN',rows,mainchainStepsAudited:600,counterexampleControl:'REJECTED_AS_ARCHIVED'};
 fs.mkdirSync('test/reports/t342',{recursive:true});
 fs.writeFileSync('test/reports/t342/mainchain-verification.json',JSON.stringify(report,null,2)+'\n');
 console.log(JSON.stringify(report));
})().catch(e=>{console.error(String(e));process.exit(1);});
