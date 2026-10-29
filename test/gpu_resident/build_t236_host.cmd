@echo off
setlocal EnableDelayedExpansion
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "!errorlevel!"=="0" exit /b 1
if not exist .codex\local-test\t236-host mkdir .codex\local-test\t236-host
for %%V in (Od O2) do (
 cl /nologo /std:c++17 /EHsc /fp:strict /%%V /Ientry/src/main/cpp/gpu_resident test/gpu_resident/t236_foundation_test.cpp entry/src/main/cpp/gpu_resident/integer.cpp entry/src/main/cpp/gpu_resident/control.cpp entry/src/main/cpp/gpu_resident/resources.cpp entry/src/main/cpp/gpu_resident/product_wire.cpp /Fe.codex/local-test/t236-host/foundation-%%V.exe /Fo.codex/local-test/t236-host/
 if not "!errorlevel!"=="0" exit /b 1
 .codex\local-test\t236-host\foundation-%%V.exe
 if not "!errorlevel!"=="0" exit /b 1
)
