# T95 DEMO 物理阻断后复测：R0-R4
#   依据: docs/task-T95-demo-unblock-retest-runbook-v1.md
#   基线: a419f5a (T166 waterTop<-EPS 修复 + T167 46/46), s150 f28ec28e, 树指纹 aee7374caa8ff7cb
#   主判: J2 code5 零复现 + LAND 长窗 + 手持重力 + 稳定性（PID 隔离）
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t95-demo-unblock-retest-20261027'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T95 retest start (build a419f5a, s150 f28ec28e, tree aee7374caa8ff7cb)'
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
  & $hdc shell "snapshot_display -f /data/local/tmp/t95-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t95-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'first_code5_trace|demo_diag|empty_sand_demo|physics-failed|FAULT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}
function Check-AlivePID($arm) {
  $pidLine = & $hdc shell "pidof $bundle" 2>$null
  $appPid = ($pidLine -split '\s+')[0]
  if ([string]::IsNullOrEmpty($appPid)) { Log-Step "$arm PROCESS DEAD (pidof empty)"; return }
  Log-Step "$arm app PID=$appPid"
  Shot "$arm-alive"
  & $hdc shell 'hilog -x' 2>$null | Select-String 'THREAD_BLOCK|APP_INPUT_BLOCK|appfreeze' | ForEach-Object { $_.Line } | Select-String -SimpleMatch "$appPid" | Out-File (Join-Path $out "$arm-freeze.txt") -Encoding utf8
  $n = (Get-Content (Join-Path $out "$arm-freeze.txt") -ErrorAction SilentlyContinue | Measure-Object -Line).Lines
  if ($n -gt 0) { Log-Step "$arm FREEZE for PID $appPid" } else { Log-Step "$arm alive: no freeze for PID $appPid" }
}

# ---------- R0 冒烟 ----------
Log-Step 'R0 smoke'
Start-Demo 'R0'
Start-Sleep -Seconds 15
Shot 'R0-idle'
Check-AlivePID 'R0'
Dump 'R0-full.txt'
Log-Step 'R0 done'

# ---------- R1 J2 复现臂：4 swipe + 45s 静置（LAND 长窗） ----------
Log-Step 'R1 J2: 4x swipe + 45s settle (LAND window)'
Start-Demo 'R1'
Start-Sleep -Seconds 4
for ($i=1; $i -le 4; $i++) {
  & $hdc shell 'uitest uiInput swipe 400 1200 850 1200 2000' | Out-Null
  Start-Sleep -Seconds 3
  Log-Step "R1 swipe $i done"
}
Shot 'R1-t0'
Start-Sleep -Seconds 10
Shot 'R1-t10'
Start-Sleep -Seconds 10
Shot 'R1-t20'
Start-Sleep -Seconds 10
Shot 'R1-t30'
Start-Sleep -Seconds 15
Shot 'R1-t45'
Check-AlivePID 'R1'
Dump 'R1-full.txt'
Log-Step 'R1 done'

# ---------- R2 J2 加压臂：6 swipe + 45s ----------
Log-Step 'R2 J2 stress: 6x swipe + 45s settle'
Start-Demo 'R2'
Start-Sleep -Seconds 4
for ($i=1; $i -le 6; $i++) {
  & $hdc shell 'uitest uiInput swipe 400 1200 850 1200 2000' | Out-Null
  Start-Sleep -Seconds 2
  Log-Step "R2 swipe $i done"
}
Shot 'R2-t0'
Start-Sleep -Seconds 15
Shot 'R2-t15'
Start-Sleep -Seconds 15
Shot 'R2-t30'
Start-Sleep -Seconds 15
Shot 'R2-t45'
Check-AlivePID 'R2'
Dump 'R2-full.txt'
Log-Step 'R2 done'

# ---------- R3 手持重力（手动窗口） ----------
Log-Step 'R3 manual handheld window (~60s): tilt device, press-hold to pour, observe flow'
Start-Demo 'R3'
Start-Sleep -Seconds 5
Shot 'R3-pre'
Start-Sleep -Seconds 60
Shot 'R3-post'
Dump 'R3-full.txt'
Log-Step 'R3 done (manual observation window elapsed)'

# ---------- R4 finish + 双流 ----------
Log-Step 'R4 finish -> CLOSED -> pull two streams'
& $hdc shell 'uitest uiInput click 70 140' | Out-Null
Start-Sleep -Seconds 8
Dump 'R4-post.txt'
$closed = Select-String -Path (Join-Path $out 'R4-post.txt') -Pattern 'P011_OBS_STORE_CLOSED' | Select-Object -First 1
if ($closed) { Log-Step "R4 CLOSED: $($closed.Line.Substring([Math]::Max(0,$closed.Line.Length-240)))" } else { Log-Step 'R4 CLOSED NOT FOUND (may be R3 page)' }
# R1 store 拉取（R1 主判臂）
$fpLine = Select-String -Path (Join-Path $out 'R1-full.txt') -Pattern 'P011_OBS_STORE ' | Select-Object -First 1
if ($fpLine -match '"filePath":"([^"]+)"') {
  $fp = $matches[1]
  Start-Sleep -Seconds 3
  & $hdc file recv $fp (Join-Path $out 'R1-store.ndjson') 2>&1 | Out-Null
  if (Test-Path (Join-Path $out 'R1-store.ndjson')) { Log-Step "R1 ledger $((Get-Item (Join-Path $out 'R1-store.ndjson')).Length)B" }
  $afp = $fp -replace '\.ndjson$', '-admissions.ndjson'
  & $hdc file recv $afp (Join-Path $out 'R1-admissions.ndjson') 2>&1 | Out-Null
  if (Test-Path (Join-Path $out 'R1-admissions.ndjson')) { Log-Step "R1 admissions $((Get-Item (Join-Path $out 'R1-admissions.ndjson')).Length)B" }
} else { Log-Step 'R1 store path not found' }
Log-Step 'R4 done'

& $hdc shell 'power-shell -r' | Out-Null
Write-Output 'DONE'
