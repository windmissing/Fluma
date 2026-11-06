'use strict';
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),Module=require('node:module');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const root=path.join(process.argv[2],'entry/src/main/ets');
require.extensions['.ets']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,esModuleInterop:true,target:ts.ScriptTarget.ES2020}}).outputText,f);
const original=Module.prototype.require;
const sha=data=>crypto.createHash('sha256').update(data).digest('hex');
Module.prototype.require=function(id){
 if(id==='@ohos.file.fs')return {OpenMode:{CREATE:1,READ_WRITE:2,TRUNC:4},openSync:f=>({fd:fs.openSync(f,'w')}),writeSync:(fd,buffer)=>fs.writeSync(fd,Buffer.from(buffer).subarray(0,17)),fsyncSync:fs.fsyncSync,closeSync:f=>fs.closeSync(f.fd)};
 if(id==='@ohos.util')return {TextEncoder:class{encodeInto(t){return new TextEncoder().encode(t);}}};
 if(id==='@ohos.security.cryptoFramework')return {createMd:()=>{const hash=crypto.createHash('sha256');return {update:async o=>hash.update(o.data),digest:async()=>({data:hash.digest()})};}};
 if(id.startsWith('.')&&!path.extname(id)&&fs.existsSync(path.resolve(path.dirname(this.filename),id+'.ets')))return original.call(this,id+'.ets');
 return original.call(this,id);
};
const {parseWindowRoi,saveWindowProbe}=require(path.resolve(root,'render/SingleSandWindowCold.ets'));
async function test(){
 const dir=fs.mkdtempSync('.codex/local-test/t372-archive-');
 const checked=parseWindowRoi('1,2,64,64; 3,4,32,16; 9,8,1,1');
 assert.deepEqual(Array.from(checked),[1,2,64,64,3,4,32,16,9,8,1,1]);
 for(const bad of ['', '0,0,65,1','0,0,1,0','-1,0,1,1','1.5,0,1,1','2147483648,0,1,1','1,2,3,4;1,2,3,4;1,2,3,4;1,2,3,4'])assert.equal(parseWindowRoi(bad),null,bad);
 const source=new Uint8Array([55,0,120,255,17,66]),candidate=source.subarray(1,5),back=Uint8Array.of(9,8,7,6);
 const manifest={captureStatus:'CAPTURED',session:7,frameSerial:11,requestedToken:'token-11',actualToken:'token-11',candidate:{sha256:sha(candidate)},back:{sha256:sha(back)}};
 const receipt=await saveWindowProbe(dir,{manifest,candidate,back});
 const name=receipt.split(' ')[1],bytes=fs.readFileSync(path.join(dir,name)),archive=JSON.parse(bytes);
 assert.deepEqual(archive.probe,manifest);
 for(const [expected,file] of [[candidate,archive.candidate],[back,archive.back]]){
  const data=fs.readFileSync(path.join(dir,file.name));
  assert.deepEqual(data,Buffer.from(expected));assert.equal(sha(data),file.sha256);
 }
 assert.equal(fs.readFileSync(path.join(dir,name.replace('.json','.sha256')),'utf8'),sha(bytes)+'  '+name+'\n');
 const failed={...manifest,frameSerial:12,captureStatus:'CAPTURE_FAILED',candidate:{},back:{}};
 const failReceipt=await saveWindowProbe(dir,{manifest:failed});
 const fail=JSON.parse(fs.readFileSync(path.join(dir,failReceipt.split(' ')[1])));
 assert.equal(fail.probe.captureStatus,'CAPTURE_FAILED');assert.equal(fail.candidate,null);assert.equal(fail.back,null);
 await assert.rejects(()=>saveWindowProbe(dir,{manifest,candidate:back,back}),/WINDOW_HASH_MISMATCH/);
 await assert.rejects(()=>saveWindowProbe(dir,{manifest,candidate}),/WINDOW_INCOMPLETE_CURRENT/);
 await assert.rejects(()=>saveWindowProbe(dir,{manifest:{...manifest,actualToken:'stale'},candidate,back}),/WINDOW_INCOMPLETE_CURRENT/);
 console.log('PASS ROI parser, raw bytes including view offset, SHA sidecar, same-token manifest, failure without pixels and hash rejection');
}
test().catch(e=>{console.error(e);process.exitCode=1;});
