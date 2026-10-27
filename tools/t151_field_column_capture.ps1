# T151 FIELD_COLUMN 原值真机补采：复现并捕获 V2 完整失败链
#   依据: T149 V2 诊断(e3ee81f) + T150 验证 12/12(d5c9513)
#   基线: 6ee7e19, s149 a59311eb, 树指纹 2f2c2263362ffda3
#   预期: first_code5_trace 携带 V2 原值（八分量 IEEE754 bits/badMask/三次左结合和/
#         最后合法→首次非法窗口/writerStage/PREEXISTING_IN_WINDOW 标记）
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t151-field-column-raw-20261027'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T151 capture start (build 6ee7e19, s149 a59311eb, tree 2f2c2263362ffda3)'
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
  & $hdc shell "snapshot_display -f /data/local/tmp/t151-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t151-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'first_code5_trace|demo_diag|empty_sand_demo|physics-failed|FAULT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}

# ---------- C1: 复现 FIELD_COLUMN ----------
Log-Step 'C1 reproduce FIELD_COLUMN (4x swipe + settle)'
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

# ---------- C2: 第二 run 对照 ----------
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
