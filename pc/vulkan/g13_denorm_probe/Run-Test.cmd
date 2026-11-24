@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Run-Test.ps1" %*
set "test_exit=%errorlevel%"
if "%~1"=="" pause
exit /b %test_exit%
