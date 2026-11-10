@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
if not exist .codex\local-test\t452-host mkdir .codex\local-test\t452-host
python -X utf8 entry/src/main/cpp/gpu_resident/m2/assemble_kernels.py .codex/local-test/t452-host/source-v3.glsl
if errorlevel 1 exit /b 1
python -X utf8 entry/src/main/cpp/gpu_resident/m2/assemble_kernels.py .codex/local-test/t452-host/quantum-v3.glsl --quantum
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t452/source_host.cpp entry/src/main/cpp/gpu_resident/integer.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t452-host/roots-v3.exe /Fo.codex/local-test/t452-host/
if errorlevel 1 exit /b 1
.codex\local-test\t452-host\roots-v3.exe
