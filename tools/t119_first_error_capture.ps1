# T119 设备首错误原码补采：复现 COLLECT99 并捕获 firstFailure.origin
#   依据: T117 诊断（P011_COLLECT_FIRST_FAILURE_V1，2c024c8）+ T103 触发序列
#   基线: 2c024c8, s147 707d7213, 树指纹 cd9401cd54661870
#   预期: first_code5_trace 内 traces[].collectFailure{phase,rawCode,...} + firstFailure{origin,...}
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t119-first-error-capture-20261026'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T119 capture start (build 2c024c8, s147 707d7213, tree cd9401cd54661870)'
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
  & $hdc shell "snapshot_display -f /data/local/tmp/t119-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t119-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'first_code5_trace|demo_diag|empty_sand_demo|physics-failed|FAULT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}

# ---------- C1: 复现 COLLECT99（T103 同款 4 swipe 序列） ----------
Log-Step 'C1 reproduce COLLECT99'
Start-Demo 'C1'
Start-Sleep -Seconds 4
for ($i=1; $i -le 4; $i++) {
  & $hdc shell 'uitest uiInput swipe 400 1200 850 1200 2000' | Out-Null
  Start-Sleep -Seconds 3
  Log-Step "C1 swipe $i done"
}
Start-Sleep -Seconds 15
Shot 'C1-settled'
Dump 'C1-full.txt'
Log-Step 'C1 done'

# ---------- C2: 第二 run（对照 + 双子步前成后败复现窗口） ----------
Log-Step 'C2 second run'
Start-Demo 'C2'
Start-Sleep -Seconds 4
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
