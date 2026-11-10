const fs=require('node:fs'),cp=require('node:child_process');
const project=process.argv[2],out='.codex/local-test/t467-host';fs.mkdirSync(out,{recursive:true});
const main=project+'/entry/src/main/cpp';
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(l=>/^set (SRCS|INCS)=/.test(l));
const cmd=['@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',...vars.map(l=>l.replaceAll('entry/src/main/cpp',main)),`cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT %INCS% test/t467/control_host.cpp test/t324/gl_extra.cpp ${main}/gpu_resident/foundation.cpp %SRCS% /Fe${out}/controls.exe /Fo${out}/`,'if errorlevel 1 exit /b 1',out.replaceAll('/','\\')+'\\controls.exe'].join('\r\n');
fs.writeFileSync(out+'/build.cmd',cmd);cp.execFileSync('cmd.exe',['/c',out.replaceAll('/','\\')+'\\build.cmd'],{stdio:'inherit'});
