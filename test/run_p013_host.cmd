@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "P013SDK=C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\native"
rem bridge_host_test includes and executes lifecycle_test_main; run each assertion once.
cl.exe /std:c++17 /EHsc /nologo /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /DP013_MATERIAL_CORE_ONLY /DP013_MATERIAL_PASS_CORE_ONLY /FItest/p013_native/host_compat/clock.h /I. /Ientry/src/main/cpp/p013renderer /Itest/p013_native/host_compat /I.codex/local-test/p013-host/sdk test/p013_native/bridge_host_test.cpp entry/src/main/cpp/p013renderer/egl_context.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp entry/src/main/cpp/p013renderer/material_pass.cpp /Fe.codex/local-test/p013-host/p013_bridge.exe /Fo.codex/local-test/p013-host/
if errorlevel 1 exit /b 1
.codex\local-test\p013-host\p013_bridge.exe
