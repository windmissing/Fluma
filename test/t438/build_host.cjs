const fs=require('node:fs'),cp=require('node:child_process');
const main=(process.argv[2]??'.codex/local-test/t438-regular')+'/entry/src/main/cpp';
const out='.codex/local-test/t438-host';fs.mkdirSync(out,{recursive:true});
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(line=>/^set (SRCS|INCS)=/.test(line));
const script=[
 '@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'if errorlevel 1 exit /b 1','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',
 ...vars.map(line=>line.replaceAll('entry/src/main/cpp',main)),
 `cl /nologo /std:c++17 /EHsc /O2 %INCS% test/t438/app_host.cpp test/t324/gl_extra.cpp ${main}/gpu_resident/foundation.cpp %SRCS% /Fe${out}/app.exe /Fo${out}/`,
 'if errorlevel 1 exit /b 1',`${out.replaceAll('/','\\')}\\app.exe`,'exit /b %errorlevel%'
].join('\r\n');
fs.writeFileSync(out+'/build.cmd',script);
cp.execFileSync('cmd.exe',['/c',out.replaceAll('/','\\')+'\\build.cmd'],{stdio:'inherit',timeout:240000});
