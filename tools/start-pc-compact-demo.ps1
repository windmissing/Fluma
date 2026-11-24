param(
    [switch]$Baseline,
    [switch]$SandOnly,
    [switch]$CheckOnly
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$local = Join-Path $root '.codex/local-test'
$exe = Join-Path $local 't918-ninja/hello_world_pc.exe'
$profile = if ($Baseline) { 't830-m1-20261123' } else { 't918-shaders' }
$shaders = Join-Path $local $profile
$modules = @('full_source.spv', 'quantum_candidate.spv', 'quantum_commit.spv') |
    ForEach-Object { Join-Path $shaders $_ }
foreach ($path in @($exe) + $modules) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing T918 artifact: $path"
    }
}
$launch = @('--m1-window') + $modules + @('--m1-one-quantum-grant', '--m1-source-trace')
if ($Baseline) {
    $launch += @('--m1-empty-preview', '--m1-manual-gravity')
} else {
    $launch += '--m1-compact-demo'
}
if (-not $SandOnly) { $launch += '--m1-dual-source' }
if ($CheckOnly) {
    Get-FileHash -LiteralPath (@($exe) + $modules)
    return
}

$log = Join-Path $local ('t918-demo-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $log | Out-Null
$stdout = Join-Path $log 'demo.jsonl'
[ordered]@{ exe = $exe; profile = $profile; argv = $launch; cwd = $root } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $log 'launch.json') -Encoding utf8
$start = @{
    FilePath = $exe
    ArgumentList = @($launch | ForEach-Object { if ($_ -match '\s') { '"' + $_ + '"' } else { $_ } })
    WorkingDirectory = $root
    RedirectStandardOutput = $stdout
    RedirectStandardError = Join-Path $log 'stderr.txt'
    PassThru = $true
}
$process = Start-Process @start
$until = [DateTime]::UtcNow.AddSeconds(60)
do {
    if ((Test-Path -LiteralPath $stdout) -and
        (Select-String -LiteralPath $stdout -SimpleMatch '"stage":"M1_POINTER_READY"' -Quiet)) {
        Write-Host "Ready. Hold inside the box to pour. Left/Right turns the box. R or the top-right button resets. Logs: $log"
        return
    }
    if ($process.HasExited) { break }
    Start-Sleep -Milliseconds 250
} while ([DateTime]::UtcNow -lt $until)
Write-Warning "Input READY has not arrived. PID $($process.Id). Logs: $log"
