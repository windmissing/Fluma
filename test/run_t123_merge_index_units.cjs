'use strict';
const fs=require('fs'),a=require('assert/strict'),ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,f);
const {P011MergeExactIndex:Index}=require('../entry/src/main/ets/physics/P011MergeExactIndex.ets');
function pool(n){return {nmax:n,cols:128,liveCount:0,freeCount:n,nextId:1,
 pMass:new Float64Array(n),pX:new Float64Array(n),pY:new Float64Array(n),pVx:new Float64Array(n),pVy:new Float64Array(n),
 pPhase:new Uint8Array(n),pSource:new Uint8Array(n),pSourceId:new Uint32Array(n),pState:new Uint8Array(n),
 pId:new Uint32Array(n),freeSlots:new Int32Array(n),colParticleMassH:new Float64Array(128),colParticleMassL:new Float64Array(128)};}
function old(p,r){let afterId=0;for(let pass=0;pass<p.nmax;pass++){
 let x=-1;for(let i=0;i<p.nmax;i++)if(p.pState[i]&&p.pId[i]>afterId&&(x<0||p.pId[i]<p.pId[x]))x=i;
 if(x<0)break;let b=-1;for(let j=0;j<p.nmax;j++)if(p.pState[j]&&p.pId[x]<p.pId[j]&&
 p.pPhase[x]===p.pPhase[j]&&p.pSource[x]===p.pSource[j]&&p.pSourceId[x]===p.pSourceId[j]&&
 p.pState[x]===p.pState[j]&&Math.floor(p.pX[x])===Math.floor(p.pX[j])&&
 Math.abs(p.pY[x]-p.pY[j])<=r&&(b<0||p.pId[j]<p.pId[b]))b=j;
 if(b>=0)return [x,b];afterId=p.pId[x];}return [-1,-1];}
let seed=0x1234abcd;function rnd(){seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;}
let cases=0;
function verify(p,r){const index=new Index(p.nmax),expected=old(p,r),found=index.find(p,r);
 a.equal(found,expected[0]>=0);a.deepEqual([index.selectedA,index.selectedB],expected);
 a.ok(index.sortComparisons<=p.nmax*80,`sort ${index.sortComparisons}`);
 a.ok(index.treeVisits<=p.nmax*25,`tree ${index.treeVisits}`);cases++;}
for(const n of [4,16,64,192,512])for(let run=0;run<30;run++){
 const p=pool(n);let ids=[];for(let i=0;i<n;i++)ids.push(i+1);ids.sort(()=>rnd()-.5);
 for(let i=0;i<n;i++){if(rnd()<.15)continue;p.pState[i]=1+(rnd()<.12?1:0);p.pId[i]=ids[i];
  p.pPhase[i]=rnd()<.5?0:1;p.pSource[i]=rnd()<.5?0:1;p.pSourceId[i]=Math.floor(rnd()*4);
  p.pX[i]=Math.floor(rnd()*8)+(.1+rnd()*.8);p.pY[i]=Math.floor(rnd()*10)+rnd()*.2;}
 verify(p,run%3===0?0:run%3===1?.25:2);
}
for(const n of [192,512]){const p=pool(n);for(let i=0;i<n;i++){p.pState[i]=1;p.pId[i]=i+1;p.pX[i]=i+.5;p.pY[i]=i;}verify(p,2);}
{
 const p=pool(6);p.pId.set([4294967295,2,10,4,5,7]);p.pState.set([1,1,1,1,2,1]);
 p.pX.set([2.5,2.5,2.5,2.5,2.5,2.5]);p.pY.set([1,1,2,1,1,1]);
 p.pSourceId.set([1,1,1,1,1,1]);verify(p,0);verify(p,1);
 // A prior merge changes the current centroid and removes one slot. Rebuild
 // rather than using a frozen group or Y index.
 p.pY[1]=1.5;p.pState[3]=0;verify(p,.5);
}
console.log(JSON.stringify({suite:'T123 exact merge index differential',cases,failed:0}));
