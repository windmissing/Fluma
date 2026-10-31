@echo off
rem T336 (T324 Amendment 2) rebuild of the five non-device suites.
rem NOTE: this script does NOT call vcvars64.bat (its reg.exe invocation is
rem sandbox-blocked); the caller must export INCLUDE/LIB/PATH with the
rem MSVC+SDK toolchain first (see the bash export block in the T336 log).
setlocal
rem partial_host and app_gl define their own grs::GetFoundationInfo stub,
rem so foundation.cpp is EXCLUDED from the real-GL link units (T329 did the
rem same). run_contract is a pure scripted double: single translation unit.
set SRCS=entry/src/main/cpp/gpu_resident/m1/run.cpp entry/src/main/cpp/gpu_resident/m1/scene.cpp entry/src/main/cpp/p013renderer/fullframe_targets.cpp entry/src/main/cpp/gpu_resident/m1/full_transport.cpp entry/src/main/cpp/gpu_resident/m1/full_kernel.cpp entry/src/main/cpp/gpu_resident/m1/quantum_candidate.cpp entry/src/main/cpp/gpu_resident/m1/quantum_kernel.cpp entry/src/main/cpp/gpu_resident/m1/source_transport.cpp entry/src/main/cpp/gpu_resident/m1/source_owner.cpp entry/src/main/cpp/gpu_resident/m1/source_kernel.cpp entry/src/main/cpp/gpu_resident/m1/profile.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib
set INCS=/DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident /Itest/t295 /I.codex/local-test/t281 /I.codex/local-test/t236-source-host /FIhost_clock.h /I.codex/local-test/t280 /I.codex/local-test/t279 /I.codex/local-test/t240-source /Ientry/.cxx/default/default/debug/arm64-v8a/gpu_resident
if not exist .codex\local-test\t336 mkdir .codex\local-test\t336
cl /nologo /std:c++17 /EHsc /O2 %INCS% test/t324/partial_host.cpp test/t324/gl_extra.cpp test/t336/fwd_extra.cpp %SRCS% /Fe.codex/local-test/t336/partial.exe /Fo.codex/local-test/t336/
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 %INCS% test/t324/run_contract.cpp /Fe.codex/local-test/t336/run_contract.exe /Fo.codex/local-test/t336/
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t236-source-host /FIhost_clock.h /Itest/t295 /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t324/bridge_test.cpp entry/src/main/cpp/gpu_resident/run.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp /Fe.codex/local-test/t336/bridge.exe /Fo.codex/local-test/t336/
if errorlevel 1 exit /b 1
rem app_gl links the FULL production chain like t317/build-gl.cmd:
rem foundation.cpp included (app_gl has no GetFoundationInfo stub), but its
rem own glUniform1f/4fv forwards mean fwd_extra.cpp stays out.
cl /nologo /std:c++17 /EHsc /O2 %INCS% test/t324/app_gl.cpp test/t324/gl_extra.cpp entry/src/main/cpp/gpu_resident/foundation.cpp %SRCS% /Fe.codex/local-test/t336/app.exe /Fo.codex/local-test/t336/
if errorlevel 1 exit /b 1
echo BUILD_OK
