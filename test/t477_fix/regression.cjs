const fs = require('node:fs');
const Module = require('node:module');
const project = process.argv[2];
const group = process.argv[3];
if (!project || !['t306','t404'].includes(group)) throw Error('PROJECT_AND_GROUP_REQUIRED');
let source = fs.readFileSync('test/run_t306_timeline_units.cjs','utf8')
  .replaceAll('entry/src/main/ets/',project+'/entry/src/main/ets/')
  .replace('test/reports/t306/units.json','.codex/local-test/t477-fix-'+group+'.json');
if (group === 't404') {
  const extension = fs.readFileSync('test/t404/cold_units.cjs','utf8');
  const start = extension.indexOf("source=source.replace('const report={revision:',");
  const patch = extension.slice(start,extension.indexOf('const m=new Module',start));
  source = new Function('source',patch+'return source;')(source);
}
const test = new Module(__filename);
test.paths = module.paths;
test._compile(source,__filename);
