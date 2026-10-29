@echo off
setlocal EnableDelayedExpansion
call "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat" >nul
if not "!errorlevel!"=="0" exit /b 1
set "KIT=C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0"
set "KITLIB=C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0"
set "INCLUDE=%INCLUDE%;%KIT%\ucrt;%KIT%\um;%KIT%\shared"
set "LIB=%LIB%;%KITLIB%\ucrt\x64;%KITLIB%\um\x64"
if not exist .codex\local-test\t244-host mkdir .codex\local-test\t244-host
cl /nologo /std:c++17 /EHsc /fp:strict /O2 /I. /FItest/p013_native/host_compat/clock.h /Ientry/src/main/cpp/p013renderer test\p013_native\t244_clear_water_independent.cpp entry\src\main\cpp\p013renderer\clear_water_packet.cpp /Fe.codex\local-test\t244-host\t244.exe /Fo.codex\local-test\t244-host\
if not "!errorlevel!"=="0" exit /b 1
.codex\local-test\t244-host\t244.exe
