param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [string]$OutputRoot = 'demo\effect\P011-v28-heavy-interface\preflight'
)

$ErrorActionPreference = 'Stop'
$OutputRoot = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Invoke-Hdc {
  param([string[]]$Arguments)
  & $Hdc -t $Target @Arguments
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed' }
}

function Send-Tilt {
  param([double]$Angle)
  Invoke-Hdc @(
    'shell', 'aa', 'start', '-b', $Bundle, '-a', $Ability,
    '--ps', 'rendermode', '4', '--ps', 'testmode', 'true',
    '--ps', 'forceinvert', 'false', '--ps', 'seed', '20261018',
    '--ps', 'wspreset', 'thick', '--ps', 'wssnapshot', 'true',
    '--ps', 'wscols', '10', '--ps', 'wscapacity', '1000',
    '--ps', 'wssandtopmax', '30', '--ps', 'wstilt', $Angle
  )
}

function Capture-Frame {
  param([string]$Name, [long]$CommandTimeMs)
  $devicePath = '/data/local/tmp/p011-v28-' + $Name + '.jpeg'
  $localPath = Join-Path $OutputRoot ($Name + '.jpeg')
  Invoke-Hdc @('shell', 'snapshot_display', '-f', $devicePath) | Out-Null
  Invoke-Hdc @('file', 'recv', $devicePath, $localPath) | Out-Null
  Invoke-Hdc @('shell', 'rm', '-f', $devicePath) | Out-Null
  return @{
    segment = $Name
    image = [System.IO.Path]::GetFullPath($localPath)
    commandTimeMs = $CommandTimeMs
    captureTimeMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  }
}

$manifest = @()

Invoke-Hdc @('shell', 'aa', 'force-stop', $Bundle)
Invoke-Hdc @('shell', 'hilog', '-r')
$logPath = Join-Path $OutputRoot 'hilog.txt'
$stderrPath = $logPath + '.stderr'
$hilog = Start-Process -FilePath $Hdc -ArgumentList @('-t', $Target, 'shell', 'hilog') `
  -RedirectStandardOutput $logPath -RedirectStandardError $stderrPath `
  -WindowStyle Hidden -PassThru

try {
  Start-Sleep -Seconds 1
  $commandTimeMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  Send-Tilt 0
  Start-Sleep -Seconds 2
  $manifest += Capture-Frame 'baseline' $commandTimeMs
  $commandTimeMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  Send-Tilt 15
  Start-Sleep -Seconds 2
  $manifest += Capture-Frame 'positive' $commandTimeMs
  $commandTimeMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  Send-Tilt 0
  Start-Sleep -Seconds 1
  $manifest += Capture-Frame 'positive-return' $commandTimeMs
  $commandTimeMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  Send-Tilt -15
  Start-Sleep -Seconds 2
  $manifest += Capture-Frame 'negative' $commandTimeMs
  $commandTimeMs = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
  Send-Tilt 0
  Start-Sleep -Seconds 1
  $manifest += Capture-Frame 'final-return' $commandTimeMs
} finally {
  if (-not $hilog.HasExited) { Stop-Process -Id $hilog.Id -Force }
  $hilog.WaitForExit(3000) | Out-Null
}

$manifestPath = Join-Path $OutputRoot 'manifest.json'
$manifest | ConvertTo-Json | Set-Content -Path $manifestPath -Encoding UTF8

python (Join-Path $PSScriptRoot '..\test\p011_v28_preflight_check.py') `
  --baseline (Join-Path $OutputRoot 'baseline.jpeg') `
  --positive (Join-Path $OutputRoot 'positive.jpeg') `
  --negative (Join-Path $OutputRoot 'negative.jpeg') `
  --log $logPath `
  --manifest $manifestPath `
  --output (Join-Path $OutputRoot 'preflight.json')
if ($LASTEXITCODE -ne 0) { throw 'P-011 v2.8 preflight analysis failed' }
