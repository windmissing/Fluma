// P-011 R9 contract / manager-approved legacy negative control.
'use strict';
const fs=require('fs'), assert=require('assert/strict'), crypto=require('crypto');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
require.extensions['.ets']=(mod,file)=>mod._compile(ts.transpileModule(fs.readFileSync(file,'utf8'),
  {compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText,file);
const {P011ParticleRefundAdapter,P011ParticleRefundReceiptBuffer}=require('../entry/src/main/ets/physics/P011ParticleRefundAdapter.ets');
const {P011GuardLedgerRefund}=require('../entry/src/main/ets/physics/P011MaterialGuard.ets');
const {WaterSandField}=require('../entry/src/main/ets/physics/WaterSandField.ets');
const results=[],negativeControls=[];
function test(name,fn){try{fn();results.push({name,status:'PASS'});}catch(e){results.push({name,status:'FAIL',error:e.stack});}}
function near(a,b){assert.ok(Math.abs(a-b)<=1e-9,`${a} != ${b}`);}
function setup(target=61,amount=0.4,capacity=16){
  const ledger={localUsed:new Float64Array(128),localRemaining:new Float64Array(128),totalUsed:amount,totalRemaining:5-amount};
  ledger.localRemaining.fill(1); ledger.localUsed[60]=amount; ledger.localRemaining[60]-=amount;
  if(target>=0){ledger.localUsed[target]=amount;ledger.localRemaining[target]-=amount;}
  const adapter=new P011ParticleRefundAdapter(ledger,1,capacity), out=new P011ParticleRefundReceiptBuffer();
  const ticket={ticketId:1,requestKey:'r9-1',substepId:1,source:60,target,requested:1,localAllowed:amount,reserved:amount,refunded:0};
  adapter.registerTicket(ticket); return {ledger,adapter,out,ticket};
}
test('R9 pair WAIT: both endpoints refund 0.4, total once, CLOSED deltas zero',()=>{
  const e=setup(); const buffers=[e.out.localColumns,e.out.localReserved,e.out.localRefunded,e.out.netLocalUsed,e.out.netLocalDelta];
  e.adapter.refundParticleTicket(1,0.4,e.out);
  assert.equal(e.out.status,'REFUNDED');assert.equal(e.out.netDeltaValid,false);
  assert.deepEqual(Array.from(e.out.localRefunded),[0.4,0.4]);near(e.out.totalRefunded,0.4);
  near(e.ledger.localRemaining[60],1);near(e.ledger.localRemaining[61],1);near(e.ledger.totalRemaining,5);
  e.adapter.closeParticleTicket(1,0,e.out);
  assert.equal(e.out.status,'CLOSED');assert.equal(e.out.netDeltaValid,true);
  assert.deepEqual(Array.from(e.out.netLocalUsed),[0,0]); assert.deepEqual(Array.from(e.out.netLocalDelta),[0,0]);
  assert.equal(e.out.netTotalUsed,0);assert.equal(e.out.netTotalDelta,0);
  assert.equal(e.out.bottomReserved,0);assert.equal(e.out.bottomRefunded,0);
  assert.deepEqual(buffers,[e.out.localColumns,e.out.localReserved,e.out.localRefunded,e.out.netLocalUsed,e.out.netLocalDelta]);
});
test('R9 single guard partial refund: final net 0.3 is not incorrectly forced to zero',()=>{
  const e=setup(-1);e.adapter.refundParticleTicket(1,0.1,e.out);e.adapter.closeParticleTicket(1,0.3,e.out);
  near(e.out.netLocalUsed[0],0.3);near(e.out.netTotalUsed,0.3);near(e.out.netLocalDelta[0],0);
  near(e.out.netTotalDelta,0);assert.equal(e.out.localCount,1);assert.equal(e.out.localColumns[1],-1);
  assert.equal(e.out.localReserved[1],0);assert.equal(e.out.localRefunded[1],0);
});
test('R9 two legal partial refunds, zero refund, then positive over-refund rejected unchanged',()=>{
  const e=setup();e.adapter.refundParticleTicket(1,0.2,e.out);e.adapter.refundParticleTicket(1,0,e.out);
  e.adapter.refundParticleTicket(1,0.2,e.out); const before=structuredClone({ledger:e.ledger,out:e.out});
  assert.throws(()=>e.adapter.refundParticleTicket(1,Number.EPSILON,e.out),/OVER_REFUND/);
  assert.deepEqual(structuredClone({ledger:e.ledger,out:e.out}),before); e.adapter.closeParticleTicket(1,0,e.out);
});
test('R9 malformed/NaN/negative/unfunded/closed operations have no partial ledger or receipt writes',()=>{
  for(const amount of [-1,NaN,Infinity,0.4000000000000001]) {
    const e=setup(), before=structuredClone({ledger:e.ledger,out:e.out});
    assert.throws(()=>e.adapter.refundParticleTicket(1,amount,e.out));
    assert.deepEqual(structuredClone({ledger:e.ledger,out:e.out}),before);
  }
  for(const corrupt of ['source','target','total']) {
    const e=setup();if(corrupt==='source')e.ledger.localUsed[60]=0.1;
    if(corrupt==='target')e.ledger.localUsed[61]=0.1;if(corrupt==='total')e.ledger.totalUsed=0.1;
    const before=structuredClone({ledger:e.ledger,out:e.out});
    assert.throws(()=>e.adapter.refundParticleTicket(1,0.4,e.out),/LEDGER_MISMATCH/);
    assert.deepEqual(structuredClone({ledger:e.ledger,out:e.out}),before);
  }
  const e=setup(); e.adapter.closeParticleTicket(1,0.4,e.out);
  const before=structuredClone({ledger:e.ledger,out:e.out});
  assert.throws(()=>e.adapter.refundParticleTicket(1,0,e.out),/STALE/);
  assert.throws(()=>e.adapter.closeParticleTicket(1,0.4,e.out),/STALE/);
  assert.deepEqual(structuredClone({ledger:e.ledger,out:e.out}),before);
});
test('R9 short/aliased receipt rejects before ledger changes',()=>{
  for(const mode of ['short','alias','ledger']) {
    const e=setup();if(mode==='short')e.out.localReserved=new Float64Array(1);
    if(mode==='alias')e.out.localRefunded=e.out.localReserved;
    if(mode==='ledger')e.out.netLocalUsed=e.ledger.localUsed.subarray(60,62);
    const before=structuredClone({ledger:e.ledger,out:e.out});
    assert.throws(()=>e.adapter.refundParticleTicket(1,0.1,e.out),/INVALID_RECEIPT_BUFFER/);
    assert.deepEqual(structuredClone({ledger:e.ledger,out:e.out}),before);
  }
});
test('R9 expiry/ID generations and fixed sidecar overflow reject atomically',()=>{
  const e=setup(-1,0.4,2);e.adapter.beginSubstep(2);
  const before=structuredClone({ledger:e.ledger,out:e.out});
  assert.throws(()=>e.adapter.refundParticleTicket(1,0.1,e.out),/STALE/);
  assert.throws(()=>e.adapter.registerTicket({...e.ticket,substepId:2}),/INVALID_SIDECAR/);
  assert.deepEqual(structuredClone({ledger:e.ledger,out:e.out}),before);
  e.adapter.registerTicket({...e.ticket,ticketId:2,substepId:2});
  e.adapter.registerTicket({...e.ticket,ticketId:3,substepId:2});
  assert.throws(()=>e.adapter.registerTicket({...e.ticket,ticketId:4,substepId:2}),/OVERFLOW/);
});
test('R9 close residual rejection preserves sidecar/output and copied ticket metadata',()=>{
  const e=setup(); e.ticket.reserved=999;e.ticket.source=99;
  const before=structuredClone({ledger:e.ledger,out:e.out});
  assert.throws(()=>e.adapter.closeParticleTicket(1,0.3,e.out),/UNBALANCED/);
  assert.throws(()=>e.adapter.refundParticleTicket(999,0.1,e.out),/STALE/);
  assert.deepEqual(structuredClone({ledger:e.ledger,out:e.out}),before);
  e.adapter.refundParticleTicket(1,0.1,e.out);e.adapter.closeParticleTicket(1,0.3,e.out);
  assert.equal(e.out.localColumns[0],60);near(e.out.totalReserved,0.4);
  near(e.out.netLocalDelta[0],0);near(e.out.netTotalDelta,0);
});
test('R9 adapter storage can be swapped; refund/close have no normal-path allocation syntax',()=>{
  const source=fs.readFileSync(require.resolve('../entry/src/main/ets/physics/P011ParticleRefundAdapter.ets'),'utf8');
  const ast=ts.createSourceFile('adapter.ts',source,ts.ScriptTarget.Latest,true);
  const methods=new Set(['beginSubstep','registerTicket','slot','validateOut','refundParticleTicket','closeParticleTicket','fill']);
  const allocations=[];
  function scan(node,inMethod){
    if(ts.isMethodDeclaration(node))inMethod=methods.has(node.name.getText(ast));
    if(inMethod && (ts.isArrayLiteralExpression(node)||ts.isObjectLiteralExpression(node)||ts.isArrowFunction(node)||ts.isFunctionExpression(node)||
      (ts.isNewExpression(node)&&!ts.isThrowStatement(node.parent)))) allocations.push(node.getText(ast));
    ts.forEachChild(node,n=>scan(n,inMethod));
  } scan(ast,false);assert.deepEqual(allocations,[]);
  const e=setup(), refs=[e.ledger.localUsed,e.ledger.localRemaining,e.out.localReserved,e.out.localRefunded,e.out.netLocalUsed,e.out.netLocalDelta];
  // Constructor/method traps supplement the AST audit; assertions run only
  // after traps are removed, so test framework allocation is not counted.
  const names=['Array','Float64Array','Int32Array','Uint8Array','Map','Set'];const saved=names.map(n=>global[n]);
  const oldStringify=JSON.stringify;let problem;
  try {
    for(const name of names)global[name]=new Proxy(global[name],{construct(){throw Error('allocation '+name);}});
    JSON.stringify=()=>{throw Error('hot stringify');};
    for(let step=2;step<=10001;step++) {
      e.adapter.beginSubstep(step); e.ticket.ticketId=step;e.ticket.substepId=step;
      e.ledger.localUsed[60]=0.4;e.ledger.localUsed[61]=0.4;
      e.ledger.localRemaining[60]=0.6;e.ledger.localRemaining[61]=0.6;
      e.ledger.totalUsed=0.4;e.ledger.totalRemaining=4.6;
      e.adapter.registerTicket(e.ticket);e.adapter.refundParticleTicket(step,0.4,e.out);
      e.adapter.closeParticleTicket(step,0,e.out);
    }
  } catch(error){problem=error;} finally {names.forEach((n,i)=>global[n]=saved[i]);JSON.stringify=oldStringify;}
  assert.equal(problem,undefined);assert.deepEqual(refs,[e.ledger.localUsed,e.ledger.localRemaining,e.out.localReserved,e.out.localRefunded,e.out.netLocalUsed,e.out.netLocalDelta]);
  near(e.ledger.totalRemaining,5);assert.equal(e.out.netDeltaValid,true);
});
test('R9 legacy pair negative control retains exact 0.2/0.2 difference; total once',()=>{
  const e=setup(); const field=new WaterSandField(128,512);field.beginSubstepMotionBudget(1/60,false,1);
  field.substepMotionBudgetUsed[60]=0.4;field.substepMotionBudgetUsed[61]=0.4;
  field.substepMotionBudgetRemaining[60]=0.6;field.substepMotionBudgetRemaining[61]=0.6;
  field.totalMotionBudgetUsed=0.4;field.totalMotionBudgetRemaining=4.6;
  e.adapter.refundParticleTicket(1,0.4,e.out);field.materialGuardRefund(0.4,[60,61],false);
  near(field.substepMotionBudgetUsed[60],0.2);near(field.substepMotionBudgetUsed[61],0.2);
  near(e.ledger.localUsed[60],0);near(e.ledger.localUsed[61],0);
  near(field.totalMotionBudgetUsed,0);near(e.ledger.totalUsed,0);
  negativeControls.push({requested:1,reserved:0.4,refund:0.4,adapterNetLocal:[0,0],legacyNetLocal:[0.2,0.2],difference:[0.2,0.2],netTotal:0});
});
test('R9 legacy split and conservative non-redistribution behavior remain pinned',()=>{
  const remaining=new Float64Array([1,1]),used=new Float64Array([0.1,0.4]);
  new P011GuardLedgerRefund(remaining,used).refundLocal([0,1],0.4);
  near(remaining[0],1.1);near(remaining[1],1.2);near(used[0],0);near(used[1],0.2);
  const r=new Float64Array([1,1]),u=new Float64Array([0.3,0.4]);
  new P011GuardLedgerRefund(r,u).refundLocal([0,999],0.4);
  near(r[0],1.2);near(r[1],1);near(u[0],0.1);near(u[1],0.4);
});
test('R9 legacy method bodies are unchanged from b9b3884 (normalized source hashes)',()=>{
  for(const [file,name,expected] of [
    ['P011MaterialGuard.ets','refundLocal','375ab40956931b58112bb2c414dd56ec2296b15a84089788bfac8af6fb1357d2'],
    ['WaterSandField.ets','materialGuardRefund','0a83e2fd1113b5b99e75aa5895c78b24d2ca835565d41be5424da342c2d1db42']]){
    const src=fs.readFileSync(require.resolve('../entry/src/main/ets/physics/'+file),'utf8').replace(/\r\n/g,'\n');
    const ast=ts.createSourceFile(file,src,ts.ScriptTarget.Latest,true,ts.ScriptKind.TS);let found=0;
    function visit(node){if(ts.isMethodDeclaration(node)&&node.name.getText(ast)===name){found++;
      assert.equal(crypto.createHash('sha256').update(node.getText(ast)).digest('hex'),expected);}
      ts.forEachChild(node,visit);}
    visit(ast);assert.equal(found,1);
  }
});
const failed=results.filter(r=>r.status==='FAIL');
console.log(JSON.stringify({suite:'p011_particle_R9_refund',passed:results.length-failed.length,failed:failed.length,results,negativeControls,
  allocationScope:'adapter normal path only; not a whole-M1 zero-allocation claim'},null,2));
if(failed.length)process.exitCode=1;
