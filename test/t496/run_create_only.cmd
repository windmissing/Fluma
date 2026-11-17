@echo off
rem T496 CREATE_ONLY 三臂驱动（T518 §3）：每臂独立新进程+新输出目录，
rem 外部 watchdog 900s 硬杀（timeout /kill），约 30s 心跳由 CREATE_WAIT 自带。
rem 用法：run_create_only.cmd C0|C1|C2
setlocal
set ARM=%1
if "%ARM%"=="" (echo usage: run_create_only.cmd C0^|C1^|C2 & exit /b 1)
cd /d D:/hmprojects/HelloWorld
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
rem 有界 attempt：每臂只跑一次，900s watchdog 到点杀进程并留收据
start /b "" cmd /c "timeout /t 900 /nobreak >nul & taskkill /f /im t496.exe >nul 2>&1 & echo WATCHDOG_KILL_900S %ARM% >> .codex\local-test\t496\watchdog-%ARM%.txt"
codex\local-test\t496\t496.exe %ARM%
set RC=%errorlevel%
taskkill /f /fi "WINDOWTITLE eq timeout*" >nul 2>&1
echo CREATE_ONLY %ARM% exit=%RC%
exit /b %RC%
