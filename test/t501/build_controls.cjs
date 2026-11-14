const fs=require('node:fs'),Module=require('node:module');
let source=fs.readFileSync('test/t492/build_controls.cjs','utf8');
source=source.replace("const wrappers=fs.readFileSync",`source=source.replaceAll('App::NewCaptureStorage()','a->NewCaptureStorage()');
source+='\\n'+fs.readFileSync('test/t501/controls.inc','utf8');
source=source.replace('int main(int argc,char** argv){','int PreviousControls(int argc,char** argv){');
const wrappers=fs.readFileSync`);
// The previous main needs an explicit return when called as a normal function.
source=source.replace("let source=original+'\\n'+fs.readFileSync('test/t492/controls.inc','utf8');", "let controlExtra=fs.readFileSync('test/t492/controls.inc','utf8');controlExtra=controlExtra.slice(0,controlExtra.lastIndexOf('}'))+'return 0;\\n}';let source=original+'\\n'+controlExtra;");
const moduleFile=new Module(__filename);moduleFile.paths=module.paths;moduleFile._compile(source,__filename);
