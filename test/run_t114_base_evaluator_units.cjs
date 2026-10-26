'use strict';
const fs=require('fs'),a=require('assert/strict'),crypto=require('crypto'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P013BaseByteEvaluator:E,P013BaseStripDescriptor:D}=require('../entry/src/main/ets/render/preparation/P013BaseByteEvaluator.ets');
const e=new E(),profile=new Float32Array(2048);for(let i=0;i<128;i++)profile[(256+i)*4+1]=Math.fround(1.15);
let passed=0;
for(const w of [128,256,1178,3840]){a.equal(e.evaluate(w,0,0,w,false,true,profile),true);a.equal(e.bytes.length,w*12);passed++;}
a.equal(e.evaluate(128,0,0,128,false,true,profile),true);a.equal(e.bytes[(1*128+64)*4+2],172);passed++;
for(let i=0;i<128;i++)profile[(128+i)*4+1]=1;
a.equal(e.evaluate(128,0,0,128,false,false,profile),true);a.equal(e.bytes[(2*128+64)*4],145);passed++;
a.equal(e.evaluate(384,0,16.5,128.7,false,false,profile),true);let count=0;for(let j=0;j<384;j++)count+=e.bytes[j*4+3];a.equal(count,129);passed++;
a.equal(e.evaluate(130,0,0,128,false,false,profile),true);a.equal(e.bytes[128*4+3],0);a.equal(e.bytes[128*4],0);passed++;
const d=new D();e.populateDescriptor(d,profile);a.equal(d.byteLength,130*12);a.equal(d.payloadSha256,crypto.createHash('sha256').update(e.bytes).digest('hex'));passed++;
// Independent byte oracle over native pixel centers, all three bands and
// uploaded Float32 layout. Compare exact bytes (no tolerance).
for(let c=0;c<128;c++){
  profile[(128+c)*4+1]=Math.fround(c/127);
  profile[(256+c)*4+1]=Math.fround(.83+c/127*.31);
}
function oracle(W,vX,bx,bw,mirror,spark){
  const out=new Uint8Array(W*12),H=[145,168,111],L=[216,196,150];
  for(let k=0;k<3;k++)for(let j=0;j<W;j++){
    let x=vX+j+.5-bx,valid=x>=0&&x<bw;if(mirror)x=bw-x;
    const r=Math.min(Math.max(x/(bw/128),0),127),i=Math.min(Math.floor(r),126),t=r-i;
    const f=profile[(128+i)*4+1]*(1-t)+profile[(128+i+1)*4+1]*t;
    const g=spark?profile[(256+i)*4+1]*(1-t)+profile[(256+i+1)*4+1]*t:1;
    for(let c=0;c<3;c++){
      const P=k===0?H[c]:k===1?L[c]:L[c]*(1-f)+H[c]*f;
      out[(k*W+j)*4+c]=valid?Math.floor(Math.min(Math.max(P*g,0),248)+.5):0;
    }
    out[(k*W+j)*4+3]=valid?1:0;
  }
  return out;
}
for(const [W,vX,bx,bw,mirror,spark] of [[128,0,0,128,false,false],[256,12,12.25,127.8,false,true],
 [1178,-7,-3.375,1134.125,true,true],[3840,0,0,3840,true,false]]){
  a.equal(e.evaluate(W,vX,bx,bw,mirror,spark,profile),true);
  a.deepEqual(e.bytes,oracle(W,vX,bx,bw,mirror,spark));passed++;
}
console.log(JSON.stringify({suite:'T114 base F64 evaluator',passed,failed:0}));
