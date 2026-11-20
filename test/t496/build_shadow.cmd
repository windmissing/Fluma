@echo off
setlocal
set "MSVC=C:/Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231"
set "WINSDK=C:/Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%WINSDK%\Lib\%SDKVER%\um\x64"
set "PATH=%MSVC%\bin\Hostx64\x64;C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
cd /d D:/hmprojects/HelloWorld
if not exist .codex\local-test\t496 mkdir .codex\local-test\t496


cl /nologo /utf-8 /std:c++17 /Ientry/src/main/cpp/gpu_resident /EHsc /O2 test/t496/shadow_units.cpp /Fe.codex/local-test/t496/shadow_units.exe /Fo.codex/local-test/t496/shadow_units.obj
if errorlevel 1 exit /b 1
".codex/local-test/t496/shadow_units.exe"
