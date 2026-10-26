// T144 independent D-chain replay + full 1536*float32 fixture manifest.
'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const {execFileSync}=require('node:child_process');
const OUT=path.resolve('test/reports/t144'),ROOT=path.resolve('.codex/local-test/t55-fixtures');
const native=path.resolve('.codex/local-test/t80-matrix/t144_color.exe');
const fixtureName='flat_0_1_0_1_0_0_0.bin',fixture=path.join(ROOT,fixtureName);
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
fs.mkdirSync(OUT,{recursive:true});
// Deliberately remove the prior result before building: no stale-output success.
const raw=path.join(OUT,'roundtrip.json');if(fs.existsSync(raw))fs.unlinkSync(raw);
let build='';try{build=execFileSync('cmd',['/c','test\\p013_native\\build_t144.cmd'],{encoding:'utf8',timeout:600000});}
catch(e){fs.writeFileSync(path.join(OUT,'build.txt'),String(e.stdout||'')+String(e.stderr||''));throw e;}
fs.writeFileSync(path.join(OUT,'build.txt'),build);
if(!build.includes('BUILD_OK'))throw new Error('NATIVE_BUILD_NOT_CONFIRMED');
const env={...process.env,PATH:'C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;'+process.env.PATH};
let run='';try{run=execFileSync(native,[raw],{encoding:'utf8',timeout:30000,env});}
catch(e){fs.writeFileSync(path.join(OUT,'run.txt'),String(e.stdout||'')+String(e.stderr||''));throw e;}
fs.writeFileSync(path.join(OUT,'run.txt'),run);
const probe=JSON.parse(fs.readFileSync(raw,'utf8'));
if(probe.status!=='VALID'||probe.samplerUsed!==true||probe.targetEncoding!=='LINEAR'||
  probe.targetComplete!==true||!probe.sourceTextureBound||!probe.linearFboBound||
  Object.values(probe.errors).some(x=>x!=='NONE'))throw new Error('ROUNDTRIP_INVALID');
const eotf=v=>v<=0.04045?v/12.92:Math.pow((v+0.055)/1.055,2.4);
for(let c=0;c<3;c++){
  const expected=Math.round(eotf(probe.triRead[c]/255)*255);
  if(Math.abs(probe.sampled[c]-expected)>1)throw new Error(`SRGB_SAMPLER_DECODE_MISMATCH_${c}`);
}
const bytes=fs.readFileSync(fixture);const elementCount=1536,elementSize=Float32Array.BYTES_PER_ELEMENT;
if(bytes.length!==elementCount*elementSize)throw new Error('FIXTURE_LENGTH_MISMATCH');
const prefixSha=sha(bytes.subarray(0,elementCount));
const oldPrefix='e86423b1369f4b7afe49b218f15188425a109368ec0cbbd1a320672625a15f53';
if(prefixSha!==oldPrefix)throw new Error('V4_PREFIX_IDENTITY_CHANGED');
fs.copyFileSync(fixture,path.join(OUT,fixtureName));
const files=[fixtureName,'roundtrip.json','build.txt','run.txt'];
const manifest={revision:'T144_COLOR_ROUNDTRIP_MANIFEST_V1',probeStatus:probe.status,
  sourceScope:'independent D chain only; prior T126 A/B production MRT raws remain their own evidence',
  fixture:{typedElementCount:elementCount,bytesPerElement:elementSize,byteLength:bytes.length,
    sha256:sha(bytes),oldV4PrefixHash:{sha256:prefixSha,coveredBytes:elementCount,
      status:'REVOKED_AS_FULL_FIXTURE_HASH',reason:'V4 passed element count as byte count'}},
  files:files.map(name=>{const b=fs.readFileSync(path.join(OUT,name));return {name,byteLength:b.length,sha256:sha(b)};}),
  nativeExecutable:{byteLength:fs.statSync(native).size,sha256:sha(fs.readFileSync(native))},
  sourceSha256:sha(fs.readFileSync('test/p013_native/t144_color_roundtrip.cpp'))};
fs.writeFileSync(path.join(OUT,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(`T144 actual sampler roundtrip PASS; fixture ${bytes.length} bytes ${manifest.fixture.sha256}`);
