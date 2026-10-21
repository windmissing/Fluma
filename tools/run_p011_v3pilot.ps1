param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [int]$StartIndex = 1,
  [int]$Count = 1,
  [string]$Hap = 'D:\hmprojects\hw-v3pilot-742efa6\entry\build\default\outputs\default\entry-default-s101-signed.hap',
  [string]$ExpectedHapSha256 = 'f2c5b06abc078cbd511e5c1ebbce61b936772fe7afca73e9cb8b0c2cf4880d23',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p011-v3pilot-20261021',
  [string]$RunRole = 'calibration',
  [string]$ThresholdCalibrationId = 'hysteresis-calib-v1',
  [double]$ThetaStartDeg = 26,
  [double]$ThetaStopDeg = 20,
  [string]$GitCommit = '742efa6d1f3f3c5833c3f850547ef0a9d7c634a5',
  [string]$WorktreeRoot = 'D:\hmprojects\hw-v3pilot-742efa6',
  [string]$ProtocolId = 'p011-observation3-v3',
  [string]$ProtocolRevision = 'preregistration-v1',
  [string]$ScheduleCsv = 'D:\hmprojects\HelloWorld\calibration\schedule-obs3v3-A18.7-rate0.5.csv',
  [string]$ExpectedScheduleSha256 = '7b59b2e4c4f4d87e9390cfc85cc7fb7c4c94a9233d70389cc8fadcba37b6bd46'
)

$ErrorActionPreference = 'Stop'

Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

# Observation-3 v3 single-run clean pilot runner (manager approved 2026-10-21 10:46).
# Schedule: nine platforms 0/+15/+18.7/+15/0/-15/-18.7/-15/0 (A_selected from shadow
# audit, minimum grid A with SHADOW_SAFE && SHADOW_COVERED), eight ramp legs,
# 0.5 deg/s, 100 ms command step. Every command binds legId/segmentId/phaseKind.

