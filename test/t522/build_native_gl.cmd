@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
cl /nologo /std:c++17 /EHsc /DNOMINMAX /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /I.codex/local-test/t522-native-host test/t522/native_gl.cpp entry/src/main/cpp/gpu_resident/m3a/kernel_assets.cpp entry/src/main/cpp/gpu_resident/m3a/transport.cpp entry/src/main/cpp/gpu_resident/m3a/controller.cpp entry/src/main/cpp/gpu_resident/m3a/root_owner.cpp entry/src/main/cpp/gpu_resident/m3a/completion_gate.cpp entry/src/main/cpp/gpu_resident/m3a/planning_control.cpp entry/src/main/cpp/gpu_resident/m3a/progress_clock_v3.cpp entry/src/main/cpp/gpu_resident/m3a/quota_snapshot_v3.cpp entry/src/main/cpp/gpu_resident/m3a/quota_reader_v3.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t522-native-gl.exe /Fo.codex/local-test/
if errorlevel 1 exit /b 1
.codex\local-test\t522-native-gl.exe
