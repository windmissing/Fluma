param(
  [string]$Hdc = "C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe",
  [string]$Target = "62T0225B10010020",
  [string]$Bundle = "com.example.helloworld",
  [string]$Ability = "EntryAbility",
  [string]$Output = "test\artifacts\p011-tc02-original-20261016.log",
  [int]$IntervalMs = 100
)
$ErrorActionPreference = "Stop"
function Start-Tilt {
  param([double]$TargetAngle)
  & $Hdc -t $Target shell aa start -b $Bundle -a $Ability --ps rendermode 4 --ps testmode true --ps wspreset thick --ps wssnapshot true --ps wscols 10 --ps wscapacity 1000 --ps wstilt $TargetAngle
  if ($LASTEXITCODE -ne 0) { throw "aa start failed: $LASTEXITCODE" }
}
& $Hdc -t $Target shell aa force-stop $Bundle | Out-Null
if ($LASTEXITCODE -ne 0) { throw "force-stop failed" }
& $Hdc -t $Target shell hilog -r | Out-Null
if ($LASTEXITCODE -ne 0) { throw "hilog reset failed" }
$logDir = Split-Path -Parent $Output
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$argumentList = @("-t", $Target, "shell", "hilog")
$hilogProcess = Start-Process -FilePath $Hdc -ArgumentList $argumentList -RedirectStandardOutput $Output -RedirectStandardError "$Output.stderr" -WindowStyle Hidden -PassThru
Start-Tilt 0
Start-Sleep -Milliseconds 3000
$zeroToPositive = 2000
$positiveToNegative = 4000
$negativeToZero = 2000
for ($round = 1; $round -le 5; $round++) {
  for ($elapsed = $IntervalMs; $elapsed -le $zeroToPositive; $elapsed += $IntervalMs) {
    Start-Tilt ([math]::Round(20.0 * $elapsed / $zeroToPositive, 3))
    Start-Sleep -Milliseconds $IntervalMs
  }
  Start-Sleep -Milliseconds 1000
  for ($elapsed = $IntervalMs; $elapsed -le $positiveToNegative; $elapsed += $IntervalMs) {
    Start-Tilt ([math]::Round(20.0 - 40.0 * $elapsed / $positiveToNegative, 3))
    Start-Sleep -Milliseconds $IntervalMs
  }
  Start-Sleep -Milliseconds 1000
  for ($elapsed = $IntervalMs; $elapsed -le $negativeToZero; $elapsed += $IntervalMs) {
    Start-Tilt ([math]::Round(-20.0 + 20.0 * $elapsed / $negativeToZero, 3))
    Start-Sleep -Milliseconds $IntervalMs
  }
}
Start-Sleep -Seconds 2
if (-not $hilogProcess.HasExited) { Stop-Process -Id $hilogProcess.Id -Force }
$hilogProcess.WaitForExit(3000)


