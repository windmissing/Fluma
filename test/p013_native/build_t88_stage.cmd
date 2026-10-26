@echo off
setlocal
call "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat" >nul 2>&1
set "KIT=C:/Program Files (x86)\Windows Kits\10\Include\10.0.26100.0"
set "KITLIB=C:/Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0"
set "INCLUDE=%INCLUDE%;%KIT%\ucrt;%KIT%\um;%KIT%\shared"
set "LIB=%LIB%;%KITLIB%\ucrt\x64;%KITLIB%\um\x64"
set "PATH=C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
cd /d %~dp0..\..
cl /std:c++17 /EHsc /nologo /FIclock.h /Itest\p013_native\host_compat /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /Ientry\src\main\cpp\p013renderer /I.codex/local-test/t55-gpu/sdk /I.codex/local-test/t55-gpu test\p013_native\t88_stage_probe.cpp entry\src\main\cpp\p013renderer\material_pass.cpp entry\src\main\cpp\p013renderer\material_uploader.cpp .codex\local-test\t80-matrix\libEGL.lib .codex\local-test\t80-matrix\libGLESv2.lib /Fe.codex\local-test\t80-matrix\t80_matrix.exe /Fo.codex\local-test\t80-matrix\
if errorlevel 1 exit /b 1
echo BUILD_OK
.codex\local-test\t80-matrix\t80_matrix.exe .codex/local-test/t55-fixtures .codex/local-test/t88
exit /b %errorlevel%
