@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t159 mkdir .codex\local-test\t159
cl /std:c++17 /EHsc /nologo /fp:strict /Itest/p013_compute test/p013_compute/t159_identity_test.cpp /Fe.codex/local-test/t159/identity.exe /Fo.codex/local-test/t159/
if errorlevel 1 exit /b 1
.codex\local-test\t159\identity.exe
if errorlevel 1 exit /b 1
cl /std:c++17 /EHsc /nologo /fp:strict /Itest/p013_compute/host_napi /Itest/p013_compute test/p013_compute/t159_binding_test.cpp test/p013_compute/pair_prepare_v1.cpp test/p013_compute/pair_owner_v1.cpp /Fe.codex/local-test/t159/binding.exe /Fo.codex/local-test/t159/
if errorlevel 1 exit /b 1
.codex\local-test\t159\binding.exe
if errorlevel 1 exit /b 1
call test\p013_compute\build_t157.cmd
exit /b %errorlevel%
