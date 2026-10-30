@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t317 mkdir .codex\local-test\t317
cl /nologo /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t236-source-host /FIhost_clock.h /Itest/t295 /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t317/bridge_test.cpp entry/src/main/cpp/gpu_resident/run.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp /Fe.codex/local-test/t317/test.exe /Fo.codex/local-test/t317/
if errorlevel 1 exit /b 1
.codex\local-test\t317\test.exe
