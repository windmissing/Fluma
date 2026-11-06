@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
python -X utf8 test/t364/prepare_host.py
if errorlevel 1 exit /b 1
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
cl /nologo /FIclock.h /DP013_SINGLE_SAND_HOST_TEST /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Itest/p013_native/host_compat /Ientry/src/main/cpp/p013renderer /I.codex/local-test/t364-host test/t364/host.cpp entry/src/main/cpp/p013renderer/single_sand_packet.cpp entry/src/main/cpp/p013renderer/single_sand_pass.cpp entry/src/main/cpp/p013renderer/fullframe_targets.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp .codex/local-test/t364-host/gl.cpp .codex/local-test/t236-source-host/egl.lib /Fe.codex/local-test/t364-host/test.exe /Fo.codex/local-test/t364-host/
if errorlevel 1 exit /b 1
.codex\local-test\t364-host\test.exe
