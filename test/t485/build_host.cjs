const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const project=process.argv[2],route=process.argv[3],out=process.argv[4];
if(!project||!['m1','m2route'].includes(route)||!out||fs.existsSync(out))throw Error('PROJECT_ROUTE_FRESH_OUTPUT_REQUIRED');
fs.mkdirSync(out,{recursive:true});
const main=project+'/entry/src/main/cpp',base=main+'/gpu_resident';
// Consume the exact CMake product; translation phase 1 normalizes its CRLF.
const header=project+'/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident/m1_scene_sources.h';
if(!fs.existsSync(header))throw Error('BUILD_PROJECT_FIRST');
fs.copyFileSync(header,out+'/m1_scene_sources.h');
let source=fs.readFileSync('test/t485/scene_host.cpp','utf8');
if(process.argv[5]==='faults')source='#define main PixelFixtureUnusedMain\n'+source+'\n#undef main\n'+fs.readFileSync('test/t485/factory_faults.inc','utf8');
if(route==='m2route')source=source.replaceAll('"m1/','"m2route/').replaceAll('m1::','m2route::').replaceAll('namespace m1 {','namespace m2route {');
const wrappers=fs.readFileSync('test/t475/gl_test.cpp','utf8');
source+='\n'+wrappers.slice(wrappers.indexOf('#define PROC'),wrappers.indexOf('using namespace grs::programcache;'));
source+='\n'+fs.readFileSync('test/t485/gl_exports.inc','utf8');
fs.writeFileSync(out+'/host.cpp',source);
const vars=fs.readFileSync('test/t336/build.cmd','utf8').split(/\r?\n/).filter(l=>/^set (SRCS|INCS)=/.test(l));
const sources=fs.readdirSync(base+'/'+route).filter(n=>n.endsWith('.cpp')).map(n=>base+'/'+route+'/'+n).join(' ');
let forward='.codex/local-test/t236-source-host/gl_forward.cpp';
if(process.argv[5]==='faults'){
 const stub='extern bool t485NoFormats;\n'+fs.readFileSync(forward,'utf8').replace(
  'glGetIntegerv(GLenum pname, GLint *data){',
  'glGetIntegerv(GLenum pname, GLint *data){if(t485NoFormats&&pname==GL_NUM_PROGRAM_BINARY_FORMATS){*data=0;return;}');
 forward=out+'/gl_forward.cpp';fs.writeFileSync(forward,stub);
}
const command=['@echo off','setlocal',
 'call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
 'if errorlevel 1 exit /b 1','set "PATH=C:\\Users\\admin\\AppData\\Local\\Programs\\CokePLUS;%PATH%"',
 ...vars.map(l=>l.replace(/entry\/src\/main\/cpp\/gpu_resident\/m1\/[^ ]+\.cpp /g,'').replaceAll('entry/src/main/cpp',main)
  .replaceAll('.codex/local-test/t236-source-host/gl_forward.cpp',forward)),
 'cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DGRS_PROGRAM_CACHE_TEST_FAULTS /I'+out+' %INCS% '+out+'/host.cpp test/t324/gl_extra.cpp %SRCS% '+sources+' /Fe'+out+'/scene.exe /Fo'+out+'/',
 'exit /b %errorlevel%'].join('\r\n');
fs.writeFileSync(out+'/build.cmd',command);
cp.execFileSync('cmd.exe',['/c',path.join(out,'build.cmd')],{stdio:'inherit',timeout:300000});
