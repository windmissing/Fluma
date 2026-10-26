// Replays T140's frozen PRNG, checks independent pool SHA, emits binary64 inputs.
'use strict';
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const official=require('../../../test/reports/t140/p140_gpu_pair_fixtures.json');
const manifest=require('../../../test/reports/t140/p140_manifest.json');
const sha=s=>crypto.createHash('sha256').update(s).digest('hex');
if(sha(JSON.stringify(official))!==manifest.fixturesSha256)throw new Error('T140_MANIFEST_SHA_MISMATCH');
const out=path.resolve(process.argv[2]||'.codex/local-test/t141');fs.mkdirSync(out,{recursive:true});
let seed=0x7137fab;const rnd=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};
for(const n of [192,512]){
  const pool=Array.from({length:n},(_,i)=>({slot:i,id:i+1,active:rnd()>.1,
    x:Math.floor(rnd()*16)+.5,y:Math.floor(rnd()*32)+rnd()*.5,
    group:[rnd()<.5?0:1,rnd()<.5?0:1,Math.floor(rnd()*3),rnd()<.3?1:0]}));
  const digest=sha(JSON.stringify(pool.map(p=>[p.id,p.slot,+p.active,p.x,p.y,p.group])));
  if(digest!==official.pools[n].poolSha)throw new Error(`T140_POOL_SHA_MISMATCH_${n}`);
  const b=Buffer.alloc(4+n*40);b.writeUInt32LE(n,0);
  pool.forEach((p,i)=>{const o=4+i*40;b.writeUInt32LE(p.id,o);b.writeUInt32LE(p.active?1:0,o+4);
    b.writeDoubleLE(p.x,o+8);b.writeDoubleLE(p.y,o+16);
    p.group.forEach((g,k)=>b.writeUInt32LE(g,o+24+k*4));});
  fs.writeFileSync(path.join(out,`t140-${n}.bin`),b);
  console.log(`T140 ${n} verified ${digest}`);
}
