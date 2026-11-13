const fs=require('node:fs'),cp=require('node:child_process'),path=require('node:path');
const project=process.argv[2],route=process.argv[3],out=process.argv[4];if(!project||!['m1','m2route'].includes(route)||!out||fs.existsSync(out))throw Error('PROJECT_ROUTE_FRESH_OUTPUT');fs.mkdirSync(out,{recursive:true});
const main=project+'/entry/src/main/cpp',base=main+'/gpu_resident';
let original=fs.readFileSync('test/t467/control_host.cpp','utf8').replaceAll('\r\n','\n');
original=original.replace(/^extern "C" void GL_APIENTRY glGet(?:Shader|Program)InfoLog.*$/gm,'');
original=original.replace('int main(){','int OriginalControls(){');original=original.slice(0,original.lastIndexOf('}'))+'return 0;\n}';
if(route==='m2route')original=original.replaceAll('m1::','m2route::');
let source=original+'\n'+fs.readFileSync('test/t492/controls.inc','utf8');
// Pure control fixtures have no owner loop; retire their real supervisor to
// avoid racing the manually controlled native clock. GL App tests keep it live.
source=source.replaceAll('auto a=std::make_unique<App>(false);','auto a=std::make_unique<App>(false);{std::lock_guard<std::mutex> lock(a->mutex);a->quit=true;a->wake.notify_all();}if(a->supervisor.joinable())a->supervisor.join();a->quit=false;');
const wrappers=fs.readFileSync('test/t475/gl_test.cpp','utf8');source+='\n'+wrappers.slice(wrappers.indexOf('#define PROC'),wrappers.indexOf('using namespace grs::programcache;'));
const extra=fs.readFileSync('test/t485/gl_exports.inc','utf8').split(/\r?\n/).filter(l=>l.startsWith('#define')||/glGetActiveAttrib|glGetProgramInterfaceiv|glGetProgramResourceiv|glGetAttribLocation|glGetFragDataLocation|glGetProgramResourceIndex|glIsProgram/.test(l)).join('\n');source+='\n'+extra;
fs.writeFileSync(out+'/controls.cpp',source);
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(l=>/^set (SRCS|INCS)=/.test(l)).map(l=>l.replace(/entry\/src\/main\/cpp\/gpu_resident\/m1\/[^ ]+\.cpp /g,'').replaceAll('entry/src/main/cpp',main));
const sources=fs.readdirSync(base+'/'+route).filter(n=>n.endsWith('.cpp')).map(n=>base+'/'+route+'/'+n).join(' ');
const command=['@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',...vars,
`cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT /I${project}/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident %INCS% ${out}/controls.cpp test/t324/gl_extra.cpp ${base}/foundation.cpp %SRCS% ${sources} /Fe${out}/controls.exe /Fo${out}/`,'if errorlevel 1 exit /b 1',out.replaceAll('/','\\')+'\\controls.exe '+out].join('\r\n');
fs.writeFileSync(out+'/build.cmd',command);cp.execFileSync('cmd.exe',['/c',path.join(out,'build.cmd')],{stdio:'inherit',timeout:300000});
