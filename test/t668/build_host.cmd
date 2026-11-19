@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /utf-8 /EHsc /std:c++17 /DT668_STORE_ONLY /Ientry/src/main/cpp/gpu_resident test/t668/driver.cpp /Fe.codex/local-test/t668-store.exe /Fo.codex/local-test/t668-store.obj
if errorlevel 1 exit /b 1
.codex\local-test\t668-store.exe full_queue_cancel
if errorlevel 1 exit /b 1
.codex\local-test\t668-store.exe fence_cancel
if errorlevel 1 exit /b 1
.codex\local-test\t668-store.exe deadline

if errorlevel 1 exit /b 1
.codex\local-test\t668-store.exe full_queue_pop
