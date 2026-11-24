param(
    [ValidateSet('OP48', 'Business', 'Verify')]
    [string]$Stage = 'OP48'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$exe = Join-Path $root 'hello_world_pc.exe'

function Get-Sha256([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $algorithm.Dispose()
    }
}

$manifest = Get-Content -Raw -LiteralPath (Join-Path $root 'manifest.json') | ConvertFrom-Json
if ($manifest.op48SHA256 -cnotmatch '^[0-9a-f]{64}$') {
    throw 'Invalid OP_48 SHA-256 in manifest.'
}
foreach ($file in $manifest.files.PSObject.Properties) {
    $actual = Get-Sha256 (Join-Path $root $file.Name)
    if ($actual -ine $file.Value) {
        throw "Frozen input mismatch: $($file.Name)"
    }
}
if ($Stage -eq 'Verify') {
    Write-Host 'Frozen executable and all six fixture files verified.'
    return
}

$tag = 'G13-' + $Stage + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$out = Join-Path $root $tag
New-Item -ItemType Directory -Path $out | Out-Null

function Get-LogRows([string]$Path) {
    foreach ($line in (Get-Content -Encoding UTF8 -LiteralPath $Path -ErrorAction SilentlyContinue)) {
        try { $line | ConvertFrom-Json -ErrorAction Stop } catch { }
    }
}

function Invoke-FrozenCase([string]$Name, [string[]]$Arguments) {
    $stdout = Join-Path $out "$Name.jsonl"
    $stderr = Join-Path $out "$Name.stderr.txt"
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $process = Start-Process -FilePath $exe -ArgumentList $Arguments -WorkingDirectory $root -PassThru `
        -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    # Keep the owned process handle open so Windows PowerShell can read its exit code.
    $null = $process.Handle
    $reason = 'CHILD_EXIT'
    while (-not $process.WaitForExit(750)) {
        $candidate = @(Get-LogRows $stdout | Where-Object { $_.role -eq 'OP_48' -and $_.call -eq 'vkCreateComputePipelines' } | Select-Object -Last 1)
        if ($candidate.Count -and $candidate[0].edge -eq 'ENTER' -and [Diagnostics.Stopwatch]::IsHighResolution) {
            $seconds = ([Diagnostics.Stopwatch]::GetTimestamp() - [long]$candidate[0].qpc) / [double]$candidate[0].qpcFrequency
            if ($seconds -ge 60) {
                $reason = 'OP48_CALL_UNRETURNED_IN_60S'
                break
            }
        }
        if ($clock.Elapsed.TotalSeconds -ge 120) {
            $reason = 'CHILD_UNRETURNED_IN_120S'
            break
        }
    }
    if (-not $process.HasExited) {
        try { $process.Kill() } catch [InvalidOperationException] { if (-not $process.HasExited) { throw } }
    }
    $process.WaitForExit()
    $clock.Stop()
    $rows = @(Get-LogRows $stdout)
    $boot = $rows | Where-Object { $_.devices -and $null -ne $_.selectedDevice } | Select-Object -Last 1
    $enter = $rows | Where-Object {
        $_.role -eq 'OP_48' -and $_.call -eq 'vkCreateComputePipelines' -and $_.edge -eq 'ENTER'
    } | Select-Object -Last 1
    $exitEvent = $rows | Where-Object {
        $_.role -eq 'OP_48' -and $_.call -eq 'vkCreateComputePipelines' -and $_.edge -eq 'EXIT'
    } | Select-Object -Last 1
    $final = $rows | Select-Object -Last 1
    $capture = Join-Path $out "$Name.bin"
    $captureHash = if (Test-Path -LiteralPath $capture -PathType Leaf) {
        Get-Sha256 $capture
    }
    $adapter = if ($boot) { $boot.devices[[int]$boot.selectedDevice] }
    $op48Observation = if ($exitEvent -and $exitEvent.moduleSHA -ine $manifest.op48SHA256) {
        'SHADER_HASH_MISMATCH'
    } elseif ($exitEvent -and $null -ne $exitEvent.vkResult -and [int]$exitEvent.vkResult -eq 0) {
        'VK_SUCCESS'
    } elseif ($exitEvent) {
        'VK_ERROR_OR_UNRECORDED_RESULT'
    } elseif ($enter -and $reason -ne 'CHILD_EXIT') {
        'NO_RETURN_IN_OBSERVATION_WINDOW'
    } elseif ($enter) {
        'PROCESS_EXITED_WITHOUT_PIPELINE_RETURN'
    } else {
        'OP48_NOT_OBSERVED'
    }
    $record = [ordered]@{
        test = $Name; argv = $Arguments; exitCode = $process.ExitCode; reason = $reason
        observedSeconds = $clock.Elapsed.TotalSeconds; selectedAdapter = $adapter
        op48 = $op48Observation; op48Enter = $enter; op48Exit = $exitEvent
        final = $final; captureSHA256 = $captureHash
    }
    $record | ConvertTo-Json -Depth 24 | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $out "$Name.summary.json")
    $process.Dispose()
    [pscustomobject]$record
}

if ($Stage -eq 'OP48') {
    $result = Invoke-FrozenCase 'OP48' @('--qualify-steady-role', 'OP_48')
    $summary = switch ($result.op48) {
        'VK_SUCCESS' { 'OK' }
        'NO_RETURN_IN_OBSERVATION_WINDOW' {
            if ($result.reason -eq 'OP48_CALL_UNRETURNED_IN_60S') { 'TIMEOUT_60S' } else { 'TIMEOUT_120S' }
        }
        'VK_ERROR_OR_UNRECORDED_RESULT' { "VK_ERROR($($result.op48Exit.vkResult))" }
        default {
            $where = if ($result.final.stage) { $result.final.stage } else { $result.reason }
            "$($result.op48)($where,exit=$($result.exitCode))"
        }
    }
    $gpu = if ($result.selectedAdapter) { $result.selectedAdapter.deviceName } else { 'unknown' }
    $driver = if ($null -ne $result.selectedAdapter.driverVersion) { $result.selectedAdapter.driverVersion } else { 'unknown' }
    Write-Host "REPLY T916 | GPU=$gpu | driver=$driver | OP48=$summary"
} else {
    $results = foreach ($case in @('F02', 'F03', 'F05')) {
        $argsForCase = @('--g13-candidate', "$tag/$case.bin", '--g13-fixture', $case,
            "fixtures/$case-config128.bin", "fixtures/$case-state.bin", '--g13-diagnostics', 'hot')
        if ($case -eq 'F03') { $argsForCase += @('--g13-trial', '1') }
        Invoke-FrozenCase $case $argsForCase
    }
    $gpu = ($results | ForEach-Object { $_.selectedAdapter.deviceName } | Where-Object { $_ } | Select-Object -Unique) -join '/'
    $cases = foreach ($result in $results) {
        $state = if ($result.reason -eq 'CHILD_EXIT' -and $result.final.state) { $result.final.state } else { $result.reason }
        $where = if ($result.final.stage) { "@$($result.final.stage)" } else { '' }
        "$($result.test)=$state$where"
    }
    Write-Host "REPLY T916 | GPU=$gpu | $($cases -join ' | ')"
}
