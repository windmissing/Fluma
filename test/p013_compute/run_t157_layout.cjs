'use strict';
const fs=require('fs'),vm=require('vm'),a=require('assert/strict');
const root='test/p013_compute/',source=fs.readFileSync(root+'glsl/pair_reduce_v1.comp','utf8');
const h=Array.from({length:64},(_,i)=>(0x12340000+i*0x101)>>>0);
function vec(...args){const values=args.flat();if(values.length===1)while(values.length<4)values.push(values[0]);a.equal(values.length,4);return new Proxy(values,{get(t,k){if(typeof k==='string'&&/^[xyzw]+$/.test(k)){const x=[...k].map(c=>t['xyzw'.indexOf(c)]);return x.length===1?x[0]:x;}return t[k]}})}
for(const found of [false,true]){
 const ctx={H:{h:Array.from({length:16},(_,i)=>vec(...h.slice(i*4,i*4+4)))},a0:[found?vec(1,11,22,3):vec(0)],a1:[vec(5,7,9,0)],uDispatchCount:12,uvec4:vec,headerWord:i=>h[i]};
 for(const m of source.matchAll(/const uint ([^;]+);/g))for(const n of m[1].split(',')){const [k,v]=n.trim().split('=');ctx[k]=Number(v.replace(/u$/,''));}
 const actual=new Array(32).fill(0);for(const m of source.matchAll(/R\.result\[([^\]]+)\]=([^;]+);/g)){
  const index=vm.runInNewContext(m[1],ctx),expression=m[2].replace(/(0x[\da-f]+|\d+)u\b/gi,'$1');const value=vm.runInNewContext(expression,ctx);actual.splice(index*4,4,...value);
 }
 // Explicit T138 spec order, independent of the JSON schema and C++ helper.
 const expected=[0x47505231,1,found?1:2,0,...(found?[11,22,3,5,7,9]:[0,0,0,0,0,0]),12,0,...[28,29,30,31,14,15,16,17,18,19,48,49,50,51,52,53,54,55].map(i=>h[i]),0x444f4e45,0];
 a.deepEqual(actual,expected);
}
for(const name of ['pair_build_v1.comp','pair_query_v1.comp']){const archived=fs.readFileSync('test/reports/t142/'+name,'utf8');a.equal(fs.readFileSync(root+'glsl/'+name,'utf8').replace(/\r\n/g,'\n'),archived.replace(/\r\n/g,'\n'));}
const old=fs.readFileSync('test/reports/t142/pair_reduce_v1.comp','utf8');for(let i=0;i<6;i++){
 const line=s=>s.split(/\r?\n/).find(x=>x.includes('R.result['+i+']='));a.equal(line(source),line(old));
}
const executor=fs.readFileSync(root+'pair_executor_v1.cpp','utf8');const failure=executor.slice(executor.indexOf('auto fail='),executor.indexOf('auto uploadStart='));a.ok(!failure.includes('glGetError('));
console.log(JSON.stringify({suite:'T157_shader_wire_source',passed:4,failed:0,execution:'SOURCE_EXPRESSION_EVALUATION_NOT_GPU',cases:['PAIR and NO_PAIR all32 words with distinct nonzero SHA tail','build/query match archived whole-word-renamed bytes','reduce R0..5 unchanged','failure handler never rereads consumed GL error']},null,2));
