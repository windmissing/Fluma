param(
  [ValidateSet("Schedule", "Tc02", "SchedulePrecheckOnly", "PressureSegments", "All")][string]$Mode = "Schedule",
  [Alias("schedule-precheck-only")][switch]$SchedulePrecheckOnly,
  [string]$ReuseTc02Log,
  [string]$Hdc = "C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe",
  [string]$Target = "62T0225B10010020",
  [string]$Bundle = "com.example.helloworld",
  [string]$Ability = "EntryAbility",
  [string]$OutputRoot = "test\artifacts\p011-v28-canonical-20261018",
  [int]$IntervalMs = 100,
  [int]$Seed = 20261016,
  [double]$MinDisplayFrameMs = 8.333,
  [int]$PressureHoldSeconds = 20,
  [int]$ReadyTimeoutSeconds = 15,
  [string]$SuiteJson,
  [string]$SuiteMarkdown
)

$ErrorActionPreference = "Stop"
if ($SchedulePrecheckOnly) { $Mode = "SchedulePrecheckOnly" }
if ($IntervalMs -lt $MinDisplayFrameMs) {
  throw "IntervalMs=$IntervalMs is below one display frame ($MinDisplayFrameMs ms)"
}
if ($Seed -lt 0) { throw "Seed must be non-negative" }
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
$SchedulePath = Join-Path $OutputRoot "p011-tc02-schedule.csv"
$IdentityPath = Join-Path $OutputRoot "p011-tc02-capture-path.json"
$DeviceScriptPath = Join-Path $OutputRoot "p011-tc02-formal-device.sh"
if (-not $SuiteJson) { $SuiteJson = Join-Path $OutputRoot "p011-suite.json" }
if (-not $SuiteMarkdown) { $SuiteMarkdown = Join-Path $OutputRoot "p011-suite.md" }

# P-011 v2.8: never kill the hdc daemon or another target's capture.
function Get-TargetHilogCollectors {
  @(Get-CimInstance Win32_Process -Filter "Name = 'hdc.exe'" | Where-Object {
    $_.CommandLine -match ('(?:^|\s)-t\s+"?' + [regex]::Escape($Target) + '"?(?=\s|$)') -and
    $_.CommandLine -match '\bshell\s+hilog(?:\s|$)' -and
    $_.CommandLine -notmatch '\bhilog\s+-(?:r|G|h)(?:\s|$)'
  })
}

function Clear-ResidualHilogCollectors {
  foreach ($collector in @(Get-TargetHilogCollectors)) {
    Write-Host "Stopping residual target hilog collector PID $($collector.ProcessId)"
    Stop-Process -Id $collector.ProcessId -Force -ErrorAction Stop
    Wait-Process -Id $collector.ProcessId -Timeout 5 -ErrorAction SilentlyContinue
  }
  if (@(Get-TargetHilogCollectors).Count -ne 0) { throw "captureNotFrozen: residual target hilog" }
}

