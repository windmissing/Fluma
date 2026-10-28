'use strict';
// T191 strict build overlay; V2/V3 entry points and arithmetic remain untouched.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),cp=require('node:child_process');
const root=path.resolve(__dirname,'../../..'),base=path.join(root,'.codex/local-test');
const out=path.resolve(process.argv[2]||''),variant=process.argv[3];
if(!out.startsWith(base+path.sep)||out===base)throw Error('OUTPUT_OUTSIDE_ISOLATED_ROOT');
if(!['STRICT_O0','STRICT_O2'].includes(variant))throw Error('EXPLICIT_STRICT_VARIANT_REQUIRED');
// A fresh directory prevents stale objects, HAPs, or pre-existing overlays.
if(fs.existsSync(out))throw Error('T191_OUTPUT_ALREADY_EXISTS');
cp.execFileSync(process.execPath,[path.join(__dirname,'generate_transaction_project_v3.cjs'),out],{cwd:root,stdio:'pipe'});
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const v3=JSON.parse(fs.readFileSync(path.join(out,'t171-source-manifest.json'))),edits=[],inputs={};
function patch(file,from,to){const p=path.join(out,file),original=fs.readFileSync(p,'utf8'),eol=original.includes('\r\n')?'\r\n':'\n',s=original.replaceAll('\r\n','\n');
 if(s.split(from).length!==2)throw Error('V4_WHITELIST_MISMATCH:'+file);
 const changed=s.replace(from,to),written=changed.replaceAll('\n',eol);
 if(changed.split(to).length!==2||changed.replace(to,from).replaceAll('\n',eol)!==original)throw Error('V4_INVERSE_DIFF:'+file);
 fs.writeFileSync(p,written);edits.push({file,from,to,eol,originalSha256:sha(original),generatedSha256:sha(written)});
}
const ets='entry/src/main/ets/physics/';
patch(ets+'T171Protocol.ets',"import { T159_SOURCE_SHA,T159_SHADER_SHA }", "import { T191_BUILD_VARIANT,T159_SOURCE_SHA,T159_SHADER_SHA }");
patch(ets+'T171Protocol.ets','extends T159Config {fullDump:boolean;}','extends T159Config {fullDump:boolean;buildVariant:string;}');
patch(ets+'T171Protocol.ets',"return 'T171_HIGH_MERGE_V3|fixture:'", "return 'T191_STRICT_CONFIG_V1|variant:'+T191_BUILD_VARIANT+'|fixture:'");
patch(ets+'T171Protocol.ets',"if(c.config.diagnosticOracle!==(c.oracleMode==='ON')", "if(c.config.buildVariant!==T191_BUILD_VARIANT||c.config.diagnosticOracle!==(c.oracleMode==='ON')");
patch(ets+'T165PageLauncher.ets',"import { T159_SOURCE_SHA,T159_SHADER_SHA }", "import { T191_BUILD_VARIANT,T159_SOURCE_SHA,T159_SHADER_SHA }");
patch(ets+'T165PageLauncher.ets','config:{nmax:fixture.nmax,','config:{buildVariant:T191_BUILD_VARIANT,nmax:fixture.nmax,');
patch(ets+'T171Micro.ets','const config:T171Config={nmax:cell.nmax,','const config:T171Config={buildVariant:c.config.buildVariant,nmax:cell.nmax,');
const cmake='entry/src/main/cpp/p013_compute/CMakeLists.txt';
const cmakeModule='test/p013_compute/strict_variant_v1.cmake';
const copied=fs.readFileSync(path.join(root,cmakeModule));fs.writeFileSync(path.join(out,'entry/src/main/cpp/p013_compute/strict_variant_v1.cmake'),copied);inputs[cmakeModule]=sha(copied);
inputs['test/p013_compute/tools/generate_transaction_project_v4.cjs']=sha(fs.readFileSync(__filename));
// Source identity covers the variant and every additive transformation. Avoid
// hashing its own final literal: bind it in a separately invertible last pass.
const identityPayload={revision:'T191_SOURCE_V4',v3Source:v3.sourceManifestSha256,variant,inputs,edits:JSON.parse(JSON.stringify(edits))};
const sourceSha=sha(JSON.stringify(identityPayload));
patch(ets+'T159BuildIdentity.ets',`export const T159_SOURCE_SHA='${v3.sourceManifestSha256}';`,
 `export const T159_SOURCE_SHA='${sourceSha}';\nexport const T191_BUILD_VARIANT:string='${variant}';`);
patch(cmake,`T159_SOURCE_SHA="${v3.sourceManifestSha256}"`,`T159_SOURCE_SHA="${sourceSha}"`);
const old=fs.readFileSync(path.join(out,cmake),'utf8');
patch(cmake,old.replaceAll('\r\n','\n'),old.replaceAll('\r\n','\n')+`\nset(T191_BOUND_VARIANT ${variant})\n`+'include("${CMAKE_CURRENT_LIST_DIR}/strict_variant_v1.cmake")\n');
const manifest={revision:'T191_SOURCE_V4',sourceManifestSha256:sourceSha,identityPayload:JSON.parse(JSON.stringify(identityPayload)),v3SourceManifest:v3,
 variant,edits,inputs,abi:'arm64-v8a',productionTouched:false,arithmeticOverlay:false,instrumentation:'UNCHANGED_T171_V3',
 artifacts:{postBuildManifest:'t191-post-build-manifest.json',signed:'NOT_BUILT'}};
fs.writeFileSync(path.join(out,'t191-source-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify({generatedRoot:out,variant,sourceManifestSha256:sourceSha,overlayEdits:edits.length,productionTouched:false}));
