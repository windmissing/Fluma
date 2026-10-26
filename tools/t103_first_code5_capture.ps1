# T103 DEMO 首个 code5 设备补采
#   依据: T102 导出机制（failureTrace 默认启用，getLastError()==5 时一次性分片导出 WS_EVENT first_code5_trace）
#   基线: c0c58c3 (T96 池优化 + T102 code5 诊断), s145, 树指纹 926b87fce4e7842b
#   复现路径: T82 R2 同款——干场 DEMO + 拖动注入 + 粒子沉降触底（设备平放 USB 张力→tilt 30°）
#   臂: C1 复现首 code5（触发即导出）/ C2 第二次冷启验证重置重布防（可选）
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t103-first-code5-capture-20261026'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T103 capture start (build c0c58c3, s145 553507fc, tree 926b87fce4e7842b)'
& $hdc shell 'power-shell timeout -o 600000' | Out-Null

function Start-Demo($arm) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps emptysanddemo 1" | Out-Null
  Start-Sleep -Seconds 5
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null
  Start-Sleep -Seconds 2
  Log-Step "$arm started"
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/t103-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t103-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'first_code5_trace|demo_diag|empty_sand_demo|FAULT|physics-failed|pool_progress' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}

# ---------- C1: 复现首 code5 ----------
Log-Step 'C1 reproduce first code5 (drag inject x3 + settle)'
Start-Demo 'C1'
Start-Sleep -Seconds 4
# T82 R2 触发序列: 拖动注入让粒子沉降触底（tilt 30° 由设备姿态自然给）
for ($i=1; $i -le 4; $i++) {
  & $hdc shell 'uitest uiInput swipe 400 1200 850 1200 2000' | Out-Null
  Start-Sleep -Seconds 3
  Log-Step "C1 swipe $i done"
  Shot "C1-after-swipe-$i"
}
# 等待沉降触底触发 code5（T82 在注入后约 6-8s 触发）
Start-Sleep -Seconds 15
Shot 'C1-settled'
Dump 'C1-full.txt'
Log-Step 'C1 done'

# ---------- C2: 新 run 重布防验证 ----------
Log-Step 'C2 fresh run re-arm (reset button + repeat)'
Start-Demo 'C2'
Start-Sleep -Seconds 4
& $hdc shell 'uitest dumpLayout -p /data/local/tmp/t103-layout.json' | Out-Null
& $hdc file recv /data/local/tmp/t103-layout.json (Join-Path $out 'C2-layout.json') 2>&1 | Out-Null
for ($i=1; $i -le 4; $i++) {
  & $hdc shell 'uitest uiInput swipe 400 1200 850 1200 2000' | Out-Null
  Start-Sleep -Seconds 3
}
Start-Sleep -Seconds 15
Shot 'C2-settled'
Dump 'C2-full.txt'
Log-Step 'C2 done'

& $hdc shell 'power-shell -r' | Out-Null
Write-Output 'DONE'
