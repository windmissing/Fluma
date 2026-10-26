@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl.exe /std:c++17 /EHsc /nologo /DP013_MATERIAL_CORE_ONLY /Ientry/src/main/cpp/p013renderer test/p013_native/material_uploader_test.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp /Fe.codex/local-test/p054-material.exe /Fo.codex/local-test/p013-host/
if errorlevel 1 exit /b 1
.codex\local-test\p054-material.exe
