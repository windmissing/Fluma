param([switch]$Verify, [switch]$ProbeOnly)
$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot 'G13-FP32-Probe.exe'
$manifest = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') | ConvertFrom-Json
$sha = [Security.Cryptography.SHA256]::Create()
$file = [IO.File]::OpenRead($exe)
try {
    $digest = [BitConverter]::ToString($sha.ComputeHash($file)).Replace('-', '').ToLowerInvariant()
} finally {
    $file.Dispose()
    $sha.Dispose()
}
if ($digest -cne $manifest.exeSHA256) { throw 'Test executable hash mismatch.' }
if ($Verify) {
    Write-Host "Verified $($manifest.version) executable."
    return
}

$folder = Join-Path $PSScriptRoot ('G13-v2-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
[IO.Directory]::CreateDirectory($folder) | Out-Null
$stdout = Join-Path $folder 'stdout.txt'
$stderr = Join-Path $folder 'stderr.txt'
$launch = @{
    FilePath = $exe; WorkingDirectory = $PSScriptRoot; PassThru = $true; WindowStyle = 'Hidden'
    RedirectStandardOutput = $stdout; RedirectStandardError = $stderr
}
if ($ProbeOnly) { $launch.ArgumentList = '--probe-only' }

function Read-Result {
    $fields = @{}
    foreach ($line in (Get-Content -Encoding UTF8 -LiteralPath $stdout -ErrorAction SilentlyContinue)) {
        if ($line -match '^([A-Z0-9]+)=(.*)$') { $fields[$Matches[1]] = $Matches[2].Trim() }
    }
    $fields
}

try {
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $process = Start-Process @launch
    $null = $process.Handle
} catch {
    Write-Host 'REPLY G13-FP32-v2 | GPU=unknown | probe=NOT_RUN | OP48=START_FAILED'
    throw
}
$op48Start = $null
$timeout = ''
try {
    while (-not $process.WaitForExit(350)) {
        $fields = Read-Result
        if ($fields.STAGE -eq 'OP48_CREATE' -and $null -eq $op48Start) {
            $op48Start = $clock.Elapsed.TotalSeconds
        }
        if ($null -ne $op48Start -and -not $fields.OP48 -and $clock.Elapsed.TotalSeconds - $op48Start -ge 60) {
            $timeout = 'TIMEOUT_60S'
            break
        }
        if ($clock.Elapsed.TotalSeconds -ge 120) {
            $timeout = 'TIMEOUT_120S'
            break
        }
    }
    if (-not $process.HasExited) {
        try { $process.Kill() } catch [InvalidOperationException] { if (-not $process.HasExited) { throw } }
    }
    $process.WaitForExit()
    $fields = Read-Result
    $gpu = if ($fields.GPU) { $fields.GPU } else { 'unknown' }
    $driver = if ($fields.DRIVER) { $fields.DRIVER } else { 'unknown' }
    $problem = if ($fields.ERROR) { $fields.ERROR } elseif ($timeout) { $timeout } else { "EXIT_$($process.ExitCode)" }
    $probe = if ($fields.PROBE) { $fields.PROBE } else { $problem }
    $op48 = if ($fields.OP48) {
        if ($fields.MS) { "$($fields.OP48)@$($fields.MS)ms" } else { $fields.OP48 }
    } elseif ($fields.PROBE -eq 'PASS' -and -not $ProbeOnly) { $problem } else { 'NOT_RUN' }
    $receipt = "REPLY G13-FP32-v2 | GPU=$gpu | driver=$driver | probe=$probe | OP48=$op48"
    Set-Content -Encoding UTF8 -LiteralPath (Join-Path $folder 'reply.txt') -Value $receipt
    Write-Host $receipt
} finally {
    $process.Dispose()
}
