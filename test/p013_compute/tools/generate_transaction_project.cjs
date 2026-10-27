'use strict';
// T159 offline only. Never modifies production. No Canvas scene/fallback.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'../../..'),base=path.join(root,'.codex/local-test');
const out=path.resolve(process.argv[2]||path.join(base,'t159-project'));
if(!out.startsWith(base+path.sep)||out===base)throw Error('OUTPUT_OUTSIDE_ISOLATED_ROOT');
const sha=v=>crypto.createHash('sha256').update(v).digest('hex');
const inputs={},changes=[];
function copy(src,dst){if(!fs.existsSync(src))throw Error('SOURCE_MISSING:'+src);const stat=fs.statSync(src);
 if(stat.isDirectory()){fs.mkdirSync(dst,{recursive:true});for(const n of fs.readdirSync(src))if(!['build','.hvigor','oh_modules'].includes(n))copy(path.join(src,n),path.join(dst,n));}
 else {const b=fs.readFileSync(src);fs.mkdirSync(path.dirname(dst),{recursive:true});fs.writeFileSync(dst,b);inputs[path.relative(root,src).replaceAll('\\','/')]=sha(b);}}
for(const n of ['AppScope','hvigor','hvigorfile.ts','build-profile.json5','oh-package.json5','oh-package-lock.json5','local.properties'])copy(path.join(root,n),path.join(out,n));
for(const n of ['build-profile.json5','hvigorfile.ts','oh-package.json5','src/main'])copy(path.join(root,'entry',n),path.join(out,'entry',n));
// Local package cache, not a dependency install or another checkout.
if(!fs.existsSync(path.join(out,'oh_modules')))fs.symlinkSync(path.join(root,'oh_modules'),path.join(out,'oh_modules'),'junction');
const ets=path.join(out,'entry/src/main/ets'),templates=path.join(root,'test/p013_compute/ets/t159');
const hot='physics/P011ParticleHot.ets',m2='physics/P011ParticleM2Field.ets',field='physics/WaterSandField.ets';
function patch(rel,fn){const p=path.join(ets,rel),original=fs.readFileSync(p,'utf8'),eol=original.includes('\r\n')?'\r\n':'\n';let value=original.replaceAll('\r\n','\n');
 const edits=[];const replace=(a,b)=>{if(value.split(a).length!==2)throw Error('WHITELIST_MISMATCH '+rel+' '+a.slice(0,80));value=value.replace(a,b);edits.push([a,b]);};fn(replace,()=>value);
 let back=value;for(const [a,b] of edits.toReversed())back=back.replace(b,a);if(back!==original.replaceAll('\r\n','\n'))throw Error('NON_WHITELIST_CHANGE');
 value=value.replaceAll('\n',eol);fs.writeFileSync(p,value);changes.push({file:rel,originalSha256:sha(original),generatedSha256:sha(value),edits});}
