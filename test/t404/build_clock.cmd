@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t404-host mkdir .codex\local-test\t404-host
cl /nologo /std:c++17 /EHsc /O2 test/t404/clock_host.cpp /Fe.codex/local-test/t404-host/clock.exe /Fo.codex/local-test/t404-host/
if errorlevel 1 exit /b 1
.codex\local-test\t404-host\clock.exe
