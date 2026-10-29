@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
lib /nologo /def:test/t258/node.def /machine:x64 /out:.codex/local-test/t258/node.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /LD /Od /fp:strict /I. /DNAPI_EXTERN= /FItest/p013_native/host_compat/clock.h /I.codex/local-test/p013-host/sdk /Ientry/src/main/cpp/p013renderer test/t258/addon.cpp entry/src/main/cpp/p013renderer/clear_water_packet.cpp .codex/local-test/t258/node.lib /Fo.codex/local-test/t258/ /Fe.codex/local-test/t258/identity.node
if errorlevel 1 exit /b 1
node test/t258/selftest.cjs
