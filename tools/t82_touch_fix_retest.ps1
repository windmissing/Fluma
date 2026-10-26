# T82 触摸接线修复后 DEMO 复测：六臂真机采集
#   依据: docs/task-T82-touch-fix-retest-runbook-v1.md
#   基线: 114a7ee (T56 scene + T71 输入节点), s144, 树指纹 403b08514017b326
#   臂: R0 零水基线 / R1 按住发沙落床 / R2 拖动 / R3 停止边界 / R4 手持(手动提示) / R5 后台+新Down
# 注意: 前台执行；USB 弹窗先关；L7 必须截图像素证据，不得以 swap 代通过
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t82-touch-fix-retest-20261026'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T82 retest start (build 114a7ee, s144, tree 403b08514017b326, hap af049753c8dda7e169bcf0613f384b7affa3aa3cf1fb1fac7cd65ad64db31be8)'
& $hdc shell 'power-shell timeout -o 600000' | Out-Null

function Start-Demo($arm) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps emptysanddemo 1" | Out-Null
  Start-Sleep -Seconds 5
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null   # dismiss USB popup
  Start-Sleep -Seconds 2
  Log-Step "$arm demo started"
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/t82-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t82-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'demo_diag|empty_sand_demo|demo_gravity_config|FAULT|STALE|scene' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}

# ---------- R0: 零水冷启基线 ----------
Log-Step 'R0 dry cold-start baseline (W/H/L=0, input gate ready?)'
Start-Demo 'R0'
Start-Sleep -Seconds 12
Shot 'R0-idle'
Dump 'R0-full.txt'
Log-Step 'R0 done'

# ---------- R1: 单点按住发沙->落床->松手 ----------
Log-Step 'R1 single-point hold 3s -> release 8s'
Start-Demo 'R1'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 700 1200 700 1200 3000' | Out-Null
Start-Sleep -Seconds 1
Shot 'R1-holding'
Start-Sleep -Seconds 8
Shot 'R1-settled'
Dump 'R1-full.txt'
Log-Step 'R1 done'

# ---------- R2: 拖动注沙 ----------
Log-Step 'R2 drag injection (400,1200)->(850,1200) slow'
Start-Demo 'R2'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 400 1200 850 1200 2000' | Out-Null
Start-Sleep -Seconds 1
Shot 'R2-dragging'
Start-Sleep -Seconds 8
Shot 'R2-settled'
Dump 'R2-full.txt'
Log-Step 'R2 done'

# ---------- R3: 停止边界专项 ----------
Log-Step 'R3 stop boundary: hold 2s -> release -> burst shots'
Start-Demo 'R3'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 600 1500 600 1500 2000' | Out-Null
Start-Sleep -Seconds 0
& $hdc shell "snapshot_display -f /data/local/tmp/t82-R3-rel0.jpeg" | Out-Null
& $hdc file recv /data/local/tmp/t82-R3-rel0.jpeg (Join-Path $out 'R3-rel0.jpeg') | Out-Null
Start-Sleep -Seconds 1
Shot 'R3-rel1s'
Start-Sleep -Seconds 2
Shot 'R3-rel3s'
Start-Sleep -Seconds 5
Shot 'R3-rel8s'
Dump 'R3-full.txt'
Log-Step 'R3 done'

# ---------- R4: 手持重力（手动环节提示） ----------
Log-Step 'R4 manual handheld: NOT automated. User holds device tilted ~30deg, press-hold to pour, observe flow direction. Log captured before/after.'
Start-Demo 'R4'
Start-Sleep -Seconds 5
Shot 'R4-pre-manual'
Start-Sleep -Seconds 40   # 手动窗口：手持倾斜按住发沙
Shot 'R4-post-manual'
Dump 'R4-full.txt'
Log-Step 'R4 done (manual observation window elapsed)'

# ---------- R5: 后台+新Down ----------
Log-Step 'R5 background no-refill: drag -> Home -> back -> new Down required'
Start-Demo 'R5'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 500 1500 800 1500 1500' | Out-Null
Start-Sleep -Seconds 2
& $hdc shell 'uitest uiInput keyEvent Home' | Out-Null
Start-Sleep -Seconds 4
& $hdc shell "aa start -a EntryAbility -b $bundle" | Out-Null
Start-Sleep -Seconds 4
Shot 'R5-back-no-hold'
& $hdc shell 'uitest uiInput swipe 500 1500 800 1500 1500' | Out-Null
Start-Sleep -Seconds 3
Shot 'R5-new-down'
Dump 'R5-full.txt'
Log-Step 'R5 done'

& $hdc shell 'power-shell -r' | Out-Null
Write-Output 'DONE'
