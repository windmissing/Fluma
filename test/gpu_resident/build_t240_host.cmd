@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t240-host mkdir .codex\local-test\t240-host
cl /nologo /std:c++17 /EHsc /O2 /Ientry/src/main/cpp/gpu_resident test/gpu_resident/t240_profile_test.cpp entry/src/main/cpp/gpu_resident/m1/profile.cpp /Fe.codex/local-test/t240-host/profile.exe /Fo.codex/local-test/t240-host/
if errorlevel 1 exit /b 1
.codex\local-test\t240-host\profile.exe
