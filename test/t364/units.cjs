const assert=require('node:assert/strict');
const fs=require('node:fs'),crypto=require('node:crypto'),cp=require('node:child_process'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const load=require('../t351/load.cjs').loader('entry/src/main/ets');
const {singleSandRawGrain,buildSingleSandGrain,SINGLE_GENERATOR_HASH}=load('render/preparation/P013SingleSandAssets');
const original=cp.execFileSync('git',['show','b89022c:entry/src/main/ets/render/preparation/P013SceneNumericAssets.ets'],{encoding:'utf8'});
const a=original.indexOf('  private buildGrainTileFine'),b=original.indexOf('  private buildGrainNoise',a);
let end=original.indexOf('{',b)+1,depth=1;while(depth){if(original[end]==='{')depth++;if(original[end]==='}')depth--;end++;}
assert.equal(crypto.createHash('sha256').update(original.slice(a,end)).digest('hex'),SINGLE_GENERATOR_HASH);
const ref=new Function(ts.transpile('const P012_GRAIN_TILE=256,P012_B_ENV_AMP=.4,P012_B_GRAIN_CLIP=2.5; class Original { '+original.slice(a,end)+' }')+'; return new Original();')();
const tiles=[];
for(const feature of [2,3]){
 const expected=ref.buildGrainTileFine(20261022,feature),actual=singleSandRawGrain(20261022,feature);
 assert(Buffer.from(expected.buffer).equals(Buffer.from(actual.buffer)));
 const tile=buildSingleSandGrain(20261022,feature);
 assert.equal(tile.length,65536);
 for(let i=0;i<tile.length;i++){const n=expected[i]/2.5;assert.equal(tile[i],Math.floor(128+(n<0?128:127)*n+.5));}
 tiles.push(crypto.createHash('sha256').update(tile).digest('hex'));
}
const {P051DerivedFacadeV2:F}=load('physics/derived/P051DerivedFacadeV2');
const {P013SingleSandController:C}=load('render/P013SingleSandController');
const {P013FrameAdapter:D}=load('render/P013FrameAdapter');
const {p013Sha256}=load('render/preparation/P013CanvasAssetExport');
const f=new F({isolated:true,numericPolicy:'P011_DRY_CACHE_EXACT_V2',nmax:16,sourceId:51,backend:'CPU_ORIGINAL_F64',writerRegistry:'T216_DRY_WRITERS_V2'});
const slot=f.createPublishedSlotCold();
let draws=0,stages=0;
const surface={code:0,sizeSource:'EGL_QUERY_SURFACE',surfaceEpoch:1,contextEpoch:1,widthPx:256,heightPx:256};
const fake={
 init:()=>({code:0,sessionId:1}),getState:()=>surface,prepareDualFieldSurface:()=>surface,
 registerSingleSandConfiguration:(cfg,v)=>{assert.equal(v.length,19);assert.equal(cfg.grainHash,tiles[0]);assert.match(cfg.digest,/^[a-f0-9]{64}$/);return {reason:'OK'};},
 stageSingleSandGrain:(desc,tile)=>{assert.equal(p013Sha256(tile),tiles[0]);stages++;return {reason:'OK'};},
 drawSingleSandScene:(p,profile,xy,meta)=>{draws++;assert.equal(p.displayHash,p013Sha256(new Uint8Array(profile.buffer,0,512)));assert.equal(xy.length,32);assert.equal(meta.length,32);return {reason:'OK',currentFrameSucceeded:true};}
};
const renderer=new C(fake);
function draw(serial){
 assert.equal(f.copyPublishedSceneStateInto(slot),'OK');
 const s=slot.state,context={boxX:0,boxY:0,boxW:128,boxH:128,runIdentity:s.runId,fieldEpoch:s.stateEpoch,physicsCommitId:s.physicsCommitId,frameSerial:serial,geometryEpoch:1,continuityEpoch:1};
 assert(slot.drawSingleSand(renderer,context,2)?.currentFrameSucceeded,renderer.failure);
 const legacy=new D();assert(legacy.pack(s.field));for(let i=0;i<128;i++)assert.equal(renderer.preparation.profile[i],legacy.data[i*4]);
 return context;
}
draw(1);assert.equal(renderer.preparation.packet.count,0);
const request={revision:'P051_DRY_AIR_SOURCE_V1',sourceId:51,sequence:1,inputTimeMs:100,x:64.5,y:220,seed:51,requestedH:.3,requestedL:0};
assert(f.inject(request).ok);const context=draw(2);assert(renderer.preparation.packet.count>0);
assert.equal(stages,1);assert.equal(draws,2);
const state=slot.state;
assert.equal(slot.drawSingleSand(renderer,{...context,fieldEpoch:999},2),null);
assert.equal(renderer.failure,'SINGLE_PUBLICATION_MISMATCH');
assert.equal(slot.drawSingleSand(renderer,context,0),null);
state.mediumMode='WET_COLUMN_V1';
assert.equal(slot.drawSingleSand(renderer,context,2),null);assert.equal(renderer.failure,'UNSUPPORTED_MATERIAL_SET');
state.mediumMode='DRY_AIR_V1';state.field.ml[0]=1;
assert.equal(slot.drawSingleSand(renderer,context,2),null);assert.equal(renderer.failure,'UNSUPPORTED_MATERIAL_SET');
state.field.ml[0]=0;
const pool=state.field.particleState,i=pool.pState.findIndex(x=>x!==0);pool.pPhase[i]=1;
assert.equal(slot.drawSingleSand(renderer,context,2),null);assert.equal(renderer.failure,'UNSUPPORTED_MATERIAL_SET');
pool.pPhase[i]=0;pool.pState[i]=2;
assert(slot.drawSingleSand(renderer,{...context,frameSerial:3},2)?.currentFrameSucceeded);
assert(renderer.preparation.slot.particleMeta.includes(512));
const result={generatorHash:SINGLE_GENERATOR_HASH,tiles,assertions:'raw F64 parity, R8, sealed empty/H/CONTACT_WAIT, identity/scale/wet/L rejection, display parity, one cold stage',device:'NOT_RUN'};
fs.writeFileSync('test/reports/t364/ets.json',JSON.stringify(result,null,2)+'\n');
console.log(result);
