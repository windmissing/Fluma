const fs=require('node:fs'),cp=require('node:child_process');
const project=process.argv[2],out='.codex/local-test/t467-gl';fs.mkdirSync(out,{recursive:true});const main=project+'/entry/src/main/cpp';
let fixture=fs.readFileSync('test/t438/app_host.cpp','utf8');
fixture=fixture.replace('std::chrono::seconds(150)','std::chrono::seconds(500)');
fixture=fixture.replace('  if(a.accepted>lastAccepted)stamp+=8400000;else ++stamp;', '  if(!a.jobLive||a.accepted>lastAccepted)stamp+=8400000;else ++stamp;');
fixture=fixture.replace('  if(!a.AllowedLocked())return;',`  if(a.recovery.phase==recovery467::Phase::WAIT_NEW_SESSION&&a.recovery.session==0){assert(a.RecoverySessionLocked(a.recovery.serial,a.epoch,a.transportIdentity.run,1));}
  if(a.progress.open){const auto pulse=a.PagePulseLocked(a.progress.session,a.progress.sequence+1,stamp);assert(std::string(pulse)=="PAGE_PULSE_ACCEPTED");}
  if(!a.AllowedLocked())return;`);
fixture=fixture.replace(' assert(app->ui.SetMode(false));',' assert(app->ui.SetMode(false));app->nextProgressV2=true;app->invalidate(nullptr,false);');
fixture=fixture.replace('  lastAccepted=a.accepted;', '  lastAccepted=a.accepted;static std::string lastState;static unsigned notes=0;const auto state=std::to_string(phase)+a.status;if(notes<24&&state!=lastState){std::cerr<<\"phase=\"<<phase<<\" \"<<a.status<<std::endl;lastState=state;++notes;}');
fixture=fixture.replace('T438_DEVELOPER_NATIVE_GL_V1','T467_GENERATED_APP_GL_V1');
fixture+='\n'+fs.readFileSync('test/t456/build_host.cjs','utf8').split('fixture+=`')[1].split('`;')[0].replaceAll('\\n','\n');
fs.writeFileSync(out+'/app.cpp',fixture);
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(l=>/^set (SRCS|INCS)=/.test(l));
const script=['@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',...vars.map(l=>l.replaceAll('entry/src/main/cpp',main)),`cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT %INCS% ${out}/app.cpp test/t324/gl_extra.cpp ${main}/gpu_resident/foundation.cpp %SRCS% /Fe${out}/app.exe /Fo${out}/`,'if errorlevel 1 exit /b 1',out.replaceAll('/','\\')+'\\app.exe'].join('\r\n');
fs.writeFileSync(out+'/build.cmd',script);cp.execFileSync('cmd.exe',['/c',out.replaceAll('/','\\')+'\\build.cmd'],{stdio:'inherit',timeout:650000});