function Prepare-CanonicalCapture {
  python (Join-Path $PSScriptRoot "p011_tc02_schedule.py") --frame-ms $IntervalMs --output $SchedulePath `
    --prepare-capture --device-script $DeviceScriptPath --capture-identity $IdentityPath `
    --bundle $Bundle --ability $Ability --seed $Seed --target $Target
  if ($LASTEXITCODE -ne 0) { throw "canonical capture preparation failed" }
  $script:CanonicalCaptureIdentity = Get-Content -LiteralPath $IdentityPath -Raw -Encoding UTF8 | ConvertFrom-Json
  if ((Get-FileHash -LiteralPath $SchedulePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $script:CanonicalCaptureIdentity.scheduleSha256 -or
      (Get-FileHash -LiteralPath $DeviceScriptPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $script:CanonicalCaptureIdentity.injectionScriptSha256) {
    throw "capturePathMismatch: canonical schedule or injection script changed before capture"
  }
}

function Test-FrozenTc02 {
  param([string]$LogPath, [string]$FreezeContext)
  $arguments = @((Join-Path $PSScriptRoot "p011_tc02_schedule.py"), "--frame-ms", $IntervalMs,
    "--output", $SchedulePath, "--validate-log", $LogPath, "--capture-identity", $IdentityPath,
    "--validation-json", (Join-Path $OutputRoot "p011-tc02-validation.json"))
  if ($FreezeContext) { $arguments += @("--freeze-context", $FreezeContext) }
  python @arguments
  if ($LASTEXITCODE -ne 0) { throw "TC02 schedule precheck failed; physical scoring blocked" }
}

function Invoke-Device {
  param([string[]]$Arguments)
  & $Hdc -t $Target @Arguments | Out-Null
  if ($LASTEXITCODE -ne 0) { throw "hdc failed ($LASTEXITCODE): $($Arguments -join ' ')" }
}

function Send-Want {
  param([double]$TargetAngle)
  $argumentList = @(
    "shell", "aa", "start", "-b", $Bundle, "-a", $Ability,
    "--ps", "rendermode", "4",
    "--ps", "testmode", "true",
    "--ps", "forceinvert", "false",
    "--ps", "seed", "$Seed",
    "--ps", "wspreset", "thick",
    "--ps", "wssnapshot", "true",
    "--ps", "wscols", "10",
    "--ps", "wscapacity", "1000",
    "--ps", "wssandtopmax", "30",
    "--ps", "wstilt", "$TargetAngle"
  )
  Invoke-Device $argumentList
}

function Start-HilogCapture {
  param([string]$Output)
  $stderr = "$Output.stderr"
  if ((Test-Path -LiteralPath $Output) -or (Test-Path -LiteralPath $stderr)) {
    throw "Evidence already exists: $Output; use a fresh OutputRoot or -ReuseTc02Log"
  }
  Clear-ResidualHilogCollectors
  $hilogParameters = $script:CanonicalCaptureIdentity.hilogParameters
  Invoke-Device -Arguments @($hilogParameters.reset)
  Invoke-Device -Arguments @($hilogParameters.buffer)
  return Start-Process -FilePath $Hdc -ArgumentList (@("-t", $Target) + @($hilogParameters.capture)) `
    -RedirectStandardOutput $Output -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
}

function Stop-HilogCapture {
  param([System.Diagnostics.Process]$Process)
  if (-not $Process.HasExited) { Stop-Process -Id $Process.Id -Force }
  if (-not $Process.WaitForExit(5000)) { throw "captureNotFrozen: collector did not stop" }
  if (@(Get-TargetHilogCollectors).Count -ne 0) { throw "captureNotFrozen: live target hilog collector" }
}

function Wait-ResetRunMarker {
  param([string]$Output)
  $deadline = [DateTime]::UtcNow.AddSeconds($ReadyTimeoutSeconds)
  while ([DateTime]::UtcNow -lt $deadline) {
    if (Test-Path -LiteralPath $Output) {
      $stream = [System.IO.FileStream]::new($Output, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
      $reader = [System.IO.StreamReader]::new($stream)
      $text = $reader.ReadToEnd()
      $reader.Dispose()
      $resetMatch = [regex]::Match($text, 'P011_EVENT reset\s+(\{[^\r\n]*\})')
      if ($resetMatch.Success) {
        $resetEvent = $resetMatch.Groups[1].Value | ConvertFrom-Json
        if ($resetEvent.runId -and $text -match ('P011_METRIC\s+\{[^\r\n]*"runId":\s*"' + [regex]::Escape($resetEvent.runId) + '"')) {
          return $resetEvent.runId
        }
      }
    }
    Start-Sleep -Milliseconds 200
  }
  throw "Timed out waiting for P011_EVENT reset and first P011_METRIC with the same runId in $Output"
}

function Invoke-Tc02 {
  $logPath = Join-Path $OutputRoot "p011-tc02-formal.log"
  if (Test-Path -LiteralPath $logPath) { throw "TC02 evidence exists; choose fresh OutputRoot" }
  Prepare-CanonicalCapture
  Invoke-Device @("shell", "aa", "force-stop", $Bundle)
  Invoke-Device @("file", "send", $DeviceScriptPath, "/data/local/tmp/p011-tc02-formal-device.sh")
  $hilog = Start-HilogCapture $logPath
  $injectionCompleted = $false
  try {
    Start-Sleep -Seconds 1
    # This foreground call must return before stopping hilog or parsing any data.
    $injectionOutput = & $Hdc -t $Target shell sh /data/local/tmp/p011-tc02-formal-device.sh
    if ($LASTEXITCODE -ne 0 -or ($injectionOutput -join "`n") -notmatch 'P011_INJECTION_COMPLETED:531') {
      throw "TC02 injection failed or did not finish all 531 commands"
    }
    $injectionCompleted = $true
    if ($hilog.HasExited) { throw "captureNotFrozen: collector exited during injection" }
    Start-Sleep -Seconds 2
  }
  finally {
    Stop-HilogCapture $hilog
  }
  $contextPath = Join-Path $OutputRoot "p011-tc02-lifecycle.json"
  @{ injectionCompleted = $injectionCompleted; collectorStopped = $hilog.HasExited;
     collectorPid = $hilog.Id; residualCollectors = @(Get-TargetHilogCollectors).Count } |
    ConvertTo-Json | Set-Content -LiteralPath $contextPath -Encoding UTF8
  Test-FrozenTc02 $logPath $contextPath
  return $logPath
}

