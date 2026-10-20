param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [int]$StartIndex = 1,
  [int]$Count = 1,
  [string]$Hap = 'D:\hmprojects\hw-obs3v2-891370d\entry\build\default\outputs\default\entry-default-p011-obs3v2-891370d-signed.hap',
  [string]$ExpectedHapSha256 = 'c9867dac82cd9689bdbb97187b3f9ae195ef3f3a57c9e80eedbf3cbaadf62d6f',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-obs3v2-20261020',
  [string]$RunRole = 'calibration',
  [string]$ThresholdCalibrationId = 'hysteresis-calib-v1',
  [double]$ThetaStartDeg = 28,
  [double]$ThetaStopDeg = 22.911,
  [string]$GitCommit = '891370d731eef32bf813f52910fa5a82439df2ed',
  [string]$ProtocolId = 'p011-observation3-v2',
  [string]$ProtocolRevision = 'preregistration-v1',
  [string]$ScheduleCsv = 'D:\hmprojects\HelloWorld\calibration\schedule-obs3v2-preregistration-v1.csv',
  [string]$ExpectedScheduleSha256 = '685be2bcbfdc14e261d86e3a105fc9a383a386709c02b96613f48f46504fe5a4'
)

$ErrorActionPreference = 'Stop'

Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

# Protocol v2 (preregistration-v1) nine-phase runner. Frozen schedule 0/+15/0/+30/0/-15/0/-30/0.
# Schedule identity: canonical CSV verified against designer-registered scheduleSha256 before any run.

$scheduleSha = (Get-FileHash -LiteralPath $ScheduleCsv -Algorithm SHA256).Hash.ToLowerInvariant()
if ($scheduleSha -ne $ExpectedScheduleSha256) {
  throw "Schedule SHA256 mismatch: $scheduleSha != $ExpectedScheduleSha256 (preregistration binding failed)"
}
$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) {
  throw "HAP SHA256 mismatch: $hapSha != $ExpectedHapSha256 (stale-package guard)"
}
$runnerPath = $MyInvocation.MyCommand.Path
$runnerSha = (Get-FileHash -LiteralPath $runnerPath -Algorithm SHA256).Hash.ToLowerInvariant()

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) {
    throw 'hdc failed: ' + ($Arguments -join ' ')
  }
  return $output
}

function Send-Want {
  param([double]$TargetTilt)
  Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
    '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
    '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
    '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
    '--ps','wstilt',([string]$TargetTilt),
    '--ps','p011runrole',$RunRole,
    '--ps','thresholdcalibrationid',$ThresholdCalibrationId,
    '--ps','thetastartdeg',([string]$ThetaStartDeg),
    '--ps','thetastopdeg',([string]$ThetaStopDeg)) | Out-Null
  return [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
}

function Capture-Frame {
  param([string]$Phase, [long]$PhaseStartMs, [int]$Index)
  $name = $Phase + '-' + $Index
  $devicePath = '/data/local/tmp/p011-obs3v2-' + $name + '.jpeg'
  $phaseDir = Join-Path $RunDir ('raster/' + $Phase)
  New-Item -ItemType Directory -Force -Path $phaseDir | Out-Null
  $localPath = Join-Path $phaseDir ($name + '.jpeg')
  $captureStart = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  Invoke-Hdc @('shell','snapshot_display','-f',$devicePath) | Out-Null
  Invoke-Hdc @('file','recv',$devicePath,$localPath) | Out-Null
  Invoke-Hdc @('shell','rm','-f',$devicePath) | Out-Null
  $captureEnd = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  return @{
    phase = $Phase
    file = ('raster/' + $Phase + '/' + $name + '.jpeg')
    captureTimeMs = $captureEnd
    captureDurationMs = ($captureEnd - $captureStart)
    relativePhaseTimeMs = ($captureEnd - $PhaseStartMs)
  }
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
      if ($text -match 'P011_EVENT reset' -and $text -match 'P011_OBS_STORE') {
        return
      }
    }
    Start-Sleep -Milliseconds 200
  }
  throw 'Timed out waiting for reset and observation store'
}

