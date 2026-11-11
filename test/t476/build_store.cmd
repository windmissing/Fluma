@echo off
setlocal
set "MSVC=C:/Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231"
set "WINSDK=C:/Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%WINSDK%\Lib\%SDKVER%\um\x64"
set "PATH=%MSVC%\bin\Hostx64\x64;C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
cd /d D:/hmprojects/HelloWorld
if not exist .codex\local-test\t476 mkdir .codex\local-test\t476
if not exist .codex\local-test\t476-data mkdir .codex\local-test\t476-data
cl /nologo /utf-8 /std:c++17 /EHsc /GR /DNOMINMAX test/t476/store_diff.cpp /Fe.codex/local-test/t476/t476store.exe /Fo.codex/local-test/t476/
if errorlevel 1 exit /b 1
if exist .codex\local-test\t476-data rmdir /s /q .codex\local-test\t476-data
mkdir .codex\local-test\t476-data
.codex\local-test\t476\t476store.exe .codex\local-test\t476-data
exit /b %errorlevel%
