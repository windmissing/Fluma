'use strict';
// T171 isolated V3 overlays the unchanged V2 generator. Every overlay is invertible.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),cp=require('node:child_process');
const root=path.resolve(__dirname,'../../..'),base=path.join(root,'.codex/local-test'),out=path.resolve(process.argv[2]||path.join(base,'t171-project'));
if(!out.startsWith(base+path.sep)||out===base)throw Error('OUTPUT_OUTSIDE_ISOLATED_ROOT');
cp.execFileSync(process.execPath,[path.join(__dirname,'generate_transaction_project.cjs'),out],{cwd:root,stdio:'pipe'});
const v2=JSON.parse(fs.readFileSync(path.join(out,'t159-source-manifest.json'))),ets=path.join(out,'entry/src/main/ets'),edits=[],inputs={};
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
function patch(rel,from,to){const file=path.join(ets,rel),original=fs.readFileSync(file,'utf8'),normalized=original.replaceAll('\r\n','\n');
 if(normalized.split(from).length!==2)throw Error('V3_WHITELIST_MISMATCH:'+rel+':'+from.slice(0,60));
 const changed=normalized.replace(from,to),eol=original.includes('\r\n')?'\r\n':'\n',written=changed.replaceAll('\n',eol);if(changed.replace(to,from)!==normalized||changed.replace(to,from).replaceAll('\n',eol)!==original)throw Error('V3_INVERSE_DIFF');
 fs.writeFileSync(file,written);edits.push({file:rel,from,to,eol,originalSha256:sha(original),generatedSha256:sha(written)});
}
function copy(src,rel){const bytes=fs.readFileSync(path.join(root,src));fs.mkdirSync(path.dirname(path.join(ets,rel)),{recursive:true});fs.writeFileSync(path.join(ets,rel),bytes);inputs[src]=sha(bytes);}
for(const [src,dst]of [['T171Evidence','T171Evidence'],['T171Selector','T159Selector'],['T171Metadata','T159Metadata'],['T171Native','T171Native'],['T171Protocol','T171Protocol'],['T171Micro','T171Micro']])copy('test/p013_compute/ets/t171/'+src+'.ets','physics/'+dst+'.ets');
copy('test/p013_compute/ets/t171/T171Worker.ets','workers/T171Worker.ets');
copy('test/p013_compute/ets/t171/T171PageLauncher.ets','physics/T165PageLauncher.ets');
const profile=JSON.parse(fs.readFileSync(path.join(out,'entry/build-profile.json5')));profile.buildOption.sourceOption.workers.push('./src/main/ets/workers/T171Worker.ets');fs.writeFileSync(path.join(out,'entry/build-profile.json5'),JSON.stringify(profile,null,2));
patch('physics/P011ParticleHot.ets',"import { t159Select } from './T159Selector';","import { t159Select } from './T159Selector';\nimport { t171Evidence } from './T171Evidence';");
patch('physics/P011ParticleHot.ets','try{code=finishStepHotBody(work,expectedStepEpoch);return code;}','try{code=finishStepHotBody(work,expectedStepEpoch);if(code===0)t171Evidence.page(work);return code;}');
patch('physics/P011ParticleHot.ets','export function rollbackStepHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number {','export function rollbackStepHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number {\n  t171Evidence.rollbackCalls++;');
patch('physics/P011ParticleHot.ets',"export function auditParticleHot(work: P011ParticleHotWorkspace, source: string='OTHER'): number {","export function auditParticleHot(work: P011ParticleHotWorkspace, source: string='OTHER'): number {\n  t171Evidence.auditCalls++;");
patch('physics/WaterSandField.ets','  private audit(checkDrift: boolean): WaterSandErrorCode | null {',`  // V3 cold archive, invoked only after the original transaction/step returns.
  private t171Hex(buffer:ArrayBuffer,offset:number,length:number):string{
    const bytes=new Uint8Array(buffer,offset,length);let out='';for(let i=0;i<bytes.length;i++)out+=bytes[i].toString(16).padStart(2,'0');return out;
  }
  t171ArchiveState(): string {
    const snapshot=this.snapshot(),p=snapshot.particleState;const raw:string[]=[];
    if(p!==undefined&&p!==null){
      raw.push(this.t171Hex(p.pMass.buffer as ArrayBuffer,p.pMass.byteOffset,p.pMass.byteLength));
      raw.push(this.t171Hex(p.pX.buffer as ArrayBuffer,p.pX.byteOffset,p.pX.byteLength));
      raw.push(this.t171Hex(p.pY.buffer as ArrayBuffer,p.pY.byteOffset,p.pY.byteLength));
      raw.push(this.t171Hex(p.pVx.buffer as ArrayBuffer,p.pVx.byteOffset,p.pVx.byteLength));
      raw.push(this.t171Hex(p.pVy.buffer as ArrayBuffer,p.pVy.byteOffset,p.pVy.byteLength));
      raw.push(this.t171Hex(p.pPhase.buffer as ArrayBuffer,p.pPhase.byteOffset,p.pPhase.byteLength));
      raw.push(this.t171Hex(p.pSource.buffer as ArrayBuffer,p.pSource.byteOffset,p.pSource.byteLength));
      raw.push(this.t171Hex(p.pState.buffer as ArrayBuffer,p.pState.byteOffset,p.pState.byteLength));
      raw.push(this.t171Hex(p.pId.buffer as ArrayBuffer,p.pId.byteOffset,p.pId.byteLength));
      raw.push(this.t171Hex(p.pSourceId.buffer as ArrayBuffer,p.pSourceId.byteOffset,p.pSourceId.byteLength));
      raw.push(this.t171Hex(p.freeSlots.buffer as ArrayBuffer,p.freeSlots.byteOffset,p.freeSlots.byteLength));
      raw.push(this.t171Hex(p.colParticleMassH.buffer as ArrayBuffer,p.colParticleMassH.byteOffset,p.colParticleMassH.byteLength));
      raw.push(this.t171Hex(p.colParticleMassL.buffer as ArrayBuffer,p.colParticleMassL.byteOffset,p.colParticleMassL.byteLength));
    }
    const sourceBudget=new Float64Array([this.sourceInjectedHeavy,this.sourceInjectedLight,this.waterTransactionNet,this.sourceEpoch,this.totalMotionBudgetRemaining,this.totalMotionBudgetUsed,this.totalMotionBudgetClipped]);
    return JSON.stringify({snapshot:snapshot,poolRawBytes:raw,sourceBudgetRaw:this.t171Hex(sourceBudget.buffer as ArrayBuffer,0,sourceBudget.byteLength),
      localBudgetRaw:[this.t171Hex(this.substepMotionBudgetRemaining.buffer as ArrayBuffer,0,this.substepMotionBudgetRemaining.byteLength),this.t171Hex(this.substepMotionBudgetUsed.buffer as ArrayBuffer,0,this.substepMotionBudgetUsed.byteLength)],source:this.getExternalAdmissionEvidence(),drift:this.getDrift(),
      localRemaining:this.substepMotionBudgetRemaining,localUsed:this.substepMotionBudgetUsed,
      totalRemaining:this.totalMotionBudgetRemaining,totalUsed:this.totalMotionBudgetUsed,
      totalClipped:this.totalMotionBudgetClipped,commit:this.renderPhysicsCommitId});
  }
  private audit(checkDrift: boolean): WaterSandErrorCode | null {`);
