'use strict';
const assert=require('node:assert/strict'),{analyze}=require('./read.cjs');
function fixture(change=()=>{}, legacy=false) {
 const rows=[];
 const add=(stage,fields={})=>{
  const serial=String(rows.length+1), ns=String(rows.length+900);
  rows.push({stage,serial,ns,tid:'3',epoch:'4',run:'5',surface:'6',root:'1',
    code:0,carry:0,probeStart:ns,probeEnd:ns,probeA:'0',probeB:'0',probeC:'0',probeWriteNs:'1',...fields});
 };
 add(23);
 if(!legacy){
  add(44,{probeStart:'810',probeEnd:'870',probeA:'0',probeB:'10',carry:1});
  add(43,{probeStart:'800',probeEnd:'880',probeA:'0',probeB:'10',carry:1});
  add(45,{code:16});
  add(44,{probeStart:'890',probeEnd:'892',probeA:'10',probeB:'20',carry:2,code:1});
  add(43,{probeStart:'889',probeEnd:'895',probeA:'10',probeB:'20',carry:2});
  add(46,{carry:2});
  add(47,{code:1});
  add(47,{probeEnd:'930',probeB:'1'});
  add(48,{probeStart:'930',probeEnd:'930',probeA:'7',probeB:'1',probeC:'2',code:1});
  [13,15,16,14].forEach(stage=>add(stage));
  add(50,{probeA:'7',probeB:'1',probeC:'2',code:7,carry:1,requestedToken:'full|token',renderedToken:'full|token'});
 }
 const p={rows,stageRevision:legacy?'T324_STAGE42_V1':'T384_STAGE50_V1',count:rows.length,dropped:'0',incomplete:false};
 change(p,add);p.count=p.rows.length;
 return analyze([p]);
}
const base=fixture();assert.equal(base.state,'EVIDENCE');assert.equal(base.batches[0].flushNs,'60');
assert.equal(base.batches[1].final,true);assert.equal(base.polls.length,2);
assert.equal(base.present[0].state,'SAME_TOKEN_EXCHANGE');
assert.equal(fixture(p=>p.rows.at(-1).renderedToken='wrong').present[0].state,'NOT_JUDGABLE');
assert.equal(fixture((p,add)=>add(49,{probeStart:'940',probeEnd:'940'})).present[0].state,'REVOKED');
assert.equal(fixture((p,add)=>add(49,{probeStart:'910',probeEnd:'910'})).accepted[0].gapOrder,'AFTER');
const unsealed=fixture((p,add)=>{p.rows.length=4;add(49);add(41,{code:1});});
assert.equal(unsealed.gapCount,1);assert.equal(unsealed.polls.length,0);assert.equal(unsealed.accepted.length,0);
assert(fixture(p=>p.rows[2].tid='8').faults.includes('FLUSH_BATCH_MISMATCH'));
assert.equal(fixture(p=>p.dropped='1').state,'NOT_JUDGABLE');
assert.equal(fixture(p=>p.rows.splice(1,1)).state,'NOT_JUDGABLE');
assert.equal(fixture(undefined,true).acceptance,'NOT_JUDGABLE');
console.log('T384 cold dictionary: exchange, GAP before/after, unsealed cancel, BUSY, cross-TID, truncation, token mismatch and legacy verified');
