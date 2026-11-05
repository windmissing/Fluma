const cp=require('child_process'),path=require('path');
const result=cp.spawnSync(process.execPath,['test/developer-derived-v2/run_t216_all.cjs'],{stdio:'inherit',env:{...process.env,NODE_OPTIONS:'--require='+path.resolve('test/t351/redirect.cjs')}});process.exitCode=result.status;
