@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /DGRS_T279_HOST_TEST /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident /I.codex/local-test/t279 /I.codex/local-test/t240-source test/gpu_resident/t279_quantum_host.cpp entry/src/main/cpp/gpu_resident/m1/quantum_candidate.cpp entry/src/main/cpp/gpu_resident/m1/quantum_kernel.cpp entry/src/main/cpp/gpu_resident/m1/source_transport.cpp entry/src/main/cpp/gpu_resident/m1/source_owner.cpp entry/src/main/cpp/gpu_resident/m1/source_kernel.cpp entry/src/main/cpp/gpu_resident/m1/profile.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/integer.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t279/quantum.exe /Fo.codex/local-test/t279/
