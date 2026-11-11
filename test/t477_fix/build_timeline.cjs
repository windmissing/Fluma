const fs = require('node:fs');
const cp = require('node:child_process');
const path = require('node:path');
const project = process.argv[2];
const out = process.argv[3];
if (!project || !out || fs.existsSync(out)) throw Error('EXPLICIT_PROJECT_AND_FRESH_OUTPUT_REQUIRED');
fs.mkdirSync(out, {recursive:true});
const command = [
  '@echo off', 'setlocal',
  'call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
  'if errorlevel 1 exit /b 1',
  'cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC ' +
    '/I.codex/local-test/t55-gpu/sdk /I' + project + '/entry/src/main/cpp/gpu_resident ' +
    'test/t477_fix/timeline.cpp /Fe' + out + '/timeline.exe /Fo' + out + '/',
  'if errorlevel 1 exit /b 1',
  out.replaceAll('/', '\\') + '\\timeline.exe ' + out + '/pages.ndjson',
  'exit /b %errorlevel%'
].join('\r\n');
fs.writeFileSync(path.join(out,'build.cmd'),command);
cp.execFileSync('cmd.exe',['/c',path.join(out,'build.cmd')],{stdio:'inherit'});
