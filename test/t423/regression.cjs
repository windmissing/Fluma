const fs=require('node:fs'),Module=require('node:module'),path=require('node:path');
const name=process.argv[3],candidate=process.argv[4]!=='legacy';
if(name!=='t381'&&name!=='t400')throw Error('T423_REGRESSION_NAME');
const file=path.resolve('test',name,'units.cjs');
let source=fs.readFileSync(file,'utf8');
if(name==='t381'){
 source=source.replace("loader('entry/src/main/ets')","loader(process.argv[2]+'/entry/src/main/ets')");
 if(candidate)source=source.replaceAll('motionRevision:revision}','motionRevision:revision,cpuTxRoute:"LOCAL_UNDO_V1"}');
}else if(candidate)source=source.replaceAll('new Demo()','new Demo(512,"LOCAL_UNDO_V1")');
const m=new Module(file);m.filename=file;m.paths=Module._nodeModulePaths(path.dirname(file));m._compile(source,file);
