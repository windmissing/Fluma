@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t272/clock_queue.cpp entry/src/main/cpp/gpu_resident/run.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp /Fe.codex/local-test/t272/clock.exe /Fo.codex/local-test/t272/
if errorlevel 1 exit /b 1
.codex\local-test\t272\clock.exe
