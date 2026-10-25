# P-011/P-012 fallsand fix retest - part 1: gesture_sand per-transaction evidence + shake path
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
  & $hdc shell "snapshot_display -f /data/local/tmp/rt-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/rt-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump-Events($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_EVENT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}
function Dump-MetricSeries($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_METRIC' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}
function Last-Metric {
  $m = & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_METRIC' | Select-Object -Last 1
  if ($m) { return $m.Line } else { return '' }
}

# ================= PART A: gesture_sand per-transaction evidence =================
Start-App ""
Start-Sleep -Seconds 30
Shot 'A-00-baseline'
Set-Content -Path (Join-Path $out 'A-metric-baseline.txt') -Value (Last-Metric)

$cands = @(
  @{ n = 'swipe-v600';    a = 'uitest uiInput swipe 300 1600 900 1600 600' },
  @{ n = 'fling-v200-s6'; a = 'uitest uiInput fling 300 1600 900 1600 200 6' },
  @{ n = 'swipe-v200';    a = 'uitest uiInput swipe 300 1600 900 1600 200' },
  @{ n = 'drag-v200';     a = 'uitest uiInput drag 300 1600 900 1600 200' }
)
foreach ($c in $cands) {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell $c.a | Out-Null
  Start-Sleep -Seconds 4
  Dump-Events ("A-events-" + $c.n + ".txt")
  Dump-MetricSeries ("A-metric-series-" + $c.n + ".txt")
  Set-Content -Path (Join-Path $out ("A-metric-last-" + $c.n + ".txt")) -Value (Last-Metric)
  Shot ("A-after-" + $c.n)
  Write-Output ("PART A candidate " + $c.n + " done")
}

# ================= PART B: shake path =================
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true --ps shake 1.0" | Out-Null
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
for ($k = 0; $k -lt 8; $k++) {
  Start-Sleep -Seconds 3
  Dump-Events 'B-events-shake.txt'
  Dump-MetricSeries 'B-metric-series-shake.txt'
}
Shot 'B-shake-1.0'
Set-Content -Path (Join-Path $out 'B-metric-last-shake.txt') -Value (Last-Metric)
Write-Output "PART B shake done"

# control: same elapsed time, no shake
& $hdc shell 'hilog -r' | Out-Null
& $hdc shell "aa force-stop $bundle" | Out-Null
Start-Sleep -Seconds 2
& $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true" | Out-Null
Start-Sleep -Seconds 5
& $hdc shell 'uitest uiInput click 240 620' | Out-Null
for ($k = 0; $k -lt 8; $k++) {
  Start-Sleep -Seconds 3
  Dump-MetricSeries 'B-metric-series-control.txt'
}
Shot 'B-control-0.0'
Set-Content -Path (Join-Path $out 'B-metric-last-control.txt') -Value (Last-Metric)
Write-Output "PART B control done"
Write-Output "DONE"
