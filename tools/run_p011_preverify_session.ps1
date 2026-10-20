param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [string]$Hap = 'D:\hmprojects\hw-preverify-5e60f2d\entry\build\default\outputs\default\entry-default-p011-preverify-5e60f2d-signed.hap',
  [string]$ExpectedHapSha256 = '7ffc667fb328b849dd41c91957970b0ac6743e48e958c753a29231277e287b6a',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-preverify-20261020',
  [string]$RunRole = 'calibration',
  [string]$ThresholdCalibrationId = 'hysteresis-calib-v1',
  [double]$ThetaStartDeg = 28,
  [double]$ThetaStopDeg = 22.911,
  [string]$GitCommit = '5e60f2db5fe15ca51cd65507d06a1d8279fb4a32'
)

$ErrorActionPreference = 'Stop'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

# Preverify session (informal, NOT formal acceptance):
#   run A = seven-phase-45-v1 (C-line: +/-15 static + repose/overshoot; SLIP bottom-line rides on ledger)
#   run B = nine-phase-30-v2 (verifies 5e60f2d phase mapping + planner fields on device)

$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) { throw "HAP digest mismatch: $hapSha" }

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed: ' + ($Arguments -join ' ') }
  return $output
}

function Send-Want {
  param([double]$TargetTilt, [string]$ScheduleId)
  Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
    '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
    '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
    '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
    '--ps','wstilt',([string]$TargetTilt),
    '--ps','p011runrole',$RunRole,
    '--ps','thresholdcalibrationid',$ThresholdCalibrationId,
    '--ps','thetastartdeg',([string]$ThetaStartDeg),
    '--ps','thetastopdeg',([string]$ThetaStopDeg),
    '--ps','scheduleid',$ScheduleId) | Out-Null
  return [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
}

function Capture-Frame {
  param([string]$Phase, [long]$PhaseStartMs, [int]$Index, [string]$RunDir)
  $name = $Phase + '-' + $Index
  $devicePath = '/data/local/tmp/p011-preverify-' + $name + '.jpeg'
  $phaseDir = Join-Path $RunDir ('raster/' + $Phase)
  New-Item -ItemType Directory -Force -Path $phaseDir | Out-Null
  $localPath = Join-Path $phaseDir ($name + '.jpeg')
  Invoke-Hdc @('shell','snapshot_display','-f',$devicePath) | Out-Null
  Invoke-Hdc @('file','recv',$devicePath,$localPath) | Out-Null
  Invoke-Hdc @('shell','rm','-f',$devicePath) | Out-Null
  return @{ phase = $Phase; file = ('raster/' + $Phase + '/' + $name + '.jpeg') }
}