$scheduleSha = (Get-FileHash -LiteralPath $ScheduleCsv -Algorithm SHA256).Hash.ToLowerInvariant()
if ($scheduleSha -ne $ExpectedScheduleSha256) {
  throw "Schedule SHA256 mismatch: $scheduleSha != $ExpectedScheduleSha256"
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

# Parse the frozen schedule CSV into command objects.
$commands = @()
$csvLines = [System.IO.File]::ReadAllLines($ScheduleCsv)
for ($i = 1; $i -lt $csvLines.Count; $i++) {
  $cols = $csvLines[$i].Split(',')
  if ($cols.Count -lt 7) { continue }
  $commands += @{
    seq = [int]$cols[0]; offsetMs = [int]$cols[1]; seg = $cols[2]
    kind = $cols[3]; leg = $cols[4]; target = [double]$cols[5]; rate = [double]$cols[6]
  }
}
if ($commands.Count -eq 0) { throw 'empty schedule csv' }
Write-Output ("schedule commands: " + $commands.Count)

function Send-Want {
  param($Cmd)
  $args = @('shell','aa','start','-b',$Bundle,'-a',$Ability,
    '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
    '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','true',
    '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
    '--ps','wstilt',([string]$Cmd.target),
    '--ps','p011runrole',$RunRole,
    '--ps','thresholdcalibrationid',$ThresholdCalibrationId,
    '--ps','thetastartdeg',([string]$ThetaStartDeg),
    '--ps','thetastopdeg',([string]$ThetaStopDeg),
    '--ps','scheduleid','nine-phase-30-v3',
    '--ps','v3legid',$Cmd.leg,
    '--ps','v3segmentid',$Cmd.seg,
    '--ps','v3phasekind',$Cmd.kind)
  Invoke-Hdc @($args) | Out-Null
  return [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
}

function Wait-ObservationReady {
  param([string]$LogPath)
  $deadline = [DateTimeOffset]::UtcNow.AddSeconds(20)
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

function Capture-Frame {
  param([string]$Seg, [long]$PhaseStartMs, [int]$Index)
  $name = $Seg + '-' + $Index
  $devicePath = '/data/local/tmp/p011-v3pilot-' + $name + '.jpeg'
  $segDir = Join-Path $RunDir ('raster/' + $Seg)
  New-Item -ItemType Directory -Force -Path $segDir | Out-Null
  $localPath = Join-Path $segDir ($name + '.jpeg')
  $captureStart = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  Invoke-Hdc @('shell','snapshot_display','-f',$devicePath) | Out-Null
  Invoke-Hdc @('file','recv',$devicePath,$localPath) | Out-Null
  Invoke-Hdc @('shell','rm','-f',$devicePath) | Out-Null
  $captureEnd = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  return @{
    segment = $Seg
    file = ('raster/' + $Seg + '/' + $name + '.jpeg')
    captureTimeMs = $captureEnd
    captureDurationMs = ($captureEnd - $captureStart)
    relativePhaseTimeMs = ($captureEnd - $PhaseStartMs)
  }
}

# Install once per batch; digest verified above.
Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Invoke-Hdc @('install','-r',$Hap) | Out-Null

for ($runIndex = $StartIndex; $runIndex -lt ($StartIndex + $Count); $runIndex++) {
  $runName = 'run-' + ('{0:d3}' -f $runIndex)
  $RunDir = Join-Path $OutputRoot $runName
  if (Test-Path -LiteralPath $RunDir) { throw "Evidence already exists: $RunDir" }
  New-Item -ItemType Directory -Path $RunDir | Out-Null
  Write-Output "=== $runName start ==="

  $commandLog = @('commandSequence,segmentId,phaseKind,legId,targetTiltDeg,commandTimeMs')
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
    $t0 = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
    $firstSent = $false
    # command timeline: each command at t0 + offsetMs (wall-clock anchored)
    foreach ($cmd in $commands) {
      $due = $t0 + $cmd.offsetMs
      $now = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
      if ($due -gt $now) { Start-Sleep -Milliseconds ($due - $now) }
      $sentAt = Send-Want $cmd
      $commandLog += ([string]$cmd.seq) + ',' + $cmd.seg + ',' + $cmd.kind + ',' + $cmd.leg + ',' +
        ([string]$cmd.target) + ',' + ([string]$sentAt)
      if (-not $firstSent) {
        Wait-ObservationReady $logPath
        $firstSent = $true
      }
      # raster capture: one frame per platform segment start, plus ramp sampling
      if ($cmd.kind -eq 'platform') {
        $captures += Capture-Frame $cmd.seg $sentAt 0
      }
    }
    $totalMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds() - $t0
    Write-Output ("timeline done, wall=" + $totalMs + "ms (ideal=196300ms)")
    Start-Sleep -Seconds 3
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

  $commandLogPath = Join-Path $RunDir 'command-log.csv'
  [System.IO.File]::WriteAllLines($commandLogPath, $commandLog)

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
    scheduleCsv = $ScheduleCsv
    runnerSha256 = $runnerSha
    hapSha256 = $hapSha
    gitCommit = $GitCommit
    runName = $runName
    layout = $layout
    captures = $captures
    calibrationProtocol = @{
      protocolId = $ProtocolId
      protocolRevision = $ProtocolRevision
      scheduleSha256 = $scheduleSha
      configuredThresholds = @{ thetaStartDeg = $ThetaStartDeg; thetaStopDeg = $ThetaStopDeg }
    }
  } | ConvertTo-Json -Depth 6 |
    Set-Content -LiteralPath $manifestPath -Encoding UTF8

  $sourceFiles = @(
    'calibration/schedule-obs3v3-A18.7-rate0.5.csv',
    'tools/gen_p011_v3_schedule.py',
    'tools/run_p011_v3pilot.ps1',
    'entry/build-profile.json5',
    'entry/src/main/ets/physics/P011MaterialGuard.ets',
    'entry/src/main/ets/physics/P011NaturalObservation.ets',
    'entry/src/main/ets/physics/P011NaturalObservationStore.ets',
    'entry/src/main/ets/physics/P011NaturalObservationWorker.ets',
    'entry/src/main/ets/physics/P011RelaxHysteresis.ets',
    'entry/src/main/ets/physics/P011ScheduleV3.ets',
    'entry/src/main/ets/physics/P011TiltPlanningPass.ets',
    'entry/src/main/ets/physics/P011TiltWant.ets',
    'entry/src/main/ets/physics/WaterSandField.ets',
    'entry/src/main/ets/pages/SandCanvas.ets'
  )
  $sourceEntries = @()
  foreach ($sourceFile in $sourceFiles) {
    $fullPath = Join-Path $WorktreeRoot $sourceFile
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
    shadowAuditSha256 = (Get-FileHash -LiteralPath 'D:\hmprojects\HelloWorld\test\artifacts\p011-v3-shadow-audit\shadow-audit-results.json' -Algorithm SHA256).Hash.ToLowerInvariant()
    sources = $sourceEntries } | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $sourceManifestPath -Encoding UTF8

  Write-Output "=== $runName collection done, running v3 checker ==="
  $py = 'python'
  Push-Location 'D:\hmprojects\HelloWorld\test'
  $v3Summary = Join-Path $RunDir 'v3-summary.json'
  & $py 'p011_obs3_v3_protocol_check.py' --ledger $ledgerPath --manifest $manifestPath `
    --source pilot --output $v3Summary 2>&1 | Select-Object -Last 4
  $v3Exit = $LASTEXITCODE
  # Safety gates (unchanged MUST-PASS owner)
  $safetySummary = Join-Path $RunDir 'safety-summary.json'
  $safetyReport = Join-Path $RunDir 'safety-report.md'
  & $py 'p011_slip_formal_check.py' --ledger $ledgerPath --hilog $logPath --raster-manifest $manifestPath `
    --output $safetySummary --report $safetyReport 2>&1 | Select-Object -Last 3
  $safetyExit = $LASTEXITCODE
  Pop-Location
  Write-Output "=== $runName v3Exit=$v3Exit safetyExit=$safetyExit ==="
}
exit 0
