# T177 B 段重跑：小位移 swipe 触摸（修复 0 位移不派发问题）
#   用户配合同款三姿态；脚本用 (700,1200)->(715,1205) 微位移保证 DOWN/MOVE/UP 派发
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t177-real-handheld-20261027'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'B rerun: small-displacement touch'
& $hdc shell 'hilog -r' | Out-Null
# 复位到空场（重力 reset）并保持页面
& $hdc shell "aa start -a EntryAbility -b $bundle" | Out-Null
Start-Sleep -Seconds 3
# 姿态1 左倾 + 微位移触摸发沙
Log-Step 'B2 posture 1: tilt LEFT + touch (small disp)'
& $hdc shell 'uitest uiInput swipe 700 1200 715 1205 2500' | Out-Null
Start-Sleep -Seconds 1
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B2-p1.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B2-p1.jpeg (Join-Path $out 'B2-p1-mid.jpeg') | Out-Null
Start-Sleep -Seconds 5
Log-Step 'B2 posture 1 done'
# 姿态2 回正
Log-Step 'B2 posture 2: upright'
Start-Sleep -Seconds 8
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B2-p2.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B2-p2.jpeg (Join-Path $out 'B2-p2.jpeg') | Out-Null
# 姿态3 右倾
Log-Step 'B2 posture 3: tilt RIGHT + touch'
& $hdc shell 'uitest uiInput swipe 700 1200 715 1205 2500' | Out-Null
Start-Sleep -Seconds 1
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B2-p3.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B2-p3.jpeg (Join-Path $out 'B2-p3-mid.jpeg') | Out-Null
Start-Sleep -Seconds 5
Log-Step 'B2 posture 3 done'
# 回正沉积
Start-Sleep -Seconds 10
& $hdc shell "snapshot_display -f /data/local/tmp/t177-B2-settled.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t177-B2-settled.jpeg (Join-Path $out 'B2-settled.jpeg') | Out-Null
& $hdc shell 'hilog -x' 2>$null | Select-String 'first_code5_trace|demo_diag|empty_sand_demo|physics-failed|FAULT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out 'B2-handheld.txt') -Encoding utf8
Log-Step 'B2 done'
Write-Output 'DONE'
