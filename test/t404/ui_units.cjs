const fs=require('fs'),Module=require('module');
let source=fs.readFileSync('test/run_t228_demo_units.cjs','utf8');
source=source.replace('const report={revision:',`
for(const revision of ['GRS_M1_PROGRESS_CLOCK_V1','GRS_M1_PROGRESS_CLOCK_V2'])test('known progress revision '+revision,()=>{const {c,p}=fixture();p.s.progressClockRevision=revision;c.refresh();assert(c.snapshot);});
test('unknown progress revision explicitly refused',()=>{const {c,p}=fixture();p.s.progressClockRevision='UNKNOWN';c.refresh();assert.equal(c.snapshot,null);assert.equal(c.message,'UNSUPPORTED_PROGRESS_REVISION');});
const report={revision:`);
const m=new Module(__filename);m.paths=module.paths;m._compile(source,__filename);
