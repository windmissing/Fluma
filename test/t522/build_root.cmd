@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
cl /nologo /std:c++17 /EHsc /DNOMINMAX /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk test/t522/source_gl.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t522-source.exe /Fo.codex/local-test/
if errorlevel 1 exit /b 1
.codex\local-test\t522-source.exe root
if errorlevel 1 exit /b 1
.codex\local-test\t522-source.exe root-offset

if errorlevel 1 exit /b 1
.codex\local-test\t522-source.exe root-clamped

if errorlevel 1 exit /b 1
.codex\local-test\t522-source.exe root-full
