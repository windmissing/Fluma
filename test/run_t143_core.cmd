@echo off
setlocal
call "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat" >nul
set "KIT=C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0"
set "KITLIB=C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0"
set "INCLUDE=%INCLUDE%;%KIT%/ucrt;%KIT%/um;%KIT%/shared"
set "LIB=%LIB%;%KITLIB%/ucrt/x64;%KITLIB%/um/x64"
cd /d %~dp0..
cl /std:c++17 /EHsc /nologo /DP013_MATERIAL_CORE_ONLY /Ientry/src/main/cpp/p013renderer test/p013_native/t114_base_strip_core_test.cpp entry/src/main/cpp/p013renderer/base_strip_uploader.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp /Fe.codex/local-test/p013-host/t143_core.exe /Fo.codex/local-test/p013-host/
if errorlevel 1 exit /b 1
echo BUILD_OK
.codex/local-test/p013-host/t143_core.exe
exit /b %errorlevel%
