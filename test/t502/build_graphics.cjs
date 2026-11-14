const fs=require('node:fs'),Module=require('node:module');let source=fs.readFileSync('test/t485/build_host.cjs','utf8');
const patch="if(process.argv[5]==='faults'){const extra=fs.readFileSync('test/t502/graphics.inc','utf8');const marker=' Store().Drain();\\n std::cout<<';const at=source.lastIndexOf(marker);if(at<0)throw Error('T502_FACTORY_ANCHOR');source=source.slice(0,at)+extra+'\\n'+source.slice(at);}";
source=source.replace("if(route==='m2route')source=",patch+"\nif(route==='m2route')source=");
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
