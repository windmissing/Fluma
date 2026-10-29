@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t270-native mkdir .codex\local-test\t270-native
cl /nologo /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /I.codex/local-test/t236-source-host /FIhost_clock.h /Ientry/src/main/cpp/p013renderer /Itest/t270 test/t270/native_test.cpp /Fe.codex/local-test/t270-native/test.exe /Fo.codex/local-test/t270-native/
if errorlevel 1 exit /b 1
.codex\local-test\t270-native\test.exe
