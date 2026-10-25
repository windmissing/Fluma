# P-012 fallsand fix retest - part 3: V3 capacity-rejection scenario with above-slop swipe
$hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe'
$bundle = 'com.example.helloworld'
$out = 'D:\hmprojects\HelloWorld\test\artifacts\p012-fallsand-fixretest-20261025'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue

function Start-App {
  & $hdc shell 'hilog -r' | Out-Null
  & $hdc shell "aa force-stop $bundle" | Out-Null
  Start-Sleep -Seconds 2
  & $hdc shell "aa start -a EntryAbility -b $bundle --ps rendermode 4 --ps p012tier 3 --ps p012seed 20261020 --ps testmode true" | Out-Null
  Start-Sleep -Seconds 5
  & $hdc shell 'uitest uiInput click 240 620' | Out-Null
  Start-Sleep -Seconds 3
}
function Shot($n) {
  & $hdc shell "snapshot_display -f /data/local/tmp/r3-$n.jpeg" | Out-Null
  & $hdc file recv /data/local/tmp/r3-$n.jpeg (Join-Path $out "$n.jpeg") | Out-Null
}
function Dump-Events($file) {
  & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_EVENT' | ForEach-Object { $_.Line } | Out-File (Join-Path $out $file) -Append
}
function Last-Metric {
  $m = & $hdc shell 'hilog -x' 2>$null | Select-String 'WS_METRIC' | Select-Object -Last 1
  if ($m) { return $m.Line } else { return '' }
}

# V3: same-column repeated injection (30px sweep = above pan slop) until capacity rejects
Start-App
Start-Sleep -Seconds 20
$dir = Join-Path $out 'frames-v3b'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
& $hdc shell 'hilog -r' | Out-Null
for ($k = 1; $k -le 20; $k++) {
  & $hdc shell 'uitest uiInput swipe 620 1600 650 1600 300' | Out-Null
  Start-Sleep -Milliseconds 350
}
$end = (Get-Date).AddSeconds(15); $i = 0
while ((Get-Date) -lt $end) {
  & $hdc shell 'snapshot_display -f /data/local/tmp/r3-v3b.jpeg' | Out-Null
  & $hdc file recv /data/local/tmp/r3-v3b.jpeg (Join-Path $dir ("f-{0:d4}.jpeg" -f $i)) | Out-Null
  $i++; Start-Sleep -Milliseconds 230
}
Dump-Events 'D-events-v3b.txt'
Set-Content -Path (Join-Path $out 'D-metric-v3b.txt') -Value (Last-Metric)
Shot 'D-v3b-final'
Write-Output ("PART E v3b frames: " + $i)
Write-Output "DONE"