patch(hot,(r)=>{
 r('// P-011 D1-R3 S0/S1: fixed-capacity HOT workspace and pure dual collect.',"import { t159Select } from './T159Selector';\nimport { t159Mutation } from './T159Metadata';\n// P-011 D1-R3 S0/S1: fixed-capacity HOT workspace and pure dual collect.");
 r('  const found=work.mergeIndex.find(p,work.params.mergeHeightRadius);\n  if(!found)return P011_HOT_INVALID_PATCH_STATE;\n  const a=work.mergeIndex.selectedA,b=work.mergeIndex.selectedB;',
   "  const pair=t159Select(p,work.params.mergeHeightRadius,work);\n  if(pair.status==='ERROR')return P011_HOT_AUDIT_FAILED;\n  if(pair.status==='NO_PAIR')return P011_HOT_INVALID_PATCH_STATE;\n  const a=pair.slotA,b=pair.slotB;");
 r('  to.colParticleMassH.set(from.colParticleMassH); to.colParticleMassL.set(from.colParticleMassL);','  to.colParticleMassH.set(from.colParticleMassH); to.colParticleMassL.set(from.colParticleMassL);\n  t159Mutation(to);');
 r('  p.pMass[a] = mass; p.pMass[b] = 0; p.pState[b] = 0; p.liveCount--; p.freeSlots[p.freeCount++] = b;',
   '  p.pMass[a] = mass; p.pMass[b] = 0; p.pState[b] = 0; p.liveCount--; p.freeSlots[p.freeCount++] = b;\n  t159Mutation(p);');
 r('        if (phase === 0) p.colParticleMassH[c] += p.pMass[slot]; else p.colParticleMassL[c] += p.pMass[slot];',
   '        if (phase === 0) p.colParticleMassH[c] += p.pMass[slot]; else p.colParticleMassL[c] += p.pMass[slot];\n        t159Mutation(p);');
 r('  } else return P011_HOT_INVALID_INPUT;\n  return 0;\n}\n\nexport function preparePatchHot',
   '  } else return P011_HOT_INVALID_INPUT;\n  t159Mutation(p);\n  return 0;\n}\n\nexport function preparePatchHot');
});
patch(m2,(r)=>{
 r("import { P011ParticleM2Projection }", "import { t159Role } from './T159Metadata';\nimport { P011ParticleM2Projection }");
 r('    phase: number, sourceKind: number, sourceId: number): number {','    phase: number, sourceKind: number, sourceId: number): number {\n    t159Role(1);');
 r('sourceKind: number = 0): boolean {','sourceKind: number = 0): boolean {\n    t159Role(1);');
 r('    sourceHeavy: number, sourceLight: number, sourceEpoch: number): number {','    sourceHeavy: number, sourceLight: number, sourceEpoch: number): number {\n    t159Role(2);');
});
patch(field,(r)=>{
 r("import { P011TiltPreviewTrace,", "import { t159Role } from './T159Metadata';\nimport { P011TiltPreviewTrace,");
 r('      const status = this.particleBinding.apply(this.totalMotionBudgetScale,','      t159Role(3);\n      const status = this.particleBinding.apply(this.totalMotionBudgetScale,');
 r('          const particleStatus = this.particleBinding.apply(scale,','          t159Role(4);\n          const particleStatus = this.particleBinding.apply(scale,');
});
for(const name of ['T159Metadata','T159Selector','T159Native'])copy(path.join(templates,name+'.ets'),path.join(ets,'physics',name+'.ets'));
copy(path.join(templates,'T159PhysicsWorker.ets'),path.join(ets,'workers/T159PhysicsWorker.ets'));
copy(path.join(templates,'T159EntryAbility.ets'),path.join(ets,'entryability/EntryAbility.ets'));
copy(path.join(templates,'T159Index.ets'),path.join(ets,'pages/Index.ets'));
fs.writeFileSync(path.join(out,'entry/src/main/resources/base/profile/main_pages.json'),JSON.stringify({src:['pages/Index']}));
const profileFile=path.join(out,'entry/build-profile.json5'),profile=JSON.parse(fs.readFileSync(profileFile,'utf8'));
profile.buildOption.sourceOption.workers.push('./src/main/ets/workers/T159PhysicsWorker.ets');fs.writeFileSync(profileFile,JSON.stringify(profile,null,2));
const compute=path.join(out,'entry/src/main/cpp/p013_compute');fs.mkdirSync(compute,{recursive:true});
for(const n of fs.readdirSync(path.join(root,'test/p013_compute')))if(/^pair_.*\.(h|cpp)$/.test(n))copy(path.join(root,'test/p013_compute',n),path.join(compute,n));
const shaderSha=[];
for(const n of ['pair_build_v1.comp','pair_query_v1.comp','pair_reduce_v1.comp']) {const src=path.join(root,'test/p013_compute/glsl',n);copy(src,path.join(out,'entry/src/main/resources/rawfile/t141/glsl',n));shaderSha.push(sha(fs.readFileSync(src)));}
// Source digest is non-self-referential. Actual so/HAP SHA belongs to the post-build manifest supplied cold.
inputs['test/p013_compute/tools/generate_transaction_project.cjs']=sha(fs.readFileSync(__filename));
const sourceSha=sha(JSON.stringify({revision:'T159_SOURCE_V2',inputs:Object.fromEntries(Object.entries(inputs).sort()),changes}));
fs.writeFileSync(path.join(ets,'physics/T159BuildIdentity.ets'),`export const T159_SOURCE_SHA='${sourceSha}';\nexport const T159_SHADER_SHA: string[]=${JSON.stringify(shaderSha)};\n`);
fs.writeFileSync(path.join(compute,'CMakeLists.txt'),`add_library(p011_gpu_pair_napi SHARED pair_binding_v1.cpp pair_prepare_v1.cpp pair_executor_v1.cpp pair_owner_v1.cpp)\ntarget_compile_features(p011_gpu_pair_napi PRIVATE cxx_std_17)\ntarget_compile_options(p011_gpu_pair_napi PRIVATE -fno-fast-math -ffp-contract=off)\ntarget_compile_definitions(p011_gpu_pair_napi PRIVATE T159_SOURCE_SHA="${sourceSha}")\ntarget_link_libraries(p011_gpu_pair_napi PRIVATE libace_napi.z.so libEGL.so libGLESv3.so)\nset_target_properties(p011_gpu_pair_napi PROPERTIES OUTPUT_NAME p011_gpu_pair_prototype)\n`);
fs.appendFileSync(path.join(out,'entry/src/main/cpp/CMakeLists.txt'),'\n# T159 isolated binding (no production inclusion)\nadd_subdirectory(p013_compute)\n');
fs.writeFileSync(path.join(out,'t159-source-manifest.json'),JSON.stringify({revision:'T159_SOURCE_V2',sourceManifestSha256:sourceSha,inputs,changes,shaderSha,abi:'arm64-v8a',productionTouched:false},null,2)+'\n');
console.log(JSON.stringify({generatedRoot:out,sourceManifestSha256:sourceSha,changes:changes.map(c=>({file:c.file,edits:c.edits.length})),productionTouched:false}));
