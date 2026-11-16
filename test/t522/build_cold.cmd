@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /DNOMINMAX /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/gpu_resident test/t522/cold.cpp entry/src/main/cpp/gpu_resident/m3a/quota_reader_v3.cpp /Fe.codex/local-test/t522-cold.exe /Fo.codex/local-test/
if errorlevel 1 exit /b 1
.codex\local-test\t522-cold.exe
