@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
set "OUT=.codex/local-test/t375-host"
if not exist "%OUT%" mkdir "%OUT%"
python -X utf8 -c "from pathlib import Path;p=Path('test/t336/build.cmd').read_text();Path('.codex/local-test/t375-host/build.cmd').write_text(p.replace('.codex/local-test/t336/', '.codex/local-test/t375-host/'))"
if not "%errorlevel%"=="0" exit /b %errorlevel%
call "%OUT%/build.cmd"
if not "%errorlevel%"=="0" exit /b %errorlevel%
"%OUT%/partial.exe" 192 64
if not "%errorlevel%"=="0" exit /b %errorlevel%
"%OUT%/partial.exe" 512 128
if not "%errorlevel%"=="0" exit /b %errorlevel%
"%OUT%/run_contract.exe"
if not "%errorlevel%"=="0" exit /b %errorlevel%
"%OUT%/bridge.exe"
if not "%errorlevel%"=="0" exit /b %errorlevel%
"%OUT%/app.exe"
exit /b %errorlevel%
