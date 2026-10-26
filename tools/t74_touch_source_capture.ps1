# T74 触摸零源设备补采：五臂事件链取证
#   依据: docs/task-T74-touch-source-device-capture-runbook-v1.md
#   臂: R0 冷启冒烟 / R1 单点按住 / R2 中心拖动 / R3 右壁 / R4 reset 再触摸
#   判读: demo_diag 事件链 L1 RECEIVE -> L2 MAP/START -> L3 CLOCK -> L4 REQUEST -> L5 RECEIPT -> L6 PUBLICATION -> L7 RENDER
# 注意: 前台执行；USB 弹窗先关；emptysanddemo 1 入口；swipe 同点=按住无Move
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t74-touch-source-capture-20261026'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T74 device capture start (build 545f45e, s143, tree 5b34bc45ce9cc5b7, hap babc44c65f8fae41daecfd8f911b0bb53d01261a12d6e9c11bb1395061c9d440)'
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
  & $hdc shell "snapshot_display -f /data/local/tmp/t74-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t74-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'demo_diag|empty_sand_demo|demo_gravity_config|EmptySand|FAULT|p013renderer|submitDualField' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}

# ---------- R0: 冷启冒烟 ----------
Log-Step 'R0 smoke (no touch, diagnostics alive?)'
Start-Demo 'R0'
Start-Sleep -Seconds 15
Shot 'R0-idle'
Dump 'R0-full.txt'
Log-Step 'R0 done'

# ---------- R1: 单点按住 3s（同点 swipe = 无 Move 的持握） ----------
Log-Step 'R1 single-point hold 3s at (700,1200)'
Start-Demo 'R1'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 700 1200 700 1200 3000' | Out-Null
Start-Sleep -Seconds 1
Shot 'R1-holding'
Start-Sleep -Seconds 5
Shot 'R1-after'
Dump 'R1-full.txt'
Log-Step 'R1 done'

# ---------- R2: 中心拖动（T63 中心滑动同款 local 目标） ----------
Log-Step 'R2 center drag (400,1200)->(850,1200)'
Start-Demo 'R2'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 400 1200 850 1200 600' | Out-Null
Start-Sleep -Seconds 1
Shot 'R2-dragging'
Start-Sleep -Seconds 6
Shot 'R2-after'
Dump 'R2-full.txt'
Log-Step 'R2 done'

# ---------- R3: 右壁动作 ----------
Log-Step 'R3 right-wall drag (1160,1200)->(1220,1700)'
Start-Demo 'R3'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 1160 1200 1220 1700 600' | Out-Null
Start-Sleep -Seconds 1
Shot 'R3-dragging'
Start-Sleep -Seconds 6
Shot 'R3-after'
Dump 'R3-full.txt'
Log-Step 'R3 done'

# ---------- R4: reset 后再触摸 ----------
Log-Step 'R4 reset-then-touch (non-empty reset evidence)'
Start-Demo 'R4'
Start-Sleep -Seconds 3
& $hdc shell 'uitest uiInput swipe 400 1200 850 1200 600' | Out-Null
Start-Sleep -Seconds 8        # 粒子落床
Shot 'R4-bedded'
# 重置按钮：页面顶部 "重置为空场" Button（先 dumpLayout 找不到就用估算坐标 300,260）
& $hdc shell 'uitest dumpLayout -p /data/local/tmp/t74-layout.json' | Out-Null
& $hdc file recv /data/local/tmp/t74-layout.json (Join-Path $out 'R4-layout.json') | Out-Null
& $hdc shell 'uitest uiInput click 300 260' | Out-Null
Start-Sleep -Seconds 3
& $hdc shell 'uitest uiInput swipe 400 1200 850 1200 600' | Out-Null
Start-Sleep -Seconds 6
Shot 'R4-after-reset'
Dump 'R4-full.txt'
Log-Step 'R4 done'

& $hdc shell 'power-shell -r' | Out-Null
Write-Output 'DONE'
