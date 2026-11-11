const fs = require('node:fs');
const cp = require('node:child_process');
const path = require('node:path');
const project = process.argv[2], out = process.argv[3];
if (!project || !out || fs.existsSync(out)) throw Error('EXPLICIT_PROJECT_AND_FRESH_OUTPUT_REQUIRED');
fs.mkdirSync(out,{recursive:true});
if (process.argv[4]) fs.cpSync(process.argv[4],path.join(out,'cache'),{recursive:true});
const main = project+'/entry/src/main/cpp';
let fixture = fs.readFileSync('test/t438/app_host.cpp','utf8')
  .replace('uint32_t admittedWater=0;', 'uint32_t admittedWater=0;uint64_t observedPhysicalFrames=0;')
  .replace('w[1]=2;','w[1]=3;')
  .replace('std::chrono::seconds(150)','std::chrono::seconds(500)')
  .replace('  if(a.accepted>lastAccepted)stamp+=8400000;else ++stamp;',
    '  if(!a.jobLive||a.accepted>lastAccepted)stamp+=8400000;else ++stamp;')
  .replace('case 5: if(a.ui.settled.lo==admittedWater&&a.ui.flight.lo==0)',
    'case 5: if(a.ui.settled.lo==admittedWater&&a.ui.flight.lo==0&&a.renderCadence.newPhysicalFrames>0)')
  .replace('  if(!a.AllowedLocked())return;',`
  if(a.recovery.phase==recovery467::Phase::WAIT_NEW_SESSION&&a.recovery.session==0)
   assert(a.RecoverySessionLocked(a.recovery.serial,a.epoch,a.transportIdentity.run,1));
  if(a.progress.open)assert(std::string(a.PagePulseLocked(a.progress.session,a.progress.sequence+1,stamp))=="PAGE_PULSE_ACCEPTED");
  if(!a.AllowedLocked())return;`)
  .replace('assert(Compare64(a.identity.run,originalRun)==0);++phase;reset=true;',
    'assert(Compare64(a.identity.run,originalRun)==0);observedPhysicalFrames=a.renderCadence.newPhysicalFrames;'+
    'assert(observedPhysicalFrames>0&&!a.lastGoodControl.empty()&&a.ui.frameBytes>0);'+
    'std::cout<<"T477 native Draw/Swap current frames="<<observedPhysicalFrames<<std::endl;++phase;reset=true;')
  .replace(' assert(app->ui.SetMode(false));',`
 assert(app->ui.SetMode(false));app->nextProgressV2=true;app->invalidate(nullptr,false);
 assert(grs::programcache::Store().Configure("${out}/cache"));`)
  .replace('T438_DEVELOPER_NATIVE_GL_V1','T477_FIX_GENERATED_M2_GL_V1')
  .replace('<<grs::m1::ExperienceCapabilities(app->K,app->Kw)', '<<"{}"');
const wrappers = fs.readFileSync('test/t475/gl_test.cpp','utf8');
fixture += '\n'+wrappers.slice(wrappers.indexOf('#define PROC'),wrappers.indexOf('using namespace grs::programcache;'));
fs.writeFileSync(path.join(out,'app.cpp'),fixture);
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(line=>/^set (SRCS|INCS)=/.test(line));
const route=main+'/gpu_resident/m2route';
const sources=fs.readdirSync(route).filter(name=>name.endsWith('.cpp')).map(name=>route+'/'+name).join(' ');
const command=[
 '@echo off','setlocal',
 'call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'if errorlevel 1 exit /b 1',
 'set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',
 ...vars.map(line=>line.replace(/entry\/src\/main\/cpp\/gpu_resident\/m1\/[^ ]+\.cpp /g,'').replaceAll('entry/src/main/cpp',main)),
 'cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT %INCS% '+
  out+'/app.cpp test/t324/gl_extra.cpp '+main+'/gpu_resident/foundation.cpp %SRCS% '+sources+' /Fe'+out+'/app.exe /Fo'+out+'/',
 'if errorlevel 1 exit /b 1',out.replaceAll('/','\\')+'\\app.exe','exit /b %errorlevel%'
].join('\r\n');
fs.writeFileSync(path.join(out,'build.cmd'),command);
cp.execFileSync('cmd.exe',['/c',path.join(out,'build.cmd')],{stdio:'inherit',timeout:650000});
