@echo off
setlocal EnableDelayedExpansion
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "!errorlevel!"=="0" exit /b 1
set "INCLUDE=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;%INCLUDE%"
set "LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;%LIB%"
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
cd /d D:\hmprojects\HelloWorld
lib /nologo /def:.codex/local-test/t236-source-host/egl.def /machine:x64 /out:.codex/local-test/t236-source-host/egl.lib
if not "!errorlevel!"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /Od /fp:strict /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident /I.codex/local-test/t236-source-host /I.codex/local-test/t236-shaders /FIhost_clock.h test/p260_host/t260_host.cpp .codex/local-test/t236-source-host/gl_forward.cpp entry/src/main/cpp/gpu_resident/source_transport.cpp entry/src/main/cpp/gpu_resident/scene.cpp entry/src/main/cpp/gpu_resident/run.cpp entry/src/main/cpp/p013renderer/fullframe_targets.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp entry/src/main/cpp/gpu_resident/resources.cpp entry/src/main/cpp/gpu_resident/product_wire.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t260-host.exe /Fo.codex/local-test/t260-host/
if not "!errorlevel!"=="0" exit /b 1
.codex\local-test\t260-host.exe
if not "!errorlevel!"=="0" exit /b 1
