// Recover deterministic generated reports after the first child-runner missed
// NODE_OPTIONS. Never restores HEAD; uses the exact pre-T351 source revision.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),Module=require('module'),{loader}=require('./load.cjs'),load=loader(require('./baseline.cjs'));
const files=['allocation.json','owner.json'],before={};for(const name of files){const p='test/reports/t216/'+name;before[name]=fs.readFileSync(p);}
process.env.T351_REPORT_ROOT='test/reports/t351/restored-baseline';require('./redirect.cjs');
const cache={};function execute(name){const file=path.resolve('test/developer-derived-v2/'+name);if(cache[file])return cache[file].exports;const m=new Module(file);cache[file]=m;m.filename=file;m.require=id=>id.startsWith('../../entry/src/main/ets/')?load(id.slice('../../entry/src/main/ets/'.length)):id.startsWith('./')?execute(path.basename(id)):require(id);m._compile(fs.readFileSync(file,'utf8'),file);return m.exports;}
execute('run_t216_owner.cjs');execute('run_t216_allocation.cjs');
// The untouched dirty allocation.txt is an independent pre-task summary.
const allocation=JSON.parse(fs.readFileSync('test/reports/t351/restored-baseline/t216/allocation.json','utf8')),summary=fs.readFileSync('test/reports/t216/allocation.txt','utf8');for(const p of allocation.profiles){if(!summary.includes(`n: ${p.n}, bytes: ${p.uniqueBackingBytes}, views: ${p.actualDerivedViews}, buffers: ${p.uniqueBuffers}`))throw Error('PRE_TASK_SUMMARY_MISMATCH');}
const audit={revision:'T351_REPORT_OUTPUT_ISOLATION_RECOVERY_V1',baseline:'5970fbd',allocationSummaryMatches:true,files:[]};
for(const name of files){const p='test/reports/t216/'+name,recovered=fs.readFileSync('test/reports/t351/restored-baseline/t216/'+name);if(!fs.readFileSync(p).equals(before[name]))throw Error('CONCURRENT_REPORT_CHANGE:'+name);audit.files.push({file:p,accidentalCurrentSHA:crypto.createHash('sha256').update(before[name]).digest('hex'),recoveredSHA:crypto.createHash('sha256').update(recovered).digest('hex'),scope:'DETERMINISTIC_PRE_TASK_SOURCE_REGENERATION_NOT_CLAIMED_INITIAL_BYTE_BACKUP'});fs.writeFileSync('test/reports/t351/report-before-recovery-'+name,before[name]);}
// Restore through original FS writer, bypass the test output redirection.
const rawWrite=require('fs').writeFileSync;delete process.env.T351_REPORT_ROOT;
// The preload captures write once, so expose a narrow unredirected file handle.
for(const name of files){const recovered=fs.readFileSync('test/reports/t351/restored-baseline/t216/'+name),fd=fs.openSync('test/reports/t216/'+name,'w');try{fs.writeSync(fd,recovered);}finally{fs.closeSync(fd);}}
const fd=fs.openSync('test/reports/t351/report-recovery.json','w');try{fs.writeSync(fd,JSON.stringify(audit,null,2));}finally{fs.closeSync(fd);}console.log(JSON.stringify(audit));
