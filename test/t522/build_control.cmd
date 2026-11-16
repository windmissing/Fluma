@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /DNOMINMAX /Ientry/src/main/cpp/gpu_resident test/t522/control.cpp entry/src/main/cpp/gpu_resident/m3a/progress_clock_v3.cpp entry/src/main/cpp/gpu_resident/m3a/root_owner.cpp entry/src/main/cpp/gpu_resident/m3a/planning_control.cpp entry/src/main/cpp/gpu_resident/m3a/completion_gate.cpp entry/src/main/cpp/gpu_resident/m3a/quota_snapshot_v3.cpp entry/src/main/cpp/gpu_resident/m3a/quota_reader_v3.cpp /Fe.codex/local-test/t522-control.exe /Fo.codex/local-test/
if errorlevel 1 exit /b 1
.codex\local-test\t522-control.exe
