const fs=require('node:fs'),Module=require('node:module'),path=require('node:path');
const file=path.resolve('test/t413/controller_units.cjs');
let source=fs.readFileSync(file,'utf8').replace('const {CaptureGeometry}=load',"ctl.bindPrepareGateCold(()=> '');\nconst {CaptureGeometry}=load");
const m=new Module(file);m.filename=file;m.paths=Module._nodeModulePaths(path.dirname(file));m._compile(source,file);