# Frozen nine-phase schedule (preregistration-v1 section 2). Holds 5100ms each.
$schedule = @(
  @{ phaseIndex = 0; phase = 'baseline'; target = 0; holdMs = 5100 },
  @{ phaseIndex = 1; phase = 'positive'; target = 15; holdMs = 5100 },
  @{ phaseIndex = 2; phase = 'positiveReturn'; target = 0; holdMs = 5100 },
  @{ phaseIndex = 3; phase = 'positiveOvershoot'; target = 30; holdMs = 5100 },
  @{ phaseIndex = 4; phase = 'positiveOvershootReturn'; target = 0; holdMs = 5100 },
  @{ phaseIndex = 5; phase = 'negative'; target = -15; holdMs = 5100 },
  @{ phaseIndex = 6; phase = 'negativeReturn'; target = 0; holdMs = 5100 },
  @{ phaseIndex = 7; phase = 'negativeOvershoot'; target = -30; holdMs = 5100 },
  @{ phaseIndex = 8; phase = 'negativeOvershootReturn'; target = 0; holdMs = 5100 }
)

# Install once per batch; digest verified above.
Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Invoke-Hdc @('install','-r',$Hap) | Out-Null

for ($runIndex = $StartIndex; $runIndex -lt ($StartIndex + $Count); $runIndex++) {
  $runName = 'run-' + ('{0:d3}' -f $runIndex)
  $RunDir = Join-Path $OutputRoot $runName
  if (Test-Path -LiteralPath $RunDir) { throw "Evidence already exists: $RunDir" }
  New-Item -ItemType Directory -Path $RunDir | Out-Null
  Write-Output "=== $runName start ==="

  $scheduleRows = @('sequence,phaseIndex,phase,target,holdMs,commandTimeMs')
  $captures = @()
  $logPath = Join-Path $RunDir 'hilog.txt'

  Invoke-Hdc @('shell','aa','force-stop',$Bundle)
  Invoke-Hdc @('shell','hilog','-r') | Out-Null
  Invoke-Hdc @('shell','hilog','-G','16M','-t','app') | Out-Null
  $hilog = Start-Process -FilePath $Hdc -ArgumentList @('-t',$Target,'shell','hilog') `
    -RedirectStandardOutput $logPath -RedirectStandardError ($logPath + '.stderr') `
    -WindowStyle Hidden -PassThru

  try {
    Start-Sleep -Seconds 1
    foreach ($phase in $schedule) {
      $commandTime = Send-Want $phase.target
      $scheduleRows += ([string]($phase.phaseIndex + 1)) + ',' + ([string]$phase.phaseIndex) + ',' + $phase.phase + ',' +
        ([string]$phase.target) + ',' + ([string]$phase.holdMs) + ',' + ([string]$commandTime)
      if ($phase.phaseIndex -eq 0) {
        Wait-ObservationReady $logPath
      }
      $phaseEnd = [DateTimeOffset]::UtcNow.AddMilliseconds($phase.holdMs)
      $index = 0
      while ([DateTimeOffset]::UtcNow -lt $phaseEnd) {
        $captures += Capture-Frame $phase.phase $commandTime $index
        $index += 1
        $remaining = ($phaseEnd - [DateTimeOffset]::UtcNow).TotalMilliseconds
        if ($remaining -gt 100) { Start-Sleep -Milliseconds 100 }
      }
      $captures += Capture-Frame $phase.phase $commandTime $index
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
  if ($ledgerDevicePaths.Count -ne 1) {
    throw 'ledger physical path resolution failed'
  }
  Invoke-Hdc @('file','recv',$ledgerDevicePaths[0],$ledgerPath) | Out-Null

  Add-Type -AssemblyName System.Drawing
  foreach ($capture in $captures) {
    $imagePath = Join-Path $RunDir $capture.file
    $image = [System.Drawing.Image]::FromFile($imagePath)
    $capture.width = $image.Width
    $capture.height = $image.Height
    $capture.sha256 = (Get-FileHash -LiteralPath $imagePath -Algorithm SHA256).Hash.ToLowerInvariant()
    $image.Dispose()
  }
  $manifestPath = Join-Path $RunDir 'raster-manifest.json'
  @{
    observationRouteId = 'natural-relax-hysteresis-v1'
    runRole = $RunRole
    thresholdCalibrationId = $ThresholdCalibrationId
    thetaStartDeg = $ThetaStartDeg
    thetaStopDeg = $ThetaStopDeg
    transitionTelemetryAvailable = $true
    protocolId = $ProtocolId
    protocolRevision = $ProtocolRevision
    analysisRole = 'pilot'
    scheduleSha256 = $scheduleSha
    runnerSha256 = $runnerSha
    hapSha256 = $hapSha
    gitCommit = $GitCommit
    runName = $runName
    layout = $layout
    captures = $captures
  } | ConvertTo-Json -Depth 6 |
    Set-Content -LiteralPath $manifestPath -Encoding UTF8

  $sourceFiles = @(
    'calibration/schedule-obs3v2-preregistration-v1.csv',
    'entry/build-profile.json5',
    'entry/src/main/ets/physics/P011MaterialGuard.ets',
    'entry/src/main/ets/physics/P011NaturalObservation.ets',
    'entry/src/main/ets/physics/P011NaturalObservationStore.ets',
    'entry/src/main/ets/physics/P011NaturalObservationWorker.ets',
    'entry/src/main/ets/physics/P011TiltPlanningPass.ets',
    'entry/src/main/ets/physics/WaterSandField.ets',
    'entry/src/main/ets/pages/SandCanvas.ets'
  )
  $sourceEntries = @()
  foreach ($sourceFile in $sourceFiles) {
    $fullPath = Join-Path 'D:\hmprojects\hw-obs3v2-891370d' $sourceFile
    if (-not (Test-Path -LiteralPath $fullPath)) {
      $fullPath = Join-Path 'D:\hmprojects\HelloWorld' $sourceFile
    }
    $sourceEntries += @{
      path = $sourceFile
      sha256 = (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash.ToLowerInvariant()
    }
  }
  $sourceManifestPath = Join-Path $RunDir 'source-manifest.json'
  @{
    observationRouteId = 'natural-relax-hysteresis-v1'
    runRole = $RunRole
    thresholdCalibrationId = $ThresholdCalibrationId
    thetaStartDeg = $ThetaStartDeg
    thetaStopDeg = $ThetaStopDeg
    transitionTelemetryAvailable = $true
    protocolId = $ProtocolId
    protocolRevision = $ProtocolRevision
    analysisRole = 'pilot'
    scheduleSha256 = $scheduleSha
    runnerSha256 = $runnerSha
    hap = $Hap
    hapSha256 = $hapSha
    gitCommit = $GitCommit
    sources = $sourceEntries } | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $sourceManifestPath -Encoding UTF8

  Write-Output "=== $runName collection done, running checkers ==="
  $py = 'python'
  Push-Location 'D:\hmprojects\HelloWorld\test'
  # Safety gates (unchanged MUST-PASS owner)
  $safetySummary = Join-Path $RunDir 'safety-summary.json'
  $safetyReport = Join-Path $RunDir 'safety-report.md'
  & $py 'p011_slip_formal_check.py' --ledger $ledgerPath --hilog $logPath --raster-manifest $manifestPath `
    --output $safetySummary --report $safetyReport 2>&1 | Select-Object -Last 3
  $safetyExit = $LASTEXITCODE
  # Calibration observability (protocol metadata + platform + cell status)
  $calibSummary = Join-Path $RunDir 'calibration-summary.json'
  & $py 'p011_calibration_protocol_check.py' --ledger $ledgerPath --manifest $manifestPath `
    --output $calibSummary 2>&1 | Select-Object -Last 3
  $calibExit = $LASTEXITCODE
  Pop-Location
  Write-Output "=== $runName safetyExit=$safetyExit calibExit=$calibExit ==="
}
exit 0
