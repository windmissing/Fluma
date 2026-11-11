@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
cl /nologo /std:c++17 /EHsc /DNOMINMAX /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t475/gl_test.cpp test/t324/gl_extra.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t475-gl.exe /Fo.codex/local-test/
if errorlevel 1 exit /b 1
.codex\local-test\t475-gl.exe %1 cold
if errorlevel 1 exit /b 1
.codex\local-test\t475-gl.exe %1 warm


if errorlevel 1 exit /b 1
.codex\local-test\t475-gl.exe %1-disk cold
if errorlevel 1 exit /b 1
.codex\local-test\t475-gl.exe %1-disk corrupt_hash
if errorlevel 1 exit /b 1
.codex\local-test\t475-gl.exe %1-disk truncate