function Invoke-PressureSegment {
  param([string]$Name, [double]$TargetAngle)
  $logPath = Join-Path $OutputRoot "p011-$Name.log"
  Invoke-Device @("shell", "aa", "force-stop", $Bundle)
  $hilog = Start-HilogCapture $logPath
  try {
    Send-Want $TargetAngle
    Wait-ResetRunMarker $logPath
    Start-Sleep -Seconds $PressureHoldSeconds
  }
  finally {
    Stop-HilogCapture $hilog
  }
}

function Invoke-SuiteCheck {
  param([string[]]$LogPaths)
  $argumentList = @(Join-Path $PSScriptRoot "p011_check.py")
  foreach ($logPath in $LogPaths) {
    $argumentList += @("--suite-input", $logPath)
  }
  $argumentList += @("--json", [System.IO.Path]::GetFullPath($SuiteJson), "--markdown", [System.IO.Path]::GetFullPath($SuiteMarkdown))
  python @argumentList
  if ($LASTEXITCODE -ne 0) { throw "P011 suite check failed" }
}

if ($Mode -eq "Schedule") {
  python (Join-Path $PSScriptRoot "p011_tc02_schedule.py") --frame-ms $IntervalMs --output $SchedulePath
  if ($LASTEXITCODE -ne 0) { throw "schedule generation failed" }
}
if ($Mode -eq "Schedule") { return }

# A target mutex serializes this runner's lifecycle, including pressure segments.
$mutex = [System.Threading.Mutex]::new($false, ("Local\P011-capture-" + $Target))
if (-not $mutex.WaitOne(0)) { $mutex.Dispose(); throw "Target already has an active canonical runner" }
try {
$tc02Log = Join-Path $OutputRoot "p011-tc02-formal.log"
if ($ReuseTc02Log) {
  $tc02Log = [System.IO.Path]::GetFullPath($ReuseTc02Log)
  if ((Split-Path -Parent $tc02Log) -eq $OutputRoot) { throw "Reuse requires a fresh separate OutputRoot" }
  Prepare-CanonicalCapture
  Test-FrozenTc02 $tc02Log
} elseif ($Mode -in @("Tc02", "SchedulePrecheckOnly", "All")) {
  # Both formal and precheck-only modes execute this exact lifecycle.
  Invoke-Tc02 | Out-Host
} else {
  throw "PressureSegments requires -ReuseTc02Log with a passing frozen canonical precheck"
}
if ($Mode -in @("PressureSegments", "All")) {
  Invoke-PressureSegment "full-load" 88
  Invoke-PressureSegment "clip-segment-a-positive" 88
  Invoke-PressureSegment "clip-segment-b-negative" -88
  Invoke-PressureSegment "clip-segment-c-positive-repeat" 88
}

if ($Mode -eq "All") {
  Invoke-SuiteCheck @(
    $tc02Log,
    (Join-Path $OutputRoot "p011-full-load.log"),
    (Join-Path $OutputRoot "p011-clip-segment-a-positive.log"),
    (Join-Path $OutputRoot "p011-clip-segment-b-negative.log"),
    (Join-Path $OutputRoot "p011-clip-segment-c-positive-repeat.log")
  )
}
} finally { $mutex.ReleaseMutex(); $mutex.Dispose() }

