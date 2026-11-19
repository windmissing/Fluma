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

rem ---- Source-consistency gate (T494 SS2): class layout must match ----
cl /nologo /utf-8 /std:c++17 /EHsc /Zs /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT /DGRS_APP_M1 %INCS% /d1reportSingleClassLayoutFullM1Transport test/t496/external_source_diff.cpp > .codex/local-test/t496/layout-driver.txt 2>&1
if errorlevel 1 (echo GATE_FAIL driver syntax & exit /b 1)
cl /nologo /utf-8 /std:c++17 /EHsc /Zs /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT /DGRS_APP_M1 %INCS% /d1reportSingleClassLayoutFullM1Transport %SRC%/gpu_resident/m1/full_transport.cpp > .codex/local-test/t496/layout-impl.txt 2>&1
if errorlevel 1 (echo GATE_FAIL impl syntax & exit /b 1)
rem T547 §1 fix: MSVC /d1reportSingleClassLayout writes "size(...)" not "sizeof".
rem Compare the non-empty class-layout sections; empty section = gate failure.
rem T547 §1 nonempty gate + precise class-layout compare (python, noise-immune)
python -X utf8 test/t496/layout_gate.py .codex/local-test/t496/layout-driver.txt .codex/local-test/t496/layout-impl.txt FullM1Transport
if errorlevel 1 (
  echo GATE_FAIL FullM1Transport layout mismatch or empty (T547 nonempty-gate^)
  exit /b 1
)
echo GATE_OK single-source layout consistent

cl /nologo /utf-8 /std:c++17 /EHsc /O2 /DGRS_T456_BOOT /DGRS_PROGRESS_V2_EXPERIMENT /DGRS_APP_M1 %INCS% test/t496/external_source_diff.cpp test/t324/gl_extra.cpp %SRC%/gpu_resident/foundation.cpp %SRCS% %SRC%/gpu_resident/m1/full_kernel.cpp %SRC%/gpu_resident/m1/full_transport.cpp %SRC%/gpu_resident/m1/profile.cpp %SRC%/gpu_resident/m1/quantum_candidate.cpp %SRC%/gpu_resident/m1/quantum_kernel.cpp %SRC%/gpu_resident/m1/run.cpp %SRC%/gpu_resident/m1/scene.cpp %SRC%/gpu_resident/m1/source_kernel.cpp %SRC%/gpu_resident/m1/source_owner.cpp %SRC%/gpu_resident/m1/source_transport.cpp /Fe.codex/local-test/t496/t496.exe /Fo.codex/local-test/t496/
if errorlevel 1 exit /b 1
rem Build only. Run one named arm via run_bounded.py; total <=420s, no device.
exit /b 0
