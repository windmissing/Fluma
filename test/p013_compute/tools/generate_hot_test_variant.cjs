// Build-time only. Copies the ETS dependency tree to an isolated test output
// and changes exactly the merge-pair source, preserving original merge math.
'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=path.resolve('entry/src/main/ets');
const out=path.resolve(process.argv[2]||'.codex/local-test/t141-variant/entry/src/main/ets');
const sha=s=>crypto.createHash('sha256').update(s).digest('hex');
const hotRel='physics/P011ParticleHot.ets',hotPath=path.join(root,hotRel);
const original=fs.readFileSync(hotPath,'utf8');
const eol=original.includes('\r\n')?'\r\n':'\n';
const needle=`  const found=work.mergeIndex.find(p,work.params.mergeHeightRadius);\n  if(!found)return P011_HOT_INVALID_PATCH_STATE;\n  const a=work.mergeIndex.selectedA,b=work.mergeIndex.selectedB;`.replace(/\n/g,eol);
if(original.split(needle).length!==2)throw new Error('HOT_SELECTOR_WHITELIST_MISMATCH');
const replacement=`  const gpuPair=selectT141GpuPair(p,work.params.mergeHeightRadius,work);\n  if(gpuPair.status==='ERROR')return P011_HOT_AUDIT_FAILED;\n  if(gpuPair.status==='NO_PAIR')return P011_HOT_INVALID_PATCH_STATE;\n  const a=gpuPair.slotA,b=gpuPair.slotB;`.replace(/\n/g,eol);
let variant=original.replace(needle,replacement);
const importLine=`import { selectT141GpuPair } from './P011GpuPairSelector';${eol}`;
variant=importLine+variant;
// Only exact source prefix and the pinned three-line selector may differ.
if(variant.slice(importLine.length).replace(replacement,needle)!==original)throw new Error('NON_WHITELIST_CHANGE');
function copyTree(src,dst){fs.mkdirSync(dst,{recursive:true});
  for(const item of fs.readdirSync(src,{withFileTypes:true})){
    const a=path.join(src,item.name),b=path.join(dst,item.name);
    if(item.isDirectory())copyTree(a,b);else if(item.isFile())fs.copyFileSync(a,b);
  }}
copyTree(root,out);
fs.writeFileSync(path.join(out,hotRel),variant);
const selector=`// T141 isolated test hook. No CPU fallback on GPU errors.\nimport { P011ParticleSnapshot } from './P011ParticleField';\nexport type T141Pair = { status: 'PAIR'|'NO_PAIR'|'ERROR'; slotA: number; slotB: number; errorCode: string };\nexport interface T141PairBinding { selectPair(p: P011ParticleSnapshot, radius: number, work: Object): T141Pair; }\nlet binding: T141PairBinding | null = null;\nexport function registerT141PairBinding(next: T141PairBinding | null): void { binding=next; }\nexport function selectT141GpuPair(p: P011ParticleSnapshot, radius: number, work: Object): T141Pair {\n  if(binding===null)return {status:'ERROR',slotA:-1,slotB:-1,errorCode:'BINDING_UNAVAILABLE'};\n  const r=binding.selectPair(p,radius,work);\n  if(r.status==='PAIR'&&(r.slotA<0||r.slotB<0||r.slotA>=p.nmax||r.slotB>=p.nmax||\n    p.pState[r.slotA]===0||p.pState[r.slotB]===0||p.pId[r.slotA]>=p.pId[r.slotB]))\n    return {status:'ERROR',slotA:-1,slotB:-1,errorCode:'PROTOCOL_ERROR'};\n  return r;\n}\n`;
fs.writeFileSync(path.join(out,'physics/P011GpuPairSelector.ets'),selector);
const adapter=fs.readFileSync(path.resolve('test/p013_compute/ets/T141NativePairBinding.ets'),'utf8');
fs.writeFileSync(path.join(out,'physics/T141NativePairBinding.ets'),adapter);
const manifest={revision:'T141_ISOLATED_HOT_VARIANT_V1',original:hotRel,
  originalSha256:sha(original),generatedSha256:sha(variant),selectorSha256:sha(selector),adapterSha256:sha(adapter),
  replacementCount:1,normalizedDiff:'one import + pinned mergeIndex.find/selectedA/selectedB three-line replacement',
  generatedRoot:out,productionTouched:false};
fs.writeFileSync(path.join(out,'../../../variant-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(`T141 variant generated ${manifest.originalSha256} -> ${manifest.generatedSha256}`);
