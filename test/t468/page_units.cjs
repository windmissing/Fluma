const fs=require('node:fs'),assert=require('node:assert/strict'),crypto=require('node:crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
function load(text,imports={}){const code=ts.transpileModule(text,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS,esModuleInterop:true}}).outputText;const m={exports:{}};new Function('require','exports','module',code)(n=>{assert(n in imports,n);return imports[n];},m.exports,m);return m.exports;}
async function main(){let passed=0;
for(const values of [['A'.repeat(64),'b'.repeat(64),'DECLARED_PAIR_HOST_RECEIPT_REQUIRED'],[null,null,'DECLARATION_MISSING'],['a'.repeat(63),'b'.repeat(64),'DECLARATION_INVALID'],['a'.repeat(64)+'\n','b'.repeat(64),'DECLARATION_INVALID']]){
 const m=load(fs.readFileSync('test/t468/M2CreatePackage.ets','utf8'));m.initializeM2Package(values[0],values[1]);const d=m.m2PackageDeclaration();assert.equal(d.status,values[2]);m.initializeM2Package('c'.repeat(64),'d'.repeat(64));assert.deepEqual(m.m2PackageDeclaration(),d);++passed;
}
let pieces=[],order=[],failWrite=false;
const fakeFs={OpenMode:{CREATE:1,READ_WRITE:2,TRUNC:4},openSync:()=>({fd:1}),writeSync:(fd,data)=>{if(failWrite)return 0;const b=Buffer.from(data);pieces.push(b);return b.length;},fsyncSync:()=>order.push('sync'),closeSync:()=>order.push('close')};
const fakeCrypto={createMd:()=>{let b;return {update:async({data})=>{order.push('hash');b=Buffer.from(data);},digest:async()=>({data:crypto.createHash('sha256').update(b).digest()})};}};
const {M2CreateColdFiles}=load(fs.readFileSync('test/t468/M2CreateColdFiles.ets','utf8'),{'@ohos.file.fs':fakeFs,'@ohos.security.cryptoFramework':fakeCrypto});
const writer=new M2CreateColdFiles('/cold');let receipt=await writer.saveRaw('a.log','000afffe');assert.deepEqual(Buffer.concat(pieces),Buffer.from([0,10,255,254]));assert.equal(receipt.bytes,4);assert.equal(receipt.sha256,crypto.createHash('sha256').update(Buffer.concat(pieces)).digest('hex'));assert.deepEqual(order,['sync','close','hash']);++passed;
receipt=await writer.saveRaw('empty.log','');assert.equal(receipt.bytes,0);assert.equal(receipt.sha256,crypto.createHash('sha256').update(Buffer.alloc(0)).digest('hex'));++passed;
for(const [name,hex] of [['../bad','00'],['ok.log','0'],['ok.log','zz'],['ok.log','00'.repeat(4097)]]){await assert.rejects(writer.saveRaw(name,hex));++passed;}
failWrite=true;await assert.rejects(writer.saveRaw('short.log','00'));++passed;
console.log(JSON.stringify({schema:'T468_COLD_EXPORT_UNITS_V1',passed,failed:0}));}
main().catch(e=>{console.error(e);process.exitCode=1;});
