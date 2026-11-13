const fs=require('node:fs'),Module=require('node:module');
const project=process.argv[2],group=process.argv[3];if(!['t306','t404'].includes(group))throw Error('GROUP');
let source=fs.readFileSync('test/t477_fix/regression.cjs','utf8');
// Architect explicitly migrated the M1 page to the new independent endpoint;
// retain every original live-reader predicate and adjust only that UI assertion.
const inject=`
source=source.replace("assert(s.includes('this.cold.collect('+r.toLowerCase()))", "assert(s.includes(r==='M1'?'this.cold.collectSnapshot(m1':'this.cold.collect(m0'))");
source=source.replace('.codex/local-test/t477-fix-${group}.json','.codex/local-test/t492-regression-${group}.json');
`;
source=source.replace('const test = new Module(__filename);',inject+'\nconst test = new Module(__filename);');
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
