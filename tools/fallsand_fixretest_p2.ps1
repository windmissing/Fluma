# P-011/P-012 fallsand fix retest - part 2: long-injection stability + V1-V4 re-record
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\p012-fallsand-fixretest-20261025'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $out | Out-Null

function Start-App($extra) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true $extra" | Out-Null
  Start-Sleep -Seconds 5
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null
  Start-Sleep -Seconds 3
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/r2-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/r2-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump-Events($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_EVENT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}
function Dump-Metrics($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_METRIC' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}
function Last-Metric {
  $m = & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_METRIC' | Select-Object -Last 1
  if ($m) { return $m.Line } else { return '' }
}

# ================= PART C: long injection stability (90s) =================
Start-App ""
Start-Sleep -Seconds 12
$dir = Join-Path $out 'frames-longinj'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$end = (Get-Date).AddSeconds(90)
$i = 0; $tick = 0
while ((Get-Date) -lt $end) {
  if ($tick % 2 -eq 0) {
    & $hdc shell 'uitest uiInput swipe 300 1600 900 1600 600' | Out-Null
  } else {
    & $hdc shell 'uitest uiInput swipe 900 1600 300 1600 600' | Out-Null
  }
  & $hdc shell 'snapshot_display -f /data/local/tmp/r2-longinj.jpeg' | Out-Null
  & $hdc file recv /data/local/tmp/r2-longinj.jpeg (Join-Path $dir ("f-{0:d4}.jpeg" -f $i)) | Out-Null
  if ($tick % 3 -eq 0) {
    Dump-Metrics 'C-longinj-metric-series.txt'
    Set-Content -Path (Join-Path $out 'C-longinj-metric-tick.txt') -Value ((Get-Date).ToString('HH:mm:ss.fff') + "`t" + (Last-Metric))
  }
  $i++; $tick++
}
Write-Output ("PART C longinj frames: " + $i)
& $hdc shell 'hilog -x' 2>$null | Select-String 'WS_FRAME' | ForEach-Object { $_.Line } | Out-File (Join-Path $out 'C-longinj-wsframe.txt')
& $hdc shell 'hilog -x' 2>$null | Select-String 'WS_EVENT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out 'C-longinj-gesture_sand.txt')
& $hdc shell 'hilog -x' 2>$null | Select-String -Pattern 'FATAL|ANR|crash|Exception|died|SIGSEGV|SIGABRT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out 'C-longinj-errors.txt')
Set-Content -Path (Join-Path $out 'C-longinj-metric-last.txt') -Value (Last-Metric)
Shot 'C-longinj-final'
Write-Output "PART C done"

# ================= PART D: V1-V4 re-record =================
# V1 horizontal sweep across the water body
Start-App ""
Start-Sleep -Seconds 20
$dir = Join-Path $out 'frames-v1'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell 'uitest uiInput swipe 300 1600 900 1600 600' | Out-Null
$end = (Get-Date).AddSeconds(15); $i = 0
while ((Get-Date) -lt $end) {
  & $hdc shell 'snapshot_display -f /data/local/tmp/r2-v1.jpeg' | Out-Null
  & $hdc file recv /data/local/tmp/r2-v1.jpeg (Join-Path $dir ("f-{0:d4}.jpeg" -f $i)) | Out-Null
  $i++; Start-Sleep -Milliseconds 230
}
Dump-Events 'D-events-v1.txt'
Set-Content -Path (Join-Path $out 'D-metric-v1.txt') -Value (Last-Metric)
Write-Output ("PART D v1 frames: " + $i)

# V2 tilted 30 deg, same gesture
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true --ps wstilt 30" | Out-Null
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 22
$dir = Join-Path $out 'frames-v2'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
& $hdc shell 'uitest uiInput swipe 300 1600 900 1600 600' | Out-Null
$end = (Get-Date).AddSeconds(15); $i = 0
while ((Get-Date) -lt $end) {
  & $hdc shell 'snapshot_display -f /data/local/tmp/r2-v2.jpeg' | Out-Null
  & $hdc file recv /data/local/tmp/r2-v2.jpeg (Join-Path $dir ("f-{0:d4}.jpeg" -f $i)) | Out-Null
  $i++; Start-Sleep -Milliseconds 230
}
Dump-Events 'D-events-v2.txt'
Set-Content -Path (Join-Path $out 'D-metric-v2.txt') -Value (Last-Metric)
Write-Output ("PART D v2 frames: " + $i)

# V3 repeated injection at one point until capacity rejects
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true" | Out-Null
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 20
$dir = Join-Path $out 'frames-v3'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
for ($k = 1; $k -le 12; $k++) {
  & $hdc shell 'uitest uiInput swipe 640 1600 644 1600 300' | Out-Null
  Start-Sleep -Milliseconds 400
}
$end = (Get-Date).AddSeconds(15); $i = 0
while ((Get-Date) -lt $end) {
  & $hdc shell 'snapshot_display -f /data/local/tmp/r2-v3.jpeg' | Out-Null
  & $hdc file recv /data/local/tmp/r2-v3.jpeg (Join-Path $dir ("f-{0:d4}.jpeg" -f $i)) | Out-Null
  $i++; Start-Sleep -Milliseconds 230
}
Dump-Events 'D-events-v3.txt'
Set-Content -Path (Join-Path $out 'D-metric-v3.txt') -Value (Last-Metric)
Write-Output ("PART D v3 frames: " + $i)

# V4 sweep near the sand surface
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true" | Out-Null
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
Start-Sleep -Seconds 26
$dir = Join-Path $out 'frames-v4'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
& $hdc shell 'uitest uiInput swipe 300 1850 900 1850 600' | Out-Null
$end = (Get-Date).AddSeconds(15); $i = 0
while ((Get-Date) -lt $end) {
  & $hdc shell 'snapshot_display -f /data/local/tmp/r2-v4.jpeg' | Out-Null
  & $hdc file recv /data/local/tmp/r2-v4.jpeg (Join-Path $dir ("f-{0:d4}.jpeg" -f $i)) | Out-Null
  $i++; Start-Sleep -Milliseconds 230
}
Dump-Events 'D-events-v4.txt'
Set-Content -Path (Join-Path $out 'D-metric-v4.txt') -Value (Last-Metric)
Write-Output ("PART D v4 frames: " + $i)
Write-Output "DONE"
