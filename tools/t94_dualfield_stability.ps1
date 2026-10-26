# T94 双场卡死修复后复测：R0-R4
#   依据: docs/task-T94-dualfield-stability-runbook-v1.md
#   基线: 52a0700 (T96 优化+T97 7/7), s146, abc=165f5868 与 s145 等价
#   主判: 三臂 T78 卡死序列不冻结 + 真实准入 + 双流 CLOSED
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t94-dualfield-stability-20261026'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T94 stability start (build 52a0700, s146 6350fc8b, abc 165f5868)'
& $hdc shell 'power-shell timeout -o 600000' | Out-Null

function Start-WS($arm, $shake) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps testmode true --ps p011particle 1 --ps p011capture 1 --ps wssnapshot false --ps shake $shake" | Out-Null
  Start-Sleep -Seconds 6
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null
  Start-Sleep -Seconds 2
  Log-Step "$arm started shake=$shake"
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/t94-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t94-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Out-File (Join-Path $out $file) -Encoding utf8
}
function Burst($tag, $count, $gapSec) {
  for ($i=1; $i -le $count; $i++) { Shot "$tag-$i"; Start-Sleep -Seconds $gapSec }
}
function Check-Alive($arm) {
  Shot "$arm-alive"
  & $hdc shell 'hilog -x' 2>$null | Select-String 'THREAD_BLOCK|APP_INPUT_BLOCK|appfreeze' | ForEach-Object { $_.Line } | Out-File (Join-Path $out "$arm-freeze.txt") -Encoding utf8
  $n = (Get-Content (Join-Path $out "$arm-freeze.txt") | Measure-Object -Line).Lines
  if ($n -gt 0) { Log-Step "$arm FREEZE detected ($n lines)" } else { Log-Step "$arm alive, no freeze keywords" }
}

# ---------- R0 冒烟 ----------
Log-Step 'R0 smoke'
Start-WS 'R0' '0'
Start-Sleep -Seconds 15
Shot 'R0-idle'
Check-Alive 'R0'
Dump 'R0-full.txt'
Log-Step 'R0 done'

# ---------- R1 T78 原卡死序列 ----------
Log-Step 'R1 shake=1 + 3x swipe (T78 R1 sequence)'
Start-WS 'R1' '1'
Start-Sleep -Seconds 4
for ($i=1; $i -le 3; $i++) {
  & $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
  Start-Sleep -Seconds 2
  Shot "R1-swipe-$i"
  Log-Step "R1 swipe $i done, alive check"
}
Start-Sleep -Seconds 25
Burst 'R1-settle' 3 5
Check-Alive 'R1'
Dump 'R1-full.txt'
Log-Step 'R1 done'

# ---------- R2 ----------
Log-Step 'R2 shake=0.6 + 1x swipe (T78 R2 sequence)'
Start-WS 'R2' '0.6'
Start-Sleep -Seconds 4
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
Start-Sleep -Seconds 15
Burst 'R2-settle' 2 5
Check-Alive 'R2'
Dump 'R2-full.txt'
Log-Step 'R2 done'

# ---------- R3 ----------
Log-Step 'R3 shake=0 + 2x swipe + 25s deposition (T78 R3 sequence extended)'
Start-WS 'R3' '0'
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
Start-Sleep -Seconds 2
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
Shot 'R3-t0'
Start-Sleep -Seconds 5
Shot 'R3-t5'
Start-Sleep -Seconds 5
Shot 'R3-t10'
Start-Sleep -Seconds 5
Shot 'R3-t15'
Start-Sleep -Seconds 5
Shot 'R3-t20'
Start-Sleep -Seconds 5
Shot 'R3-t25'
Check-Alive 'R3'
Dump 'R3-full.txt'
Log-Step 'R3 done'

# ---------- R4 前台 finish + 双流 ----------
Log-Step 'R4 panel finish button -> CLOSED -> pull two streams'
& $hdc shell 'uitest uiInput click 70 140' | Out-Null
Start-Sleep -Seconds 8
Dump 'R4-post.txt'
$closed = Select-String -Path (Join-Path $out 'R4-post.txt') -Pattern 'P011_OBS_STORE_CLOSED' | Select-Object -First 1
if ($closed) { Log-Step "R4 CLOSED: $($closed.Line.Substring([Math]::Max(0,$closed.Line.Length-240)))" } else { Log-Step 'R4 CLOSED NOT FOUND' }
$fpLine = Select-String -Path (Join-Path $out 'R3-full.txt') -Pattern 'P011_OBS_STORE ' | Select-Object -First 1
if ($fpLine -match '"filePath":"([^"]+)"') {
  $fp = $matches[1]
  & $hdc file recv $fp (Join-Path $out 'R3-store.ndjson') 2>&1 | Out-Null
  $afp = $fp -replace '\.ndjson$', '-admissions.ndjson'
  & $hdc file recv $afp (Join-Path $out 'R3-admissions.ndjson') 2>&1 | Out-Null
  $ll = if (Test-Path (Join-Path $out 'R3-store.ndjson')) { (Get-Item (Join-Path $out 'R3-store.ndjson')).Length } else { 0 }
  $al = if (Test-Path (Join-Path $out 'R3-admissions.ndjson')) { (Get-Item (Join-Path $out 'R3-admissions.ndjson')).Length } else { 0 }
  Log-Step "R4 pulled ledger=$ll B admissions=$al B"
  $py = 'C:\Users\admin\.workbuddy\binaries\python\versions\3.13.12\python.exe'
  & $py 'D:\hmprojects\HelloWorld\tools\p011_capture_manifest.py' (Join-Path $out 'R3-store.ndjson') (Join-Path $out 'R3-admissions.ndjson') (Join-Path $out 'fence.json') (Join-Path $out 'manifest.json') 2>&1 | Select-Object -Last 4 | ForEach-Object { Log-Step "manifest: $_" }
} else { Log-Step 'R4 store path not found' }
Log-Step 'R4 done'

& $hdc shell 'power-shell -r' | Out-Null
Write-Output 'DONE'
