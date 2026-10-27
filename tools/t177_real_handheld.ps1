# T177 A 段：传感器→重力向量链静置基线 + 手持窗口等待
#   A 段脚本独立完成；B 段（真实手持）由用户配合，脚本在窗口期持续采样
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t177-real-handheld-20261027'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T177 capture start (s150 f28ec28e in place from T95)'
& $hdc shell 'power-shell timeout -o 600000' | Out-Null

function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'first_code5_trace|demo_diag|empty_sand_demo|physics-failed|FAULT|demo_gravity_config' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/t177-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t177-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}

# ---------- A 段：静置基线 ----------
Log-Step 'A: static baseline (sensor->tilt chain)'
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps emptysanddemo 1" | Out-Null
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 3
Log-Step 'A demo started (device flat on desk)'
Start-Sleep -Seconds 12
Dump 'A-static.txt'
Shot 'A-static'
Log-Step 'A done'

# ---------- B 段：手持窗口（用户配合；脚本持续采样+定时截图+自动触摸注入辅助） ----------
Log-Step 'B: handheld window 120s - user tilts device; script auto-touches center periodically'
& $hdc shell 'hilog -r' | Out-Null
Start-Sleep -Seconds 3
# 姿态1 左倾 ~8s：脚本在中央自动短触摸（触发发沙），用户同时倾斜
Log-Step 'B posture 1: tilt LEFT, script touches'
& $hdc shell 'uitest uiInput swipe 700 1200 700 1200 3000' | Out-Null
Start-Sleep -Seconds 1
Shot 'B-p1-mid'
Start-Sleep -Seconds 6
Log-Step 'B posture 1 done'
# 姿态2 回正
Log-Step 'B posture 2: back to upright'
Start-Sleep -Seconds 8
Shot 'B-p2'
# 姿态3 右倾
Log-Step 'B posture 3: tilt RIGHT, script touches'
& $hdc shell 'uitest uiInput swipe 700 1200 700 1200 3000' | Out-Null
Start-Sleep -Seconds 1
Shot 'B-p3-mid'
Start-Sleep -Seconds 6
Log-Step 'B posture 3 done'
# 回正沉积
Start-Sleep -Seconds 10
Shot 'B-settled'
Dump 'B-handheld.txt'
Log-Step 'B done'

& $hdc shell 'power-shell -r' | Out-Null
Write-Output 'DONE'
