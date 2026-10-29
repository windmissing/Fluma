@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t272 mkdir .codex\local-test\t272
lib /nologo /def:test/t272/node.def /machine:x64 /out:.codex/local-test/t272/node.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /LD /O2 /DNAPI_EXTERN= /FItest/p013_native/host_compat/clock.h /I. /I.codex/local-test/p013-host/sdk /Ientry/src/main/cpp/gpu_resident test/t272/addon.cpp .codex/local-test/t272/node.lib /Fo.codex/local-test/t272/ /Fe.codex/local-test/t272/bridge.node
if errorlevel 1 exit /b 1
node test/t272/napi.cjs
