@echo off
setlocal
set "MSVC=C:/Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231"
set "WINSDK=C:/Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%WINSDK%\Lib\%SDKVER%\um\x64"
set "PATH=%MSVC%\bin\Hostx64\x64;C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
cd /d D:/hmprojects/HelloWorld
if not exist .codex\local-test\t476-gl mkdir .codex\local-test\t476-gl
cl /nologo /utf-8 /std:c++17 /EHsc /GR /DNOMINMAX /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t476/gl_diff.cpp test/t324/gl_extra.cpp .codex/local-test/t236-source-host/gl_forward.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t476-gl/t476gl.exe /Fo.codex/local-test/t476-gl/
if errorlevel 1 exit /b 1
exit /b 0