function Wait-ObservationReady {
  param([string]$LogPath)
  $deadline = [DateTimeOffset]::UtcNow.AddSeconds(15)
  while ([DateTimeOffset]::UtcNow -lt $deadline) {
    if (Test-Path -LiteralPath $LogPath) {
      $stream = [System.IO.FileStream]::new($LogPath, [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
      $reader = [System.IO.StreamReader]::new($stream)
      try { $text = $reader.ReadToEnd() } finally { $reader.Dispose() }
      if ($text -match 'P011_EVENT reset' -and $text -match 'P011_OBS_STORE') { return }
    }
    Start-Sleep -Milliseconds 200
  }
  throw 'Timed out waiting for reset and observation store'
}

$sevenPhases = @(
  @{ phase = 'baseline'; target = 0; holdMs = 5100 },
  @{ phase = 'positive'; target = 15; holdMs = 5100 },
  @{ phase = 'positiveReturn'; target = 0; holdMs = 5100 },
  @{ phase = 'positiveOvershoot'; target = 45; holdMs = 5100 },
  @{ phase = 'positiveOvershootReturn'; target = 0; holdMs = 5100 },
  @{ phase = 'negativeOvershoot'; target = -45; holdMs = 5100 },
  @{ phase = 'negativeOvershootReturn'; target = 0; holdMs = 5100 }
)
$ninePhases = @(
  @{ phase = 'baseline'; target = 0; holdMs = 5100 },
  @{ phase = 'positiveLow'; target = 15; holdMs = 5100 },
  @{ phase = 'positiveLowReturn'; target = 0; holdMs = 5100 },
  @{ phase = 'positiveHigh'; target = 30; holdMs = 5100 },
  @{ phase = 'positiveHighReturn'; target = 0; holdMs = 5100 },
  @{ phase = 'negativeLow'; target = -15; holdMs = 5100 },
  @{ phase = 'negativeLowReturn'; target = 0; holdMs = 5100 },
  @{ phase = 'negativeHigh'; target = -30; holdMs = 5100 },
  @{ phase = 'negativeOvershootReturn'; target = 0; holdMs = 5100 }
)

Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Invoke-Hdc @('install','-r',$Hap) | Out-Null

$sessionDefs = @(
  @{ name = 'runA-seven45'; scheduleId = 'seven-phase-45-v1'; phases = $sevenPhases },
  @{ name = 'runB-nine30'; scheduleId = 'nine-phase-30-v2'; phases = $ninePhases }
)

foreach ($session in $sessionDefs) {
  $RunDir = Join-Path $OutputRoot $session.name
  if (Test-Path -LiteralPath $RunDir) { throw "Evidence already exists: $RunDir" }
  New-Item -ItemType Directory -Path $RunDir | Out-Null
  Write-Output ("=== " + $session.name + " start ===")

  $scheduleRows = @('sequence,phase,target,holdMs,commandTimeMs')
  $captures = @()
  $logPath = Join-Path $RunDir 'hilog.txt'

  Invoke-Hdc @('shell','aa','force-stop',$Bundle)
  Invoke-Hdc @('shell','hilog','-r') | Out-Null
  Invoke-Hdc @('shell','hilog','-G','16M','-t','app') | Out-Null
  $hilog = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog') `
    -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
    -WindowStyle Hidden -PassThru

  $seq = 0
  try {
    Start-Sleep -Seconds 1
    foreach ($phase in $session.phases) {
      $seq += 1
      $commandTime = Send-Want $phase.target $session.scheduleId
      $scheduleRows += ([string]$seq) + ',' + $phase.phase + ',' + ([string]$phase.target) + ',' +
        ([string]$phase.holdMs) + ',' + ([string]$commandTime)
      if ($seq -eq 1) { Wait-ObservationReady $logPath }
      $phaseEnd = [DateTimeOffset]::UtcNow.AddMilliseconds($phase.holdMs)
      $index = 0
      while ([DateTimeOffset]::UtcNow -lt $phaseEnd) {
        $captures += Capture-Frame $phase.phase $commandTime $index $RunDir
        $index += 1
        $remaining = ($phaseEnd - [DateTimeOffset]::UtcNow).TotalMilliseconds
        if ($remaining -gt 100) { Start-Sleep -Milliseconds 100 }
      }
      $captures += Capture-Frame $phase.phase $commandTime $index $RunDir
    }
  } finally {
    Invoke-Hdc @('shell','aa','force-stop',$Bundle)
    Start-Sleep -Seconds 1
    if (-not $hilog.HasExited) { Stop-Process -Id $hilog.Id -Force }
    $hilog.WaitForExit(5000) | Out-Null
    $hilogDump = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog','-x','-t','app') `
      -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
      -WindowStyle Hidden -PassThru
    if (-not $hilogDump.WaitForExit(30000)) {
      Stop-Process -Id $hilogDump.Id -Force
      throw 'hilog buffer dump timed out'
    }
  }

  $schedulePath = Join-Path $RunDir 'schedule.csv'
  $scheduleRows | Set-Content -LiteralPath $schedulePath -Encoding UTF8

  $storeLine = (Select-String -LiteralPath $logPath -Pattern 'P011_OBS_STORE' | Select-Object -First 1).Line
  $layoutLine = (Select-String -LiteralPath $logPath -Pattern 'P011_RASTER_LAYOUT' | Select-Object -First 1).Line
  if ($null -eq $storeLine -or $null -eq $layoutLine) { throw 'observation identity missing' }
  $store = $storeLine.Substring($storeLine.IndexOf('{')) | ConvertFrom-Json
  $layout = $layoutLine.Substring($layoutLine.IndexOf('{')) | ConvertFrom-Json
  $ledgerPath = Join-Path $RunDir 'ledger.ndjson'
  $ledgerName = [System.IO.Path]::GetFileName($store.filePath)
  $ledgerDevicePaths = @((Invoke-Hdc @('shell','find','/data/app/el2','-name',$ledgerName,'-type','f')) |
    ForEach-Object { $_.Trim() } | Where-Object { $_.StartsWith('/data/app/el2/') })
  if ($ledgerDevicePaths.Count -ne 1) { throw 'ledger physical path resolution failed' }
  Invoke-Hdc @('file','recv',$ledgerDevicePaths[0],$ledgerPath) | Out-Null

  Add-Type -AssemblyName System.Drawing
  $capList = @()
  foreach ($capture in $captures) {
    $imagePath = Join-Path $RunDir $capture.file
    if (Test-Path -LiteralPath $imagePath) {
      $image = [System.Drawing.Image]::FromFile($imagePath)
      $capture.width = $image.Width; $capture.height = $image.Height
      $capture.sha256 = (Get-FileHash -LiteralPath $imagePath -Algorithm SHA256).Hash.ToLowerInvariant()
      $image.Dispose()
      $capList += $capture
    }
  }
  $manifestPath = Join-Path $RunDir 'raster-manifest.json'
  @{
    observationRouteId = 'natural-relax-hysteresis-v1'
    runRole = $RunRole
    thresholdCalibrationId = $ThresholdCalibrationId
    thetaStartDeg = $ThetaStartDeg
    thetaStopDeg = $ThetaStopDeg
    transitionTelemetryAvailable = $true
    scheduleId = $session.scheduleId
    hapSha256 = $hapSha
    gitCommit = $GitCommit
    runName = $session.name
    layout = $layout
    captures = $capList
  } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding UTF8

  Write-Output ("=== " + $session.name + " collection done, running safety checker ===")
  $py = 'python'
  Push-Location 'D:\hmprojects\HelloWorld\test'
  & $py 'p011_slip_formal_check.py' --ledger $ledgerPath --hilog $logPath --raster-manifest $manifestPath `
    --output (Join-Path $RunDir 'safety-summary.json') --report (Join-Path $RunDir 'safety-report.md') 2>&1 |
    Select-Object -Last 3
  $exit = $LASTEXITCODE
  Pop-Location
  Write-Output ("=== " + $session.name + " safetyExit=" + $exit + " ===")
}
exit 0
