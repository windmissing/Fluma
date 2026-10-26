@echo off
setlocal
call "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat" >nul 2>&1
lib /nologo /machine:x64 /def:test\p013_native\t80_libGLESv2.def /out:.codex\local-test\t80-matrix\libGLESv2.lib
if errorlevel 1 exit /b 1
lib /nologo /machine:x64 /def:test\p013_native\t80_libEGL.def /out:.codex\local-test\t80-matrix\libEGL.lib
if errorlevel 1 exit /b 1
echo LIBS_OK
