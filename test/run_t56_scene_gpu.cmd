@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
lib /nologo /machine:x64 /def:.codex/local-test/t56-gpu/libEGL.def /out:.codex/local-test/t56-gpu/libEGL.lib
if errorlevel 1 exit /b 1
lib /nologo /machine:x64 /def:.codex/local-test/t56-gpu/libGLESv2.def /out:.codex/local-test/t56-gpu/libGLESv2.lib
if errorlevel 1 exit /b 1
cl /std:c++17 /EHsc /nologo /I. /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /FItest/p013_native/host_compat/clock.h /Itest/p013_native/host_compat /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/p013renderer /I.codex/local-test/t56-gpu test/p013_native/t56_scene_gpu_test.cpp entry/src/main/cpp/p013renderer/scene_compositor.cpp entry/src/main/cpp/p013renderer/scene_gpu.cpp entry/src/main/cpp/p013renderer/scene_layers.cpp entry/src/main/cpp/p013renderer/fullframe_targets.cpp entry/src/main/cpp/p013renderer/dualfield_pass.cpp entry/src/main/cpp/p013renderer/material_pass.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp .codex/local-test/t56-gpu/libEGL.lib .codex/local-test/t56-gpu/libGLESv2.lib /Fe.codex/local-test/t56-gpu/t56_scene_gpu.exe /Fo.codex/local-test/t56-gpu/
if errorlevel 1 exit /b 1
.codex\local-test\t56-gpu\t56_scene_gpu.exe
