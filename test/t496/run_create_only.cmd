@echo off
rem T547-compliant runner: manages ONLY this child PID, unique attempt, wall watchdog.
rem Usage: run_create_only.cmd <arm> <attemptId>
rem Previous defects fixed (T547 SS1): taskkill by image name killed ALL t496.exe
rem (including other sessions); window-title cleanup was unreliable; now the
rem watchdog is bound to this child's PID only and nothing else is touched.
setlocal enabledelayedexpansion
set "ARM=%1"
set "ATTEMPT=%2"
if "%ARM%"=="" (echo usage: run_create_only.cmd ARM ATTEMPTID & exit /b 1)
if "%ATTEMPT%"=="" (echo attemptId required & exit /b 1)
cd /d D:/hmprojects/HelloWorld
set "PATH=C:\Users\admin\AppData\Local\Programs\CokePLUS;%PATH%"
set "EXE=D:\hmprojects\HelloWorld\.codex\local-test\t496\t496.exe"
if exist ".codex\local-test\t496\child-pid-%ATTEMPT%.txt" del ".codex\local-test\t496\child-pid-%ATTEMPT%.txt"
powershell -NoProfile -Command "$p=Start-Process -FilePath '%EXE%' -ArgumentList '%ARM%','%ATTEMPT%' -WorkingDirectory 'D:/hmprojects/HelloWorld' -PassThru -RedirectStandardOutput 'D:/hmprojects/HelloWorld/.codex/local-test/t496/out-%ATTEMPT%-stdout.txt' -RedirectStandardError 'D:/hmprojects/HelloWorld/.codex/local-test/t496/out-%ATTEMPT%-stderr.txt'; $p.Id | Out-File -Encoding ascii 'D:/hmprojects/HelloWorld/.codex/local-test/t496/child-pid-%ATTEMPT%.txt'"
set /p CPID=<.codex\local-test\t496\child-pid-%ATTEMPT%.txt
echo CHILD_PID=%CPID% & echo LAUNCH_AT=%DATE% %TIME%
powershell -NoProfile -Command "$deadline=(Get-Date).AddSeconds(900); while((Get-Date) -lt $deadline -and (Get-Process -Id %CPID% -ErrorAction SilentlyContinue)){Start-Sleep -Milliseconds 500}; $p=Get-Process -Id %CPID% -ErrorAction SilentlyContinue; if($p){Stop-Process -Id %CPID% -Force; Write-Output ('WATCHDOG_KILL pid=%CPID% at='+(Get-Date -Format o))} else {Write-Output 'CHILD_EXITED_BEFORE_DEADLINE'}" > .codex\local-test\t496\watchdog-%ATTEMPT%.txt
type .codex\local-test\t496\watchdog-%ATTEMPT%.txt
endlocal
