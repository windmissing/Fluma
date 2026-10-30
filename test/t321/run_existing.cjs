// Execute existing checks unchanged, redirect only their generated evidence.
'use strict';const fs=require('fs'),path=require('path'),Module=require('module');
const file=path.resolve(process.argv[2]),report=path.resolve(process.argv[3]);fs.mkdirSync(report,{recursive:true});
let source=fs.readFileSync(file,'utf8');source=source.replaceAll('test/reports/t226',report.replaceAll('\\','/')).replaceAll('test/reports/t309',report.replaceAll('\\','/'));
// Original test/t309 scripts read their project from argv[2].
process.argv[2]=process.argv[4];
const m=new Module(file);m.filename=file;m.paths=Module._nodeModulePaths(path.dirname(file));m._compile(source,file);
