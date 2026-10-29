@echo off
setlocal EnableDelayedExpansion
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "!errorlevel!"=="0" exit /b 1
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
lib /nologo /def:.codex/local-test/t236-source-host/egl.def /machine:x64 /out:.codex/local-test/t236-source-host/egl.lib
if not "!errorlevel!"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /Od /fp:strict /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident /I.codex/local-test/t236-source-host /I.codex/local-test/t236-shaders /FIhost_clock.h test/gpu_resident/t236_source_host.cpp .codex/local-test/t236-source-host/gl_forward.cpp entry/src/main/cpp/gpu_resident/source_transport.cpp entry/src/main/cpp/gpu_resident/scene.cpp entry/src/main/cpp/gpu_resident/run.cpp entry/src/main/cpp/p013renderer/fullframe_targets.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp entry/src/main/cpp/gpu_resident/resources.cpp entry/src/main/cpp/gpu_resident/product_wire.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t236-source-host/source.exe /Fo.codex/local-test/t236-source-host/
if not "!errorlevel!"=="0" exit /b 1
.codex\local-test\t236-source-host\source.exe %*
if not "!errorlevel!"=="0" exit /b 1
