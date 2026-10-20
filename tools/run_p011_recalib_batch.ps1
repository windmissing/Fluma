param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [int]$StartIndex = 1,
  [int]$Count = 1,
  [string]$Hap = 'D:\hmprojects\HelloWorld\entry\build\default\outputs\default\entry-default-p011-recon-fix-signed.hap',
  [string]$ExpectedHapSha256 = 'dca1a7c7cff0aa08ae086cdd94e6f93a309f3a81df0dbd625e9793f960e71b74',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-v212-recalib3-20261020',
  [string]$RunRole = 'calibration',
  [string]$ThresholdCalibrationId = 'hysteresis-calib-v1',
  [double]$ThetaStartDeg = 28,
  [double]$ThetaStopDeg = 22.911,
  [string]$GitCommit = '835e663cc86616813d6567ba5a1d1d5d3dad4876'
)

$ErrorActionPreference = 'Stop'

# Sandbox proxy env vars break Start-Process env inheritance; clear them for this session.
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

# Per-run root: each run gets its own directory so run-level evidence is frozen independently.
# runRole stays 'calibration' for every new run (the app has no 'fit' role); the fit/holdout
# assignment happens post-collection via the randomization manifest (seed 20261019), never here.

$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) {
  throw "HAP SHA256 mismatch: $hapSha != $ExpectedHapSha256 (stale-package guard)"
}

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
  $devicePath = '/data/local/tmp/p011-recalib-' + $name + '.jpeg'
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

# Install once per batch: verified digest above guards against a stale package.
Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Invoke-Hdc @('install','-r',$Hap) | Out-Null

for ($runIndex = $StartIndex; $runIndex -lt ($StartIndex + $Count); $runIndex++) {
  $runName = 'run-' + ('{0:d3}' -f $runIndex)
  $RunDir = Join-Path $OutputRoot $runName
  if (Test-Path -LiteralPath $RunDir) { throw "Evidence already exists: $RunDir" }
  New-Item -ItemType Directory -Path $RunDir | Out-Null
  Write-Output "=== $runName start ==="

  $schedule = @(
    @{ sequence = 1; phase = 'baseline'; target = 0; holdMs = 5100 },
    @{ sequence = 2; phase = 'positive'; target = 15; holdMs = 5100 },
    @{ sequence = 3; phase = 'positiveReturn'; target = 0; holdMs = 5100 },
    @{ sequence = 4; phase = 'positiveOvershoot'; target = 45; holdMs = 5100 },
    @{ sequence = 5; phase = 'positiveOvershootReturn'; target = 0; holdMs = 5100 },
    @{ sequence = 6; phase = 'negativeOvershoot'; target = -45; holdMs = 5100 },
    @{ sequence = 7; phase = 'negativeOvershootReturn'; target = 0; holdMs = 5100 }
  )
  $scheduleRows = @('sequence,phase,target,holdMs,commandTimeMs')
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
      $scheduleRows += ([string]$phase.sequence) + ',' + $phase.phase + ',' +
        ([string]$phase.target) + ',' + ([string]$phase.holdMs) + ',' + ([string]$commandTime)
      if ($phase.sequence -eq 1) {
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
    hapSha256 = $hapSha
    gitCommit = $GitCommit
    runName = $runName
    layout = $layout
    captures = $captures
  } | ConvertTo-Json -Depth 6 |
    Set-Content -LiteralPath $manifestPath -Encoding UTF8

  $sourceFiles = @(
    'entry/build-profile.json5',
    'entry/src/main/ets/physics/P011NaturalObservation.ets',
    'entry/src/main/ets/physics/P011NaturalObservationStore.ets',
    'entry/src/main/ets/physics/P011NaturalObservationWorker.ets',
    'entry/src/main/ets/physics/P011TiltPlanningPass.ets',
    'entry/src/main/ets/physics/WaterSandField.ets',
    'entry/src/main/ets/pages/SandCanvas.ets'
  )
  $sourceEntries = @()
  foreach ($sourceFile in $sourceFiles) {
    $fullPath = Join-Path 'D:\hmprojects\HelloWorld' $sourceFile
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
    hap = $Hap
    hapSha256 = $hapSha
    gitCommit = $GitCommit
    sources = $sourceEntries } | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $sourceManifestPath -Encoding UTF8

  Write-Output "=== $runName collection done, running checker ==="
  $summaryPath = Join-Path $RunDir 'summary.json'
  $reportPath = Join-Path $RunDir 'observation-report.md'
  $checker = 'D:\hmprojects\HelloWorld\test\p011_slip_formal_check.py'
  $py = 'python'
  Push-Location 'D:\hmprojects\HelloWorld\test'
  & $py $checker --ledger $ledgerPath --hilog $logPath --raster-manifest $manifestPath `
    --output $summaryPath --report $reportPath 2>&1 |
    Select-Object -Last 5
  $checkerExit = $LASTEXITCODE
  Pop-Location
  Write-Output "=== $runName checker exit=$checkerExit ==="
}
exit 0
