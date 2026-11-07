const fs=require('node:fs'),cp=require('node:child_process');
const out='.codex/local-test/t398-host';
fs.mkdirSync(out,{recursive:true});
const original=fs.readFileSync('test/t336/build.cmd','utf8');
const vars=original.split(/\r?\n/).filter(line=>/^set (SRCS|INCS)=/.test(line));
if(vars.length!==2)throw Error('T398_HOST_BUILD_VARIABLES');
const forward='.codex/local-test/t236-source-host/gl_forward.cpp';
let gl='void T398HostGlPoint(int);\n'+fs.readFileSync(forward,'utf8');
for(const [signature,id] of [['glFlush(void){',1],['glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout){',2],
 ['glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access){',3]]){
 if(gl.split(signature).length!==2)throw Error('T398_HOST_GL_ANCHOR '+signature);
 gl=gl.replace(signature,signature+' T398HostGlPoint('+id+');');
}
fs.writeFileSync(out+'/gl_forward.cpp',gl);
const batch=[
 '@echo off','setlocal',
 'call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'if errorlevel 1 exit /b 1','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',
 ...vars.map(line=>line.replace(forward,out+'/gl_forward.cpp')),
 'cl /nologo /std:c++17 /EHsc /O2 %INCS% test/t398/app_host.cpp test/t324/gl_extra.cpp entry/src/main/cpp/gpu_resident/foundation.cpp %SRCS% /Fe'+out+'/app.exe /Fo'+out+'/',
 'if errorlevel 1 exit /b 1','"'+out+'/app.exe"'+(process.argv.includes('--quick')?' --quick':''),'exit /b %errorlevel%'
].join('\r\n');
fs.writeFileSync(out+'/build.cmd',batch);
cp.execFileSync('cmd.exe',['/c',(out+'/build.cmd').replaceAll('/','\\')],{stdio:'inherit'});
