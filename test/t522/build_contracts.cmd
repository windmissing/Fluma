@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /DNOMINMAX /Ientry/src/main/cpp/gpu_resident test/t522/contracts.cpp /Fe.codex/local-test/t522-contracts.exe /Fo.codex/local-test/t522-contracts.obj
if errorlevel 1 exit /b 1
.codex\local-test\t522-contracts.exe
