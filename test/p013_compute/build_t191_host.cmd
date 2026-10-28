@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%V in (Od O2) do (
  if not exist .codex\local-test\t191-host-%%V mkdir .codex\local-test\t191-host-%%V
  cl /std:c++17 /EHsc /nologo /%%V /fp:strict /Itest/p013_compute/host_napi /Itest/p013_compute test/p013_compute/t171_binding_test.cpp test/p013_compute/pair_prepare_v1.cpp test/p013_compute/pair_owner_v1.cpp /Fe.codex/local-test/t191-host-%%V/binding.exe /Fo.codex/local-test/t191-host-%%V/
  if errorlevel 1 exit /b 1
  .codex\local-test\t191-host-%%V\binding.exe
  if errorlevel 1 exit /b 1
)
exit /b 0
