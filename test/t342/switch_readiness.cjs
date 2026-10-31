'use strict';
// T342-3 V1->V2 production-switch readiness checks (no switch performed; the
// switch gate is T343's device conclusion):
// S1 V1 production default unchanged: EmptySandDemo's model is a plain
//    WaterSandField with numericPolicy V1 (no derived owner) — the V1 default
//    entry stays V1.
// S2 V2 explicit entry available: P051DerivedV2Demo page is registered and its
//    facade constructs only with the V2 policy tuple (isolated, EXACT_V2,
//    CPU_ORIGINAL_F64, T216_DRY_WRITERS_V2) — no silent V1 fallback.
// S3 Old GPU path rejects V2 before native configure (BEFORE_CONFIGURE,
//    configure never called) — honest error, no silent V1 downgrade.
// S4 Renderer gate: V2 states require a matching derived seal; V1 states with
//    no policy fields pass as before (backward compatible).
const assert=require('assert/strict'),fs=require('fs'),path=require('path'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText,f);
const R=p=>path.resolve(p);
let passed=0;
// S1: V1 production default
{
  const src=fs.readFileSync(R('entry/src/main/ets/physics/P051EmptySandDemo.ets'),'utf8');
  assert(src.includes('new WaterSandField(128,256)'),'V1 demo model must stay a plain field');
  assert(!src.includes('P051DerivedFacadeV2'),'V1 demo must not construct the V2 facade');
  const {P011PublishedRenderState}=require(R('entry/src/main/ets/physics/P011PublishedRenderState.ets'));
  const st=new P011PublishedRenderState({cols:128,capacity:256,waterTop:new Float64Array(128),bh:new Float64Array(128),bl:new Float64Array(128),mh:new Float64Array(128),ml:new Float64Array(128),suspH:new Float64Array(128),suspL:new Float64Array(128),sandTop:new Float64Array(128)});
  assert.equal(st.numericPolicy,'P011_NUMERIC_V1');
  const {derivedPublicationGateV2}=require(R('entry/src/main/ets/physics/derived/P011DerivedCommitV2.ets'));
  assert.equal(derivedPublicationGateV2(st),true,'V1 default state must keep passing the renderer gate');
  passed++;
}
// S2: V2 explicit entry
{
  const pages=JSON.parse(fs.readFileSync(R('entry/src/main/resources/base/profile/main_pages.json'),'utf8'));
  assert(pages.src.includes('pages/P051DerivedV2Demo'),'V2 explicit demo page registered');
  const {P051DerivedFacadeV2}=require(R('entry/src/main/ets/physics/derived/P051DerivedFacadeV2.ets'));
  assert.throws(()=>new P051DerivedFacadeV2({isolated:false,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:192,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'}),/DERIVED_CONFIG_UNSUPPORTED/);
  assert.throws(()=>new P051DerivedFacadeV2({isolated:true,numericPolicy:'P011_NUMERIC_V1',nmax:192,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'}),/DERIVED_CONFIG_UNSUPPORTED/);
  const f=new P051DerivedFacadeV2({isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:192,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'});
  const slot=f.createPublishedSlotCold();
  assert.equal(f.copyPublishedSceneStateInto(slot),'OK');
  assert.equal(slot.state.numericPolicy,'P011_DRY_CACHE_EXACT_V2');
  const {derivedPublicationGateV2}=require(R('entry/src/main/ets/physics/derived/P011DerivedCommitV2.ets'));
  assert.equal(derivedPublicationGateV2(slot.state),true,'V2 sealed publication passes the gate');
  passed++;
}
// S3: old GPU path rejects V2 before configure (honest error)
{
  const {TxPairBackends:B}=require(R('test/p013_compute/ets/tx/TxPairBackends.ets'));
  let configured=0;
  assert.throws(()=>new B({policy:'TX_EXACT_STRUCTURE_V1',numericPolicy:'P011_DRY_CACHE_EXACT_V2'},{configure(){configured++;}}),/BEFORE_CONFIGURE/);
  assert.equal(configured,0,'native configure must never run for V2 on the old GPU path');
  passed++;
}
// S4: renderer gate rejects a forged V2 state without a matching seal
{
  const {P011PublishedRenderState}=require(R('entry/src/main/ets/physics/P011PublishedRenderState.ets'));
  const {derivedPublicationGateV2}=require(R('entry/src/main/ets/physics/derived/P011DerivedCommitV2.ets'));
  const st=new P011PublishedRenderState({cols:128,capacity:256,waterTop:new Float64Array(128),bh:new Float64Array(128),bl:new Float64Array(128),mh:new Float64Array(128),ml:new Float64Array(128),suspH:new Float64Array(128),suspL:new Float64Array(128),sandTop:new Float64Array(128)});
  st.numericPolicy='P011_DRY_CACHE_EXACT_V2';
  assert.equal(derivedPublicationGateV2(st),false,'V2-claimed state without a seal must be rejected');
  passed++;
}
const report={revision:'T342_V2_SWITCH_READINESS_V1',passed,failed:0,device:'NOT_RUN',switchDecision:'DEFERRED_TO_T343_DEVICE_CONCLUSION',v1ProductionDefault:'UNCHANGED'};
fs.writeFileSync(R('test/reports/t342/switch-readiness.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report));
