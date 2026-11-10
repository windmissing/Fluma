@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
if not exist .codex\local-test\t452-host mkdir .codex\local-test\t452-host
cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t452/capability_host.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t452-host/capability.exe /Fo.codex/local-test/t452-host/
if errorlevel 1 exit /b 1
.codex\local-test\t452-host\capability.exe
