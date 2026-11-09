@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist .codex\local-test\t409-host mkdir .codex\local-test\t409-host
python -X utf8 test/t409/prepare_u64.py
if errorlevel 1 exit /b 1
cl /nologo /utf-8 /std:c++17 /EHsc /O2 .codex/local-test/t409-host/u64.cpp /Fe.codex/local-test/t409-host/u64.exe /Fo.codex/local-test/t409-host/
if errorlevel 1 exit /b 1
.codex\local-test\t409-host\u64.exe
exit /b %errorlevel%
