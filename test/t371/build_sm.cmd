@echo off
setlocal
rem T371 SM harness build. vcvars64.bat requires reg.exe (blocked in some sandboxes);
rem if vcvars succeeds use it, otherwise fall back to explicit INCLUDE/LIB.
set "MSVC=C://Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231"
set "WINSDK=C://Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"
set "INCLUDE=%MSVC%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared;%WINSDK%\Include\%SDKVER%\winrt"
set "LIB=%MSVC%\lib\x64;%WINSDK%\Lib\%SDKVER%\ucrt\x64;%WINSDK%\Lib\%SDKVER%\um\x64"
set "PATH=%MSVC%\bin\Hostx64\x64;C://Users//admin//AppData//Local//Programs//CokePLUS;%PATH%"
cd /d "%~dp0..\.."
python -X utf8 test/t364/prepare_host.py
if errorlevel 1 exit /b 1
cl /nologo /FIclock.h /DP013_SINGLE_SAND_HOST_TEST /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Itest/p013_native/host_compat /Ientry/src/main/cpp/p013renderer /I.codex/local-test/t364-host test/t371/host_sm.cpp test/t371/host_sm_stubs.cpp entry/src/main/cpp/p013renderer/single_sand_packet.cpp entry/src/main/cpp/p013renderer/single_sand_pass.cpp entry/src/main/cpp/p013renderer/fullframe_targets.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp .codex/local-test/t364-host/gl.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t364-host/test_sm.exe /Fo.codex/local-test/t364-host/
if errorlevel 1 exit /b 1
cd /d "%~dp0..\..\.codex\local-test\t364-host"
test_sm.exe
