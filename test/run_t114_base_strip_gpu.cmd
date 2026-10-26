@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Program Files\Huawei\DevEco Studio\jbr\bin;%PATH%"
lib /nologo /machine:x64 /def:test/p013_native/t114_gles_layer.def /out:.codex/local-test/t56-gpu/t114_gles_layer.lib
if errorlevel 1 exit /b 1
cl /std:c++17 /EHsc /nologo /I. /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /FItest/p013_native/host_compat/clock.h /Itest/p013_native/host_compat /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/p013renderer test/p013_native/t114_base_strip_gpu_test.cpp entry/src/main/cpp/p013renderer/base_strip_uploader.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp .codex/local-test/t56-gpu/libEGL.lib .codex/local-test/t56-gpu/libGLESv2.lib .codex/local-test/t56-gpu/t114_gles_layer.lib /Fe.codex/local-test/t56-gpu/t114_base_strip_gpu.exe /Fo.codex/local-test/t56-gpu/
if errorlevel 1 exit /b 1
.codex\local-test\t56-gpu\t114_base_strip_gpu.exe
