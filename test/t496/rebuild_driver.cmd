@echo off
setlocal
set "MSVC=C:/Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231"
set "WINSDK=C:/Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%WINSDK%\Lib\%SDKVER%\um\x64"
set "PATH=%MSVC%\bin\Hostx64\x64;C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
cd /d D:/hmprojects/HelloWorld
if not exist .codex\local-test\t496 mkdir .codex\local-test\t496

rem Frozen single source: t492-m1-delivery (route=m1, dcf105d+T492-overlay).
rem EVERY include path and implementation cpp resolves to this tree only.
set SRC=.codex/local-test/t492-m1-delivery/entry/src/main/cpp
set SRCS=%SRC%/p013renderer/fullframe_targets.cpp %SRC%/gpu_resident/control.cpp %SRC%/gpu_resident/integer.cpp test/t496/gl_forward_sink.cpp .codex/local-test/t236-source-host/egl.lib
set INCS=/DNOMINMAX /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /I%SRC%/gpu_resident /Itest/t295 /I.codex/local-test/t281 /I.codex/local-test/t236-source-host /FIhost_clock.h /I.codex/local-test/t280 /I.codex/local-test/t279 /I.codex/local-test/t240-source /I.codex/local-test/t492-m1-delivery/entry/.cxx/default/default/debug/arm64-v8a/gpu_resident


cl /nologo /utf-8 /std:c++17 /EHsc /O2 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT /DGRS_APP_M1 %INCS% /c test/t496/external_source_diff.cpp /Fo.codex/local-test/t496/external_source_diff.obj
if errorlevel 1 exit /b 1
cl /nologo /EHsc .codex/local-test/t496/external_source_diff.obj .codex/local-test/t496/gl_extra.obj .codex/local-test/t496/fullframe_targets.obj .codex/local-test/t496/control.obj .codex/local-test/t496/integer.obj .codex/local-test/t496/gl_forward_sink.obj .codex/local-test/t496/foundation.obj .codex/local-test/t496/full_kernel.obj .codex/local-test/t496/full_transport.obj .codex/local-test/t496/profile.obj .codex/local-test/t496/quantum_candidate.obj .codex/local-test/t496/quantum_kernel.obj .codex/local-test/t496/run.obj .codex/local-test/t496/scene.obj .codex/local-test/t496/source_kernel.obj .codex/local-test/t496/source_owner.obj .codex/local-test/t496/source_transport.obj .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t496/t496.exe
if errorlevel 1 exit /b 1
