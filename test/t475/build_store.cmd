@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /DNOMINMAX /Ientry/src/main/cpp/gpu_resident/program_cache test/t475/store_test.cpp /Fe.codex/local-test/t475-store.exe /Fo.codex/local-test/t475-store.obj
if errorlevel 1 exit /b 1
.codex\local-test\t475-store.exe %1
