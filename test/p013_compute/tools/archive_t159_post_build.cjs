'use strict';
// T159 post-build cold manifest: archive so/HAP/native-compile identity AFTER a
// successful offline build of the generated project. Read-only w.r.t. the
// build outputs; refuses to invent entries when artifacts are missing.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'../../..');
const proj=path.resolve(process.argv[2]||path.join(root,'.codex/local-test/t159-project'));
const shaFile=p=>{const b=fs.readFileSync(p);return {bytes:b.length,sha256:crypto.createHash('sha256').update(b).digest('hex')};};
const soPath=path.join(proj,'entry/build/default/intermediates/cmake/default/obj/arm64-v8a/libp011_gpu_pair_prototype.so');
const hapPath=path.join(proj,'entry/build/default/outputs/default/entry-default-unsigned.hap');
const out={revision:'T159_POST_BUILD_V1',generatedRoot:proj,abi:'arm64-v8a',signed:false};
if(!fs.existsSync(soPath))throw Error('SO_MISSING:offline native build must run first');
out.so=shaFile(soPath);
if(fs.existsSync(hapPath)){out.unsignedHap=shaFile(hapPath);out.signHap='00303116_STORE_PASSWORD_LENGTH_knowledge:SignHap_known_failure_unsigned_only';}
else out.unsignedHap='NOT_PACKAGED';
const cm=path.join(proj,'entry/build/default/intermediates/cmake/default/obj/arm64-v8a/CMakeFiles/p011_gpu_pair_napi.dir');
if(fs.existsSync(cm)){let log='';try{log=fs.readdirSync(cm).join(',');}catch(e){log='READ_FAILED';}out.nativeCompileArtifacts=log;}
const dest=path.join(proj,'t159-post-build-manifest.json');
fs.writeFileSync(dest,JSON.stringify(out,null,2)+'\n');
console.log(JSON.stringify({revision:out.revision,so:out.so.sha256,unsignedHap:out.unsignedHap.sha256||out.unsignedHap,signed:out.signed,dest}));
