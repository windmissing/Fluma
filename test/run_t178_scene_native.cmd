@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /I. /std:c++17 /EHsc /nologo /DP013_MATERIAL_CORE_ONLY /I.codex/local-test/p013-host/sdk /Ientry/src/main/cpp/p013renderer /FItest/p013_native/host_compat/clock.h test/p013_native/t178_scene_diagnostic_test.cpp entry/src/main/cpp/p013renderer/scene_compositor.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp entry/src/main/cpp/p013renderer/base_strip_uploader.cpp /Fe.codex/local-test/p013-host/t178_scene.exe /Fo.codex/local-test/p013-host/
if errorlevel 1 exit /b 1
.codex\local-test\p013-host\t178_scene.exe
