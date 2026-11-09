const fs=require('fs'),Module=require('module');
let source=fs.readFileSync('test/run_t228_demo_units.cjs','utf8');
source=source.replace('const report={revision:',`
test('T409 one native snapshot per callback, no tickSnapshot refresh',()=>{
 const {c,p}=fixture();let reads=0;const read=p.status.bind(p);p.status=()=>{reads++;return read();};
 c.refresh();c.tickSnapshot(1);assert.equal(reads,1);c.tickSnapshot(10000001);assert.equal(reads,1);
});
test('T409 first stop always reaches native; repeated same known closed scope skips',()=>{
 const {c,p}=fixture();p.s.sourceKind=-1;p.s.sourceOpen=false;p.s.sourceClosePending=false;p.s.pageSession='1';c.pointer=-1;c.refresh();
 let closes=0;const close=p.setSource.bind(p);p.setSource=k=>{if(k===-1)closes++;return close(k);};
 c.closedScope='';c.stop('FAULT');c.stop('FAULT');assert.equal(closes,1);
 p.s.runLo++;p.s.sourceKind=0;p.s.sourceOpen=true;c.refresh();c.stop('NEW_RUN');assert(closes>=2);
});
test('T409 down keeps bounded viewport and actual snapshot identity receipt',()=>{
 const {c,p}=fixture();c.up(1);p.s.pageSession='9';p.s.sampleSerial='123';c.refresh();let record;
 p.recordSourceAttempt=raw=>{record=JSON.parse(raw);return 'SOURCE_ATTEMPT_RECORDED';};
 assert.equal(c.down(1,1000,10),false);assert.equal(record.reason,'OUTSIDE_BOX');assert.equal(record.sampleSerial,'123');assert.equal(record.pageSession,'9');assert.equal(record.width,256);
 assert.equal(c.down(1,100,100),true);assert.equal(record.accepted,true);
});
const report={revision:`);
const m=new Module(__filename);m.paths=module.paths;m._compile(source,__filename);
