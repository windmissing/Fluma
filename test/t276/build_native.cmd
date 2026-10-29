@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t276-native mkdir .codex\local-test\t276-native
cl /nologo /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /I.codex/local-test/t236-source-host /FIhost_clock.h /Itest/t276 test/t276/pixel_test.cpp /Fe.codex/local-test/t276-native/test.exe /Fo.codex/local-test/t276-native/
if errorlevel 1 exit /b 1
.codex\local-test\t276-native\test.exe
