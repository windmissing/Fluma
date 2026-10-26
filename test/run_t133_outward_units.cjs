'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P011OutwardArithmetic,P011OutwardInterval}=require('../entry/src/main/ets/physics/P011OutwardArithmetic.ets');
const {P011ExactDyadic}=require('../entry/src/main/ets/physics/P011ExactDyadic.ets');
const {P011WriterEnvelope,P011LocalPatchVerifier,P011NoWritePatch}=require('../entry/src/main/ets/physics/P011WriterEnvelope.ets');
const {P011ActualMaterialPatch,P011StrictInputIdentity}=require('../entry/src/main/ets/physics/P011StateCertificate.ets');
const SHIFT=1n<<2148n;
function dyadic(v){const b=Buffer.alloc(8);b.writeDoubleBE(v);const x=b.readBigUInt64BE();
  const sign=x>>63n?-1n:1n,exp=Number((x>>52n)&2047n),frac=x&((1n<<52n)-1n);
  assert.notEqual(exp,2047);return sign*(frac+(exp?1n<<52n:0n))*(1n<<BigInt((exp?exp-1075:-1074)+2148));}
function cmp(n,d,v){const lhs=n*SHIFT,t=dyadic(v)*d;return Math.sign(lhs<t?-1:lhs>t?1:0);}
function check(a,b,op,oa,out){let n,d=SHIFT;
  const an=dyadic(a),bn=dyadic(b);
  if(op==='add')n=an+bn;else if(op==='sub')n=an-bn;
  else if(op==='mul'){n=an*bn;d=SHIFT*SHIFT;}
  else {assert.notEqual(bn,0n);n=an;d=bn;if(d<0n){n=-n;d=-d;}}
  const rn=op==='add'?a+b:op==='sub'?a-b:op==='mul'?a*b:a/b;
  assert.equal(oa[op+'Into'](a,b,out),'OK');
  const s=cmp(n,d,rn);assert.ok(Object.is(out.lo,s<0?oa.nextDown(rn):rn),`${op} ${a} ${b} lo ${out.lo} rn ${rn} s ${s}`);
  assert.ok(Object.is(out.hi,s>0?oa.nextUp(rn):rn),`${op} ${a} ${b} hi ${out.hi} rn ${rn} s ${s}`);
}
const oa=new P011OutwardArithmetic(),out=new P011OutwardInterval();
assert.equal(oa.environmentSupported,true);
for(const [a,b] of [[1,3],[1,-3],[-1,3],[-1,-3],[Number.MIN_VALUE,2],
  [1,Number.EPSILON],[-0,Number.MIN_VALUE],[1e100,1e-100],[0.1,0.2]]){
  for(const op of ['add','sub','mul'])check(a,b,op,oa,out);
  if(b!==0&&Number.isFinite(a/b))check(a,b,'div',oa,out);
}
assert.equal(oa.addInto(1e308,1e308,out),'RANGE_UNSUPPORTED');
assert.equal(oa.divInto(1,0,out),'DIVIDE_BY_ZERO');
assert.ok(Object.is(oa.nextUp(-Number.MIN_VALUE),-0));
assert.equal(oa.nextDown(0),-Number.MIN_VALUE);
function interval(lo,hi){const x=new P011OutwardInterval();x.lo=lo;x.hi=hi;x.valid=true;x.reason='OK';return x;}
assert.equal(oa.intervalMulInto(interval(-1,2),interval(-3,4),out),'OK');
assert.deepEqual([out.lo,out.hi],[-6,8]);
assert.equal(oa.intervalDivInto(interval(1,2),interval(-1,1),out),'DIVIDE_BY_ZERO');
assert.equal(oa.etaForRangeInto(1,1.5,out),'OK');assert.equal(out.hi,2**-53);
assert.equal(oa.etaForRangeInto(Number.MIN_VALUE,Number.MIN_VALUE,out),'OK');assert.equal(out.hi,Number.MIN_VALUE);
const exact=new P011ExactDyadic(),scalars=new Float64Array([1,-1,2,-2,3,-3,4]);
let r=exact.compareAffineBound(1,1,scalars,7);assert.equal(r.code,'OK');assert.equal(r.sign,1);
const saved={code:r.code,sign:r.sign};r=exact.compareAffineBound(0,0,scalars,8);
assert.equal(r.code,'ARITHMETIC_EXPRESSION_UNSUPPORTED');assert.equal(saved.sign,1);
const env=new P011WriterEnvelope(2,1);
assert.equal(env.addBranch(1,0,1,false,false),0);
assert.equal(env.addBranch(1,0,1,false,false),-1);assert.equal(env.reason,'BRANCH_CAPACITY');
assert.equal(env.addNode(1,-1,-1,0,0,0,0,0,0,0,1),0);
assert.equal(env.addNode(1,0,-1,0,0,0,0,0,0,0,1),1);
assert.equal(env.addNode(1,1,-1,0,0,0,0,0,0,0,1),-1);assert.equal(env.reason,'NODE_CAPACITY');
const bh=new Float64Array(8),bl=new Float64Array(8),mh=new Float64Array(8),ml=new Float64Array(8);
bh[5]=2;const surface=new Float64Array(bh),patch=new P011ActualMaterialPatch(2);
patch.addColumn(0,new Float64Array(7),new Float64Array(7));
const id=new P011StrictInputIdentity();id.runId='r';id.attemptId='a';id.writerKernelRevision='test';id.snapshotSerial=1;
const verifier=new P011LocalPatchVerifier(8);
let cert=verifier.verifyLocalPatch(id,1,1,patch,bh,bl,mh,ml,surface,0,1,3,'LOCAL');
assert.equal(cert.legal,true);assert.equal(cert.boundMode,'ENTRY_H_OR_OLD_RESIDUAL');assert.equal(cert.materialOnly,true);
cert=verifier.verifyLocalPatch(id,1,1,patch,bh,bl,mh,ml,surface,0,1,3,'FINAL');
assert.equal(cert.legal,false);assert.equal(cert.firstEdge,4);
const noWrite=new P011NoWritePatch();assert.equal(noWrite.prepare(false),'NO_WRITE');
assert.equal(noWrite.verifyFloatBits(new Float64Array([-0]),new Float64Array([0])),false);
assert.equal(noWrite.prepare(true),'CONTACT_HOLD_UNSUPPORTED');
console.log('T133 outward, affine, interval, envelope, local/final and no-write PASS');
