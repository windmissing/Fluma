# T177 B3 段：force-stop 冷启 + 三姿态（修复 B2 残留 failed 状态）
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t177-real-handheld-20261027'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'B3 cold restart + 3 postures'
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps emptysanddemo 1" | Out-Null
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 2
Log-Step 'B3 demo cold-started (device flat, user ready)'
# 姿态1 左倾 + 微位移触摸
Log-Step 'B3 posture 1: tilt LEFT + touch'
& $hdc shell 'uitest uiInput swipe 700 1200 715 1205 2500' | Out-Null
Start-Sleep -Seconds 1
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B3-p1.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B3-p1.jpeg (Join-Path $out 'B3-p1-mid.jpeg') | Out-Null
Start-Sleep -Seconds 5
Log-Step 'B3 posture 1 done'
# 姿态2 回正
Log-Step 'B3 posture 2: upright'
Start-Sleep -Seconds 8
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B3-p2.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B3-p2.jpeg (Join-Path $out 'B3-p2.jpeg') | Out-Null
# 姿态3 右倾 + 触摸
Log-Step 'B3 posture 3: tilt RIGHT + touch'
& $hdc shell 'uitest uiInput swipe 700 1200 715 1205 2500' | Out-Null
Start-Sleep -Seconds 1
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B3-p3.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B3-p3.jpeg (Join-Path $out 'B3-p3-mid.jpeg') | Out-Null
Start-Sleep -Seconds 5
Log-Step 'B3 posture 3 done'
# 静置沉积
Start-Sleep -Seconds 12
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B3-settled.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B3-settled.jpeg (Join-Path $out 'B3-settled.jpeg') | Out-Null
& $hdc shell 'hilog -x' 2>$null | Select-String 'first_code5_trace|demo_diag|empty_sand_demo|physics-failed|FAULT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out 'B3-handheld.txt') -Encoding utf8
Log-Step 'B3 done'
Write-Output 'DONE'
