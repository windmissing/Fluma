'use strict';
// T384 cold evidence: stage 23 remains a bridge marker in every revision.
const fs = require('node:fs');
const U64 = x => typeof x === 'string' && /^(0|[1-9][0-9]*)$/.test(x) && BigInt(x) <= 18446744073709551615n;
function analyze(pages) {
  const rows=pages.flatMap(p=>p.rows), faults=[], batches=[], exchanges=[], polls=[];
  const rev=pages[0]?.stageRevision;
  if(!['T295_STAGE23_V1','T317_STAGE39_V2','T324_STAGE41_V1','T324_STAGE42_V1','T384_STAGE50_V1','T389_STAGE57_V1','T398_STAGE59_V1'].includes(rev)) return {state:'NOT_JUDGABLE',faults:['DICTIONARY']};
  let lastSerial=0n,lastNs=0n;
  for(const p of pages) if(p.stageRevision!==rev || p.incomplete || p.dropped!=='0' || BigInt(p.count)!==BigInt(rows.length)) faults.push('INCOMPLETE_PAGE_SET');
  for(const r of rows) {
    if(!['serial','ns','tid','epoch','run','surface','root'].every(k=>U64(r[k]))) return {state:'NOT_JUDGABLE',faults:['INVALID_IDENTITY']};
    if(BigInt(r.serial)!==lastSerial+1n || BigInt(r.ns)<lastNs) faults.push('SERIAL_OR_NS');
    lastSerial=BigInt(r.serial);lastNs=BigInt(r.ns);
  }
  if(rev!=='T384_STAGE50_V1'&&rev!=='T389_STAGE57_V1'&&rev!=='T398_STAGE59_V1') return {state:'LEGACY',faults,bridgeMarkers:rows.filter(r=>r.stage===23).length,acceptance:'NOT_JUDGABLE'};
  const identity=r=>[r.epoch,r.run,r.surface,r.tid].join('/');
  const root=r=>identity(r)+'/'+r.root;
  const roots=new Map(), gaps=new Map(), pending=new Map();
  const stamp=r=>BigInt(r.probeStart);
  for(const r of rows) {
    if(r.stage<43)continue;
    if(!['probeStart','probeEnd','probeA','probeB','probeC','probeWriteNs'].every(k=>U64(r[k])) ||
       stamp(r)>BigInt(r.probeEnd)){faults.push('BAD_PROBE_FIELDS');continue;}
    const k=root(r), group=identity(r);
    const h=roots.get(k)??{cursor:0n,batch:0,final:false,accept:null};
    roots.set(k,h);
    if(r.stage===44){
      if(pending.has(k))faults.push('MISSING_BATCH_CALL');
      pending.set(k,r);
    } else if(r.stage===43){
      const f=pending.get(k);pending.delete(k);
      if(!f || f.carry!==r.carry || f.probeA!==r.probeA || f.probeB!==r.probeB ||
         stamp(f)<stamp(r) || BigInt(f.probeEnd)>BigInt(r.probeEnd) || h.final ||
         BigInt(r.probeA)!==h.cursor || BigInt(r.probeB)<=h.cursor || r.carry!==h.batch+1){
        faults.push('FLUSH_BATCH_MISMATCH');continue;
      }
      h.batch=r.carry;h.cursor=BigInt(r.probeB);h.final=f.code===1;
      batches.push({root:k,batch:r.carry,final:h.final,from:r.probeA,to:r.probeB,
        beforeFlushNs:String(stamp(f)-stamp(r)),flushNs:String(BigInt(f.probeEnd)-stamp(f)),
        afterFlushNs:String(BigInt(r.probeEnd)-BigInt(f.probeEnd)),
        writerNs:String(BigInt(r.probeWriteNs)+BigInt(f.probeWriteNs))});
    } else if(r.stage===46) {
      if(r.carry===2&&!h.final)faults.push('INFLIGHT_WITHOUT_FINAL');
      h.inflight=true;
    } else if(r.stage===47){
      if(!h.inflight)faults.push('POLL_WITHOUT_INFLIGHT');
      if(r.code!==0 && r.probeA!==r.probeB)faults.push('ACCEPTED_ON_FAILED_POLL');
      if(r.code===0 && BigInt(r.probeB)!==BigInt(r.probeA)+1n)faults.push('POLL_COUNT');
      h.poll=r;polls.push({root:k,code:r.code,before:r.probeA,after:r.probeB});
    } else if(r.stage===48){
      if(!h.poll || h.poll.code!==0 || h.poll.probeEnd!==r.probeStart || h.accept)faults.push('ACCEPT_WITHOUT_POLL');
      h.accept=r;
    } else if(r.stage===49){
      if(!gaps.has(group))gaps.set(group,r);
    } else if(r.stage===50){
      const a=h.accept;
      const marks=rows.filter(m=>root(m)===k&&m.stage>=13&&m.stage<=16&&
        BigInt(m.serial)>BigInt(a?.serial??r.serial)&&BigInt(m.serial)<BigInt(r.serial));
      const complete=!!a && r.code===7 && r.carry===a.code &&
        ['probeA','probeB','probeC'].every(key=>r[key]===a[key]) &&
        !!r.requestedToken && r.requestedToken===r.renderedToken &&
        marks.map(m=>m.stage).join(',')==='13,15,16,14' && marks[2].code===0 && marks[3].code===0;
      exchanges.push({root:k,row:r,complete,accepted:a});
    }
  }
  if(pending.size)faults.push('UNPAIRED_FLUSH');
  const present=exchanges.map(({root:k,row,complete,accepted})=>{
    const gap=gaps.get(identity(row));
    return {root:k,acceptedBeforeGap:accepted&&gap?stamp(accepted)<stamp(gap):null,
      state:gap?'REVOKED':complete?'SAME_TOKEN_EXCHANGE':'NOT_JUDGABLE'};
  });
  return {state:faults.length?'NOT_JUDGABLE':batches.length?'EVIDENCE':'NO_PROBE_ROWS',faults,batches,polls,present,
    gapCount:gaps.size,accepted:[...roots].filter(([,h])=>h.accept).map(([key,h])=>{
      const gap=gaps.get(identity(h.accept));
      return {root:key,gapOrder:gap?(stamp(h.accept)<stamp(gap)?'BEFORE':'AFTER'):'NO_GAP_IN_ARCHIVE'};
    })};
}
module.exports={analyze};
if(require.main===module){
  const pages=process.argv.slice(2).flatMap(p=>fs.readFileSync(p,'utf8').trim().split(/\r?\n/).map(JSON.parse));
  console.log(JSON.stringify(analyze(pages),null,2));
}
