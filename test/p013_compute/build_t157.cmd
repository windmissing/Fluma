@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t157 mkdir .codex\local-test\t157
cl /std:c++17 /EHsc /nologo /fp:strict /Itest/p013_compute test/p013_compute/t157_layout_test.cpp test/p013_compute/pair_prepare_v1.cpp test/p013_compute/pair_owner_v1.cpp /Fe.codex/local-test/t157/t157_layout.exe /Fo.codex/local-test/t157/
if errorlevel 1 exit /b 1
.codex\local-test\t157\t157_layout.exe
if errorlevel 1 exit /b 1
call test\p013_compute\build_t141.cmd
exit /b %errorlevel%
