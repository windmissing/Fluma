'use strict';
const fs=require('fs'),a=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P011ExactDyadic:D}=require('../entry/src/main/ets/physics/P011ExactDyadic.ets');
const {P011StrictStateCertifier:C,p011CanonicalSandTop:S}=require('../entry/src/main/ets/physics/P011StateCertificate.ets');
const fixture=require('./reports/t111/p111_dyadic_fixtures.json');
function oracle(v){const b=Buffer.allocUnsafe(8);b.writeDoubleBE(v);const bits=b.readBigUInt64BE();const ex=Number(bits>>52n&2047n),frac=bits&((1n<<52n)-1n);if(ex===2047)return null;const sig=ex?frac+(1n<<52n):frac,e=ex?ex-1075:-1074;return (bits>>63n? -1n:1n)*sig*(1n<<BigInt(e+2148));}
function cmp(x){return x>0n?1:x<0n?-1:0;}
const d=new D();a.equal(d.environmentSupported,true);
let passed=0;
for(const vals of [[0,0,0,0],[24,24+fixture.tan30.value,0,fixture.tan30.value],[5e-324,1e-323,0,5e-324],[-1e300,1e300,.1,1e300],[Number.MAX_VALUE,Number.MAX_VALUE,0,0]]){
 const [l,r,u,h]=vals;const got=d.compareSum(l,r,u,h);a.equal(got.code,'OK');a.equal(got.sign,cmp(oracle(r)-oracle(l)-oracle(u)-oracle(h)));passed++;
}
for(const [x,y,z] of [[Number.MAX_VALUE,Number.MAX_VALUE,0],[5e-324,1e-323,0],[.1,.2,.02],[-3,4,-12],[1,fixture.tan30.value,fixture.tan30NextUp.value]]){
 const got=d.compareProduct(x,y,z);a.equal(got.code,'OK');a.equal(got.sign,cmp(oracle(x)*oracle(y)-oracle(z)*(1n<<2148n)));passed++;
}
for(const [theta,before,omega,dt] of [[0,0,0,0],[1,0,60,1/60],[-1,0,60,1/60],[5e-324,0,0,1],[10,2,5,1]]){
 const got=d.compareRate(theta,before,omega,dt);a.equal(got.code,'OK');
 const diff=oracle(theta)-oracle(before),rate=oracle(omega)*oracle(dt);
 a.equal(got.sign,cmp((diff<0n?-diff:diff)*(1n<<2148n)-rate));passed++;
}
a.equal(d.compareSum(0,NaN,0,0).code,'NON_FINITE');a.equal(d.unsupportedExpression().code,'ARITHMETIC_EXPRESSION_UNSUPPORTED');passed++;
a.equal(S(1,2,3,4),10);
const cert=new C(3),z=new Float64Array(3),h=new Float64Array([24,24+fixture.tan30.value,24]);
let x=cert.verifySandTop(h,z,z,z,h,0,fixture.tan30.value,'FINAL');a.equal(x.reason,'STRICT_EDGE');a.equal(x.firstEdge,0);
x=cert.verifySandTop(z,z,z,z,z,0,fixture.tan30.value,'FINAL');a.equal(x.legal,true);a.equal(x.scope,'FINAL');passed++;
console.log(JSON.stringify({suite:'T112 exact dyadic',passed,failed:0}));
