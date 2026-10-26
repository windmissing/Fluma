# T78 双场采集修复后复测：三臂真机采集
#   依据: docs/task-T78-dualfield-retest-runbook-v1.md
#   基线: 114a7ee (T56 scene + T66 收尾), s144（同 T82 已装机）
#   臂: R0 冒烟 / R1 前台按钮 finish / R2 Want finish / R3 落床时间序列
# 判据: 双流 CLOSED+manifest COMPLETE+截图像素可见沙+deposited 时间序列；code5 记 invalid
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\t78-dualfield-retest-20261026'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Log-Step($msg) { $t = Get-Date -Format 'HH:mm:ss'; Write-Output "[$t] $msg"; Add-Content -Path (Join-Path $out 'run-console.log') -Value "[$t] $msg" }
Log-Step 'T78 retest start (build 114a7ee, s144 af049753, tree 403b08514017b326)'
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
  & $hdc shell "snapshot_display -f /data/local/tmp/t78-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/t78-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_METRIC|WS_FRAME|WS_EVENT|P011_OBS_STORE|P011_OBS_ERROR|P011_RASTER_LAYOUT|particle_route|LiquidCA want|FAULT|STALE' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Encoding utf8
}
function Get-RunId($tag) {
  $dump = Join-Path $out "$tag-pre.txt"
  $line = Select-String -Path $dump -Pattern 'P011_OBS_STORE ' | Select-Object -First 1
  if ($line -match '"runId":"([^"]+)"') { return $matches[1] }
  $line2 = Select-String -Path $dump -Pattern 'particle_route' | Select-Object -First 1
  return $null
}
function Get-Generation($tag) {
  # generation 取自 store 事件（T66 storeGeneration）——hilog 若无则用 0（第一代）
  return 0
}
function Pull-Two($tag, $fp) {
  & $hdc file recv $fp (Join-Path $out "$tag-store.ndjson") 2>&1 | Out-Null
  if (Test-Path (Join-Path $out "$tag-store.ndjson")) { Log-Step "$tag ledger $((Get-Item (Join-Path $out "$tag-store.ndjson")).Length)B" }
  $afp = $fp -replace '\.ndjson$', '-admissions.ndjson'
  & $hdc file recv $afp (Join-Path $out "$tag-admissions.ndjson") 2>&1 | Out-Null
  if (Test-Path (Join-Path $out "$tag-admissions.ndjson")) { Log-Step "$tag admissions $((Get-Item (Join-Path $out "$tag-admissions.ndjson")).Length)B" }
}
function Manifest($tag) {
  $py = 'C:\Users\admin\.workbuddy\binaries\python\versions\3.13.12\python.exe'
  & $py tools/p011_capture_manifest.py (Join-Path $out "$tag-store.ndjson") (Join-Path $out "$tag-admissions.ndjson") (Join-Path $out "$tag-fence.json") (Join-Path $out "$tag-manifest.json") 2>&1 | Select-Object -Last 5 | ForEach-Object { Log-Step "$tag manifest: $_" }
}

# ---------- R0: 冒烟 ----------
Log-Step 'R0 smoke'
Start-WS 'R0' '0'
Start-Sleep -Seconds 12
Shot 'R0-idle'
Dump 'R0-full.txt'
Log-Step 'R0 done (no finish; next arms use fresh runs)'

# ---------- R1: 注入+摇晃+前台按钮 finish ----------
Log-Step 'R1 shake+inject -> panel finish button'
Start-WS 'R1' '1'
Start-Sleep -Seconds 4
for ($i=1; $i -le 3; $i++) {
  & $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
  Start-Sleep -Seconds 2
}
Shot 'R1-shaking'
Start-Sleep -Seconds 25
Shot 'R1-settled-25s'
Dump 'R1-pre.txt'
# 前台"结束采集"按钮 position(20,112)（屏幕坐标即点 20+按钮宽心，取 70,140 保守）
& $hdc shell 'uitest uiInput click 70 140' | Out-Null
Start-Sleep -Seconds 6
Dump 'R1-post.txt'
$closed = Select-String -Path (Join-Path $out 'R1-post.txt') -Pattern 'P011_OBS_STORE_CLOSED' | Select-Object -First 1
if ($closed) { Log-Step "R1 CLOSED: $($closed.Line.Substring([Math]::Max(0,$closed.Line.Length-260)))" } else { Log-Step 'R1 CLOSED NOT FOUND' }
$fpLine = Select-String -Path (Join-Path $out 'R1-pre.txt') -Pattern 'P011_OBS_STORE ' | Select-Object -First 1
if ($fpLine -match '"filePath":"([^"]+)"') { Pull-Two 'R1' $matches[1]; Manifest 'R1' } else { Log-Step 'R1 store path not found' }
Log-Step 'R1 done'

# ---------- R2: Want finish ----------
Log-Step 'R2 want finish path'
Start-WS 'R2' '0.6'
Start-Sleep -Seconds 4
& $hdc shell 'uitest uiInput swipe 100 1200 1200 1200 600' | Out-Null
Start-Sleep -Seconds 15
Shot 'R2-settled'
Dump 'R2-pre.txt'
$runId = Get-RunId 'R2'
$gen = Get-Generation 'R2'
Log-Step "R2 runId=$runId gen=$gen"
& $hdc shell "aa start -a EntryAbility -b $bundle --ps p011finish T78R2 --ps p011finishrun $runId --ps p011finishgeneration $gen" | Out-Null
Start-Sleep -Seconds 6
Dump 'R2-post.txt'
$closed2 = Select-String -Path (Join-Path $out 'R2-post.txt') -Pattern 'P011_OBS_STORE_CLOSED' | Select-Object -First 1
if ($closed2) { Log-Step "R2 CLOSED: $($closed2.Line.Substring([Math]::Max(0,$closed2.Line.Length-260)))" } else { Log-Step 'R2 CLOSED NOT FOUND' }
$fpLine2 = Select-String -Path (Join-Path $out 'R2-pre.txt') -Pattern 'P011_OBS_STORE ' | Select-Object -First 1
if ($fpLine2 -match '"filePath":"([^"]+)"') { Pull-Two 'R2' $matches[1]; Manifest 'R2' } else { Log-Step 'R2 store path not found' }
Log-Step 'R2 done'

# ---------- R3: 落床时间序列 ----------
Log-Step 'R3 deposition timeline: inject x2 then 0/5/10/20/25s'
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
Dump 'R3-mid.txt'
Start-Sleep -Seconds 10
Shot 'R3-t20'
Start-Sleep -Seconds 5
Shot 'R3-t25'
Dump 'R3-post.txt'
$fpLine3 = Select-String -Path (Join-Path $out 'R3-post.txt') -Pattern 'P011_OBS_STORE ' | Select-Object -First 1
if ($fpLine3 -match '"filePath":"([^"]+)"') { Pull-Two 'R3' $matches[1] }
Log-Step 'R3 done (metrics timeline from mid/post dumps; no finish to keep run short)'

& $hdc shell 'power-shell -r' | Out-Null
Write-Output 'DONE'
