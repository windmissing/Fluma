@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t246-host mkdir .codex\local-test\t246-host
cl /std:c++17 /EHsc /nologo test/t246/protocol_test.cpp /Fe.codex/local-test/t246-host/protocol.exe /Fo.codex/local-test/t246-host/
if errorlevel 1 exit /b 1
.codex\local-test\t246-host\protocol.exe
