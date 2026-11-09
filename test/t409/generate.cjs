const cp=require('node:child_process'),path=require('node:path');
cp.execFileSync(process.execPath,[require.resolve('../t404/generate.cjs'),path.resolve(process.argv[2])],{stdio:'inherit'});
