@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /utf-8 /O2 /EHsc /std:c++17 /Ientry/src/main/cpp/gpu_resident test/t693/replay.cpp /Fe.codex/local-test/t693-replay.exe /Fo.codex/local-test/t693-replay.obj
