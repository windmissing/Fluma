@echo off
setlocal
set "MSVC=C:/Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231"
set "WINSDK=C:/Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%WINSDK%\Lib\%SDKVER%\um\x64"
set "PATH=%MSVC%\bin\Hostx64\x64;C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
cd /d D:/hmprojects/HelloWorld
if not exist .codex\local-test\t481 mkdir .codex\local-test\t481
cl /nologo /utf-8 /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /I.codex/local-test/t467-unit/entry/src/main/cpp/gpu_resident /Itest/t467 /Ientry/src/main/cpp/gpu_resident /Itest/t295 /I.codex/local-test/t281 /I.codex/local-test/t236-source-host /FIhost_clock.h /I.codex/local-test/t280 /I.codex/local-test/t279 /I.codex/local-test/t240-source /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT /Ientry/.cxx/default/default/debug/arm64-v8a/gpu_resident test/t481/recovery_diff.cpp test/t324/gl_extra.cpp entry/src/main/cpp/gpu_resident/foundation.cpp entry/src/main/cpp/gpu_resident/m1/run.cpp entry/src/main/cpp/gpu_resident/m1/scene.cpp entry/src/main/cpp/p013renderer/fullframe_targets.cpp entry/src/main/cpp/gpu_resident/m1/full_transport.cpp entry/src/main/cpp/gpu_resident/m1/full_kernel.cpp entry/src/main/cpp/gpu_resident/m1/quantum_candidate.cpp entry/src/main/cpp/gpu_resident/m1/quantum_kernel.cpp entry/src/main/cpp/gpu_resident/m1/source_transport.cpp entry/src/main/cpp/gpu_resident/m1/source_owner.cpp entry/src/main/cpp/gpu_resident/m1/source_kernel.cpp entry/src/main/cpp/gpu_resident/m1/profile.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t481/t481.exe /Fo.codex/local-test/t481/
if errorlevel 1 exit /b 1
exit /b 0
