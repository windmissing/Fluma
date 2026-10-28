@echo off
setlocal EnableDelayedExpansion
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "!errorlevel!"=="0" exit /b 1
if not exist .codex\local-test\t226-host mkdir .codex\local-test\t226-host
for %%V in (Od O2) do (
 cl /nologo /std:c++17 /EHsc /fp:strict /%%V /I. /FItest/p013_native/host_compat/clock.h /Ientry/src/main/cpp/p013renderer test/p013_native/t226_clear_water_test.cpp entry/src/main/cpp/p013renderer/clear_water_packet.cpp /Fe.codex/local-test/t226-host/packet-%%V.exe /Fo.codex/local-test/t226-host/
 if not "!errorlevel!"=="0" exit /b 1
 .codex\local-test\t226-host\packet-%%V.exe
 if not "!errorlevel!"=="0" exit /b 1
)
