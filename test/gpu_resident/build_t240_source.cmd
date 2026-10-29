@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
cl /nologo /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk test/gpu_resident/t240_source_host.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t240-source/source.exe /Fo.codex/local-test/t240-source/
if errorlevel 1 exit /b 1
.codex\local-test\t240-source\source.exe
