@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t236-source-host /FIhost_clock.h /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/gpu_resident/t281_run_test.cpp entry/src/main/cpp/gpu_resident/m1/run.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp /Fe.codex/local-test/t281/run.exe /Fo.codex/local-test/t281/
if errorlevel 1 exit /b 1
.codex\local-test\t281\run.exe
