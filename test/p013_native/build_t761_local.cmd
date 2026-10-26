@echo off
setlocal
call "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat" >nul 2>&1
set "KIT=C:/Program Files (x86)\Windows Kits\10\Include\10.0.26100.0"
set "KITLIB=C:/Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0"
set "INCLUDE=%INCLUDE%;%KIT%\ucrt;%KIT%\um;%KIT%\shared"
set "LIB=%LIB%;%KITLIB%\ucrt\x64;%KITLIB%\um\x64"
cd /d %~dp0..\..
cl.exe /std:c++17 /EHsc /nologo /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /DP013_MATERIAL_CORE_ONLY /DP013_MATERIAL_PASS_CORE_ONLY /DP013_MATERIAL_BRIDGE_TEST /FItest/p013_native/host_compat/clock.h /I. /Ientry/src/main/cpp/p013renderer /Itest/p013_native/host_compat /I.codex/local-test/p013-host/sdk /c test/p013_native/t761_material_bridge_entries_test.cpp entry/src/main/cpp/p013renderer/egl_context.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp entry/src/main/cpp/p013renderer/material_pass.cpp /Fo.codex\local-test\p013-host\
if errorlevel 1 exit /b 1
rem Link DIRECTLY onto the surviving copy path (overwrite bytes, no new filename)
link.exe /NOLOGO /OUT:.codex\local-test\p013-host\t761_exec.exe /SUBSYSTEM:CONSOLE .codex\local-test\p013-host\t761_material_bridge_entries_test.obj .codex\local-test\p013-host\egl_context.obj .codex\local-test\p013-host\material_uploader.obj .codex\local-test\p013-host\material_pass.obj
if errorlevel 1 exit /b 1
echo LINK_OK_ONTO_SURVIVING_PATH
exit /b 0
