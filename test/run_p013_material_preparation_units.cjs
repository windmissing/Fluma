const fs=require('fs'),a=require('assert/strict');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const p=require('../entry/src/main/ets/render/preparation/P013MaterialPreparation.ets');
const ref=JSON.parse(fs.readFileSync('docs/reference-P013-p1-material-recipe-v1.json','utf8'));
const results=[];function test(name,f){try{f();results.push({name,status:'PASS'})}catch(e){results.push({name,status:'FAIL',error:e.stack})}}
const near=(x,y)=>a.ok(Math.abs(x-y)<=1e-12*Math.max(1,Math.abs(x),Math.abs(y)));
function meta(){return Object.assign(new p.P013PreparedMetadata(),{materialBundleId:'fixture',assetGeneration:1,appearanceConfigRevision:1,renderSeed:17,geometryEpoch:1,paletteHash:'fixture-palette',grainHash:'fixture-grain',strataHash:'fixture-strata',assetGeneratorVersion:'fixture-only',effectiveLambdaMaterial:8,effectiveLambdaCanvasPx:8,warpRangeCycles:1});}
const grain=new Uint8Array(786432).fill(128),strata=new Uint8Array(196608).fill(128);for(let i=3;i<grain.length;i+=4)grain[i]=255;for(let i=3;i<strata.length;i+=4)strata[i]=255;
test('six-layer schema exact lengths, deterministic owned bytes, product disabled',()=>{let b=p.p013PrepareBundle(meta(),grain,strata),c=p.p013PrepareBundle(meta(),grain,strata);a.equal(b.contentFingerprint,c.contentFingerprint);a.equal(b.productEnabled,false);a.equal(b.status,'PREPARED_NOT_DEVICE_VALIDATED');grain[0]=17;a.equal(b.grain[0],128);grain[0]=128;});
test('missing or wrong-size asset rejects without fallback synthesis',()=>{a.throws(()=>p.p013PrepareBundle(meta(),null,strata),/MISSING/);a.throws(()=>p.p013PrepareBundle(meta(),grain,strata.subarray(4)),/LENGTH/)});
test('unknown versions, bad layout, identity and nonopaque assets reject',()=>{for(const change of [{T_dataLayoutVersion:1},{colorPipelineVersion:'old'},{strataWidth:256},{assetGeneration:0},{warpRangeCycles:0}])a.throws(()=>p.p013PrepareBundle(Object.assign(meta(),change),grain,strata));let bad=strata.slice();bad[3]=0;a.throws(()=>p.p013PrepareBundle(meta(),grain,bad),/ALPHA/)});
test('published Q table and Q(255) cap',()=>{for(const r of ref.colors.qTableSelected)near(p.p013Q(r.byte),r.Q);a.equal(p.p013Q(255),4095)});
test('published C3 blocks use exact quantized bytes',()=>{for(const r of ref.colors.c3){const byte=Math.floor(255*Math.pow(2.2*(r.index/64)/(1+2.2*(r.index/64)),1/2.2)+.5);a.equal(byte,r.byte);near(p.p013Q(byte),r.Q)}});
test('neutral B/C increments vanish and OFF ignores light',()=>{const base=[145,168,111];let x=p.p013PreparedColor(base,base,base,[128,128,128],128,.2,.3,.1,.8,false),y=p.p013PreparedColor(base,base,base,[128,128,128],128,.2,.3,.1,1.2,false);a.deepEqual(x,y);x.forEach((v,i)=>near(v,p.p013Q(base[i])/(1+p.p013Q(base[i]))))});
test('one common light before ToneMap; final negative clamp only',()=>{let x=p.p013PreparedColor([145,168,111],[0,0,0],[145,168,111],[0,0,0],0,100,100,100,1,true);a.deepEqual(x,[0,0,0]);const q=p.p013Q(145),y=p.p013PreparedColor([145,145,145],[128,128,128],[128,128,128],[128,128,128],128,0,0,0,.8,true);near(y[0],q*.8/(1+q*.8));a.notEqual(y[0],q/(1+q)*.8)});
test('negative coordinates positive-mod wrap and C continuous-warp texel phase',()=>{a.deepEqual(p.p013PreparedTexel(-1,-1,0,0,1,0,false),[255,255]);a.deepEqual(p.p013PreparedTexel(0,-1,0,-.1,1,0,true),[58,255]);a.throws(()=>p.p013PreparedTexel(0,0,0,0,0,0,true))});
test('preparation has no production caller or shader embedding',()=>{for(const f of ['entry/src/main/ets/render/P013RenderController.ets','entry/src/main/ets/pages/SandCanvas.ets','entry/src/main/cpp/CMakeLists.txt'])a.ok(!fs.readFileSync(f,'utf8').includes('P013MaterialPreparation'));a.ok(!fs.readFileSync('entry/src/main/cpp/CMakeLists.txt','utf8').includes('p1_material_functions'))});
test('layout2 packs 1536 floats with P0 row0, Gaussian slope and exact reserved zeros',()=>{
 const {P013MaterialDataPreparation}=require('../entry/src/main/ets/render/preparation/P013MaterialDataPreparation.ets');
 const snapshot={cols:128,capacity:256,sandTop:new Float64Array(128).fill(100),waterTop:new Float64Array(128).fill(200),bh:new Float64Array(128).fill(20),bl:new Float64Array(128).fill(30),mh:new Float64Array(128).fill(3),ml:new Float64Array(128).fill(1)};
 const z=new Float64Array(128),spark=new Float64Array(128).fill(1),prep=new P013MaterialDataPreparation();
 prep.pack(snapshot,128,256,z,z,z,spark);a.equal(prep.data.length,1536);
 for(let c=0;c<128;c++){a.equal(prep.data[c*4],100);near(prep.data[(128+c)*4],0);a.equal(prep.data[(128+c)*4+1],.75);a.equal(prep.data[(256+c)*4+1],1);a.equal(prep.data[(256+c)*4+2],0);a.equal(prep.data[(256+c)*4+3],0)}
 const before=prep.data.slice();spark[0]=NaN;a.throws(()=>prep.pack(snapshot,128,256,z,z,z,spark));a.deepEqual(prep.data,before);
});
test('layout2 matches signed-off flat/ramp/rawImpulse pb32 selections',()=>{
 const {P013MaterialDataPreparation}=require('../entry/src/main/ets/render/preparation/P013MaterialDataPreparation.ets');
 const expand=v=>Float64Array.from(Array.isArray(v)?v:new Array(128).fill(v));
 for(const key of ['flat','ramp','rawImpulse']){const fx=ref.fixtures[key],snapshot={cols:128,capacity:256,sandTop:expand(fx.rawS),waterTop:new Float64Array(128).fill(256),bh:expand(fx.rawBh),bl:expand(fx.bl),mh:expand(fx.mh),ml:expand(fx.ml)},prep=new P013MaterialDataPreparation(),z=new Float64Array(128),g=new Float64Array(128).fill(1);
 prep.pack(snapshot,128,256,z,z,z,g);
 if(fx.selectedExpected)for(const r of fx.selectedExpected)a.equal(prep.data[(128+r.c)*4],r.pb32);
 else for(let c=0;c<128;c++)near(prep.data[(128+c)*4],0);
 }
});
console.log(JSON.stringify({suite:'T24_material_preparation',passed:results.filter(r=>r.status==='PASS').length,failed:results.filter(r=>r.status==='FAIL').length,results},null,2));if(results.some(r=>r.status==='FAIL'))process.exitCode=1;
