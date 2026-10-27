@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t171 mkdir .codex\local-test\t171
cl /std:c++17 /EHsc /nologo /fp:strict /Itest/p013_compute/host_napi /Itest/p013_compute test/p013_compute/t171_binding_test.cpp test/p013_compute/pair_prepare_v1.cpp test/p013_compute/pair_owner_v1.cpp /Fe.codex/local-test/t171/binding.exe /Fo.codex/local-test/t171/
if errorlevel 1 exit /b 1
.codex\local-test\t171\binding.exe
if errorlevel 1 exit /b 1
cl /std:c++17 /EHsc /nologo /fp:strict /FIclock.h /Itest/p013_native/host_compat /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Itest/p013_compute test/p013_compute/t171_shader_source_test.cpp .codex/local-test/t80-matrix/libEGL.lib .codex/local-test/t80-matrix/libGLESv2.lib /Fe.codex/local-test/t171/shader.exe /Fo.codex/local-test/t171/
if errorlevel 1 exit /b 1
set "PATH=C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
.codex\local-test\t171\shader.exe .codex/local-test/t171/private-shader.comp
if errorlevel 1 exit /b 1
cl /std:c++17 /nologo /EP /Itest/p013_compute/host_napi /Itest/p013_compute test/p013_compute/pair_binding_v1.cpp > .codex/local-test/t171/release-preprocessed.i
exit /b %errorlevel%
