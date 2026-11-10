const fs=require('node:fs'),cp=require('node:child_process');
const dir=process.argv[2]??'.codex/local-test/t456-regular-final';
const mode=process.argv[3]??'regular';
const main=dir+'/entry/src/main/cpp';
const out='.codex/local-test/t456-host';fs.mkdirSync(out,{recursive:true});
let fixture=fs.readFileSync('test/t438/app_host.cpp','utf8');
fixture=fixture.replace('assert(phase==7);',`assert(phase==7);
 const auto snap=app->bootProbe.Copy();bool ready=false,egl=false,root=false,draw=false;
 for(const auto& a:snap.attempts)for(size_t i=0;i<a.count;++i){
  const std::string stage=a.points[i].stage;ready|=stage=="READY_COMMIT";egl|=stage=="EGL_MAKE_CURRENT_EXIT";
  root|=stage=="ROOT_ACCEPT";draw|=stage=="SWAP_RETURN";
 }
 std::cout<<"observed ready="<<ready<<" egl="<<egl<<" root="<<root<<" swap="<<draw<<std::endl;
 if(!(ready&&egl&&root&&draw)){boot456::Json(std::cerr,snap,"HOST",1);std::cerr<<std::endl;}
 assert(ready&&egl&&root&&draw);std::cout<<"T456 generated bridge EGL/READY/accept/Swap verified"<<std::endl;`);
fixture+=`\nextern "C" void GL_APIENTRY glGetShaderInfoLog(GLuint shader,GLsizei cap,GLsizei* n,GLchar* out){
 auto f=reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(eglGetProcAddress("glGetShaderInfoLog"));assert(f);f(shader,cap,n,out);}
extern "C" void GL_APIENTRY glGetProgramInfoLog(GLuint program,GLsizei cap,GLsizei* n,GLchar* out){
 auto f=reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(eglGetProcAddress("glGetProgramInfoLog"));assert(f);f(program,cap,n,out);}
`;
fs.writeFileSync(out+'/app.cpp',fixture);
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(line=>/^set (SRCS|INCS)=/.test(line));
const script=[
 '@echo off','setlocal','call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'if errorlevel 1 exit /b 1','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',
 `cl /nologo /std:c++17 /EHsc /O2 /Itest/t456 test/t456/probe_host.cpp /Fe${out}/probe.exe /Fo${out}/`,
 'if errorlevel 1 exit /b 1',`${out.replaceAll('/','\\')}\\probe.exe > ${out.replaceAll('/','\\')}\\probe.json`,'if errorlevel 1 exit /b 1',
 ...vars.map(line=>line.replaceAll('entry/src/main/cpp',main)),
 `cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_T456_BOOT ${mode==='experiment'?'/DGRS_PROGRESS_V2_EXPERIMENT':''} %INCS% ${out}/app.cpp test/t324/gl_extra.cpp ${main}/gpu_resident/foundation.cpp %SRCS% /Fe${out}/app.exe /Fo${out}/`,
 'if errorlevel 1 exit /b 1',`${out.replaceAll('/','\\')}\\app.exe`,'exit /b %errorlevel%'
].join('\r\n');
fs.writeFileSync(out+'/build.cmd',script);
cp.execFileSync('cmd.exe',['/c',out.replaceAll('/','\\')+'\\build.cmd'],{stdio:'inherit',timeout:280000});