patch('physics/P011ParticleM2Sink.ets',"import { P011ParticleHotWorkspace,","import { t171Evidence } from './T171Evidence';\nimport { P011ParticleHotWorkspace,");
patch('physics/P011ParticleM2Sink.ets','    this.host.internalParticleRefund(out.localColumns[0], out.localCount === 2 ? out.localColumns[1] : -1, amount);','    this.host.internalParticleRefund(out.localColumns[0], out.localCount === 2 ? out.localColumns[1] : -1, amount);\n    t171Evidence.refund(out);');
patch('physics/WaterSandField.ets',"import { t159Role } from './T159Metadata';","import { t159Role } from './T159Metadata';\nimport { t171Evidence } from './T171Evidence';");
patch('physics/WaterSandField.ets','    for (let attempt = 0; attempt < 8; attempt++) {','    for (let attempt = 0; attempt < 8; attempt++) {\n      t171Evidence.lambdaCalls++;');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {defaultP011ParticleParams}=require(path.join(root,'entry/src/main/ets/physics/P011ParticleM2Field.ets'));
const fixtures=[];
for(const nmax of [192,512])for(const packages of [64,128]){
 const parameters=defaultP011ParticleParams();parameters.nmax=nmax;
 const fixture={parameters,revision:'T171_HIGH_MERGE_FIXTURE_V3',fixtureId:'WET_'+nmax+'_'+packages,nmax,packages,massPackage:.1,column:64,sourceId:17,x:64.5,y:100,seed:13,sequences:[1,2],inputTimes:[10,11],stepsMax:32,synthetic:true};
 const bytes=Buffer.from(JSON.stringify(fixture)+'\n'),fixtureSha256=sha(bytes);fixtures.push({...fixture,fixtureSha256});
 const target=path.join(out,'entry/src/main/resources/rawfile/t171/'+fixture.fixtureId+'.json');fs.mkdirSync(path.dirname(target),{recursive:true});fs.writeFileSync(target,bytes);
}
fs.writeFileSync(path.join(ets,'physics/T171Fixtures.ets'),`import {P011ParticleParams} from './P011ParticleField';\nexport interface T171Fixture {parameters:P011ParticleParams;revision:string;fixtureId:string;nmax:number;packages:number;massPackage:number;column:number;sourceId:number;x:number;y:number;seed:number;sequences:number[];inputTimes:number[];stepsMax:number;synthetic:boolean;fixtureSha256:string;}\nexport const T171_FIXTURES:T171Fixture[]=${JSON.stringify(fixtures)};\n`);
const cmake=path.join(out,'entry/src/main/cpp/p013_compute/CMakeLists.txt');fs.appendFileSync(cmake,'\n# Test-only injection; never in the production target\ntarget_compile_definitions(p011_gpu_pair_napi PRIVATE P011_GPU_TEST_INJECTION=1)\n');
for(let i=0;i<3;i++)fs.appendFileSync(cmake,'target_compile_definitions(p011_gpu_pair_napi PRIVATE T171_SHADER'+i+'_SHA="'+v2.shaderSha[i]+'")\n');
for(const n of ['pair_binding_v1.cpp','pair_executor_v1.cpp','pair_executor_v1.h','pair_prepare_v1.cpp','pair_prepare_v1.h'])inputs['test/p013_compute/'+n]=sha(fs.readFileSync(path.join(root,'test/p013_compute',n)));
inputs['test/p013_compute/tools/generate_transaction_project_v3.cjs']=sha(fs.readFileSync(__filename));
const sourceSha=sha(JSON.stringify({revision:'T171_SOURCE_V3',v2Source:v2.sourceManifestSha256,inputs,edits,fixtures}));
const identity=path.join(ets,'physics/T159BuildIdentity.ets');fs.writeFileSync(identity,fs.readFileSync(identity,'utf8').replace(v2.sourceManifestSha256,sourceSha));
fs.writeFileSync(cmake,fs.readFileSync(cmake,'utf8').replaceAll(v2.sourceManifestSha256,sourceSha));
const manifest={revision:'T171_SOURCE_V3',sourceManifestSha256:sourceSha,v2SourceManifest:v2,inputs,edits,fixtures,abi:'arm64-v8a',testMacro:'P011_GPU_TEST_INJECTION',productionTouched:false};
fs.writeFileSync(path.join(out,'t171-source-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify({generatedRoot:out,sourceManifestSha256:sourceSha,fixtures,overlayEdits:edits.length,productionTouched:false}));
