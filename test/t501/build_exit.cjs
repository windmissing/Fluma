const fs=require('node:fs'),cp=require('node:child_process'),path=require('node:path');
const project=process.argv[2],route=process.argv[3],out=process.argv[4],seed=process.argv[5];if(!project||!['m1','m2route'].includes(route)||!out||fs.existsSync(out))throw Error('PROJECT_ROUTE_FRESH_OUTPUT');fs.mkdirSync(out,{recursive:true});
if(seed)fs.cpSync(seed,out+'/cache',{recursive:true});
const main=project+'/entry/src/main/cpp',base=main+'/gpu_resident';
let fixture=fs.readFileSync('test/t438/app_host.cpp','utf8').split('namespace {')[0];
fixture=fixture.replace('#include "app_bridge.cpp"',fs.readFileSync('test/t501/egl_faults.inc','utf8')+'\n#include "app_bridge.cpp"');
const wrappers=fs.readFileSync('test/t475/gl_test.cpp','utf8');fixture+='\n'+wrappers.slice(wrappers.indexOf('#define PROC'),wrappers.indexOf('using namespace grs::programcache;'));
fixture+='\n'+fs.readFileSync('test/t485/gl_exports.inc','utf8').split(/\r?\n/).filter(l=>l.startsWith('#define')||/glGetActiveAttrib|glGetProgramInterfaceiv|glGetProgramResourceiv|glGetAttribLocation|glGetFragDataLocation|glGetProgramResourceIndex|glIsProgram/.test(l)).join('\n');
fixture+='\n'+fs.readFileSync('test/t501/exit_host.inc','utf8');fs.writeFileSync(out+'/exit.cpp',fixture);
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(l=>/^set (SRCS|INCS)=/.test(l)).map(l=>l.replace(/entry\/src\/main\/cpp\/gpu_resident\/m1\/[^ ]+\.cpp /g,'').replaceAll('entry/src/main/cpp',main));
const sources=fs.readdirSync(base+'/'+route).filter(n=>n.endsWith('.cpp')).map(n=>base+'/'+route+'/'+n).join(' ');
const command=['@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',...vars,`cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT /I${project}/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident %INCS% ${out}/exit.cpp test/t324/gl_extra.cpp ${base}/foundation.cpp %SRCS% ${sources} /Fe${out}/exit.exe /Fo${out}/`,'exit /b %errorlevel%'].join('\r\n');fs.writeFileSync(out+'/build.cmd',command);cp.execFileSync('cmd.exe',['/c',path.join(out,'build.cmd')],{stdio:'inherit',timeout:300000});
const env={...process.env,PATH:'C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;'+process.env.PATH};
for(let mode=0;mode<=8;mode++){const result=cp.execFileSync(path.resolve(out+'/exit.exe'),[path.resolve(out+'/cache'),String(mode),path.resolve(out+'/exit-'+mode+'.json')],{env,encoding:'utf8',timeout:650000});fs.writeFileSync(out+'/mode-'+mode+'.txt',result);process.stdout.write(result);}
