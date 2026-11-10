const fs=require('node:fs'),cp=require('node:child_process');
const project=process.argv[2]??'.codex/local-test/t452-route-final';
const cpp=project+'/entry/src/main/cpp';
const out='.codex/local-test/t452-route-host';fs.mkdirSync(out,{recursive:true});
let fixture='#include <fstream>\n'+fs.readFileSync('test/t438/app_host.cpp','utf8').replace('w[1]=2;','w[1]=3;').replaceAll('T438_DEVELOPER_NATIVE_GL_V1','T452_DEVELOPER_NATIVE_GL_V3');
fixture=fixture.replace('std::chrono::seconds(150)','std::chrono::seconds(450)');
fixture=fixture.replace('    assert(Compare64(a.identity.run,originalRun)==0);++phase;reset=true;',`    assert(Compare64(a.identity.run,originalRun)==0);
    if(!a.m2Trace){if(!a.m2TraceRequested){a.m2TraceRequested=true;a.m2TraceEpoch=a.epoch;}break;}
    assert(a.m2Trace->completion[1]==3&&a.m2Trace->completion[27]==2);
    assert(a.m2Trace->events.size()==size_t(a.m2Trace->completion[53])*64);
    std::ofstream raw(".codex/local-test/t452-route-host/trace.bin",std::ios::binary);
    raw.write(reinterpret_cast<const char*>(a.m2Trace->completion.data()),1024);
    for(const auto* v:{&a.m2Trace->state,&a.m2Trace->events})raw.write(reinterpret_cast<const char*>(v->data()),v->size()*4);
    ++phase;reset=true;`);
fixture=fixture.replace('  <<grs::m1::ExperienceCapabilities(app->K,app->Kw)', '  <<grs::m1::ExperienceCapabilities(app->K,app->Kw)');
fs.writeFileSync(out+'/app.cpp',fixture);
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(line=>/^set (SRCS|INCS)=/.test(line));
const m2=fs.readdirSync(cpp+'/gpu_resident/m2route').filter(n=>n.endsWith('.cpp')).map(n=>cpp+'/gpu_resident/m2route/'+n).join(' ');
const script=[
 '@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'if errorlevel 1 exit /b 1','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',
 ...vars.map(line=>line.replace(/entry\/src\/main\/cpp\/gpu_resident\/m1\/[^ ]+\.cpp /g,'').replaceAll('entry/src/main/cpp',cpp).replaceAll('entry/.cxx',project+'/entry/.cxx')),
 `cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_PROGRESS_V2_EXPERIMENT %INCS% ${out}/app.cpp test/t324/gl_extra.cpp ${cpp}/gpu_resident/foundation.cpp %SRCS% ${m2} /Fe${out}/app.exe /Fo${out}/`,
 'if errorlevel 1 exit /b 1',`${out.replaceAll('/','\\')}\\app.exe`,'exit /b %errorlevel%'
].join('\r\n');
fs.writeFileSync(out+'/build.cmd',script);
cp.execFileSync('cmd.exe',['/c',out.replaceAll('/','\\')+'\\build.cmd'],{stdio:'inherit',timeout:650000});
