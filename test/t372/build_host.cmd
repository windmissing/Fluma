@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
python -X utf8 test/t372/prepare_host.py "%~1"
if errorlevel 1 exit /b 1
set "CPP=%~1/entry/src/main/cpp/p013renderer"
set "OUT=.codex/local-test/t372-host"
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
cl /nologo /FIclock.h /DP013_SINGLE_SAND_HOST_TEST /std:c++17 /EHsc /O2 /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Itest/p013_native/host_compat /I"%CPP%" /I"%OUT%" "%OUT%/host.cpp" "%CPP%/single_sand_packet.cpp" "%CPP%/single_sand_pass.cpp" "%CPP%/fullframe_targets.cpp" "%CPP%/material_uploader.cpp" "%OUT%/gl.cpp" .codex/local-test/t236-source-host/egl.lib /Fe"%OUT%/test.exe" /Fo"%OUT%/"
if errorlevel 1 exit /b 1
.codex\local-test\t372-host\test.exe
