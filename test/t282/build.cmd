@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t282 mkdir .codex\local-test\t282
cl /nologo /std:c++17 /EHsc /O2 /Ientry/src/main/cpp/gpu_resident test/t282/state.cpp /Fe.codex/local-test/t282/state.exe /Fo.codex/local-test/t282/
if errorlevel 1 exit /b 1
.codex\local-test\t282\state.exe
