param(
  [string]$Hdc = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\toolchains\hdc.exe',
  [string]$Target = '62T0225B10010020',
  [string]$Bundle = 'com.example.helloworld',
  [string]$Ability = 'EntryAbility',
  [int]$Seed = 20261016,
  [string]$Hap = 'D:\hmprojects\hw-p012-c-verify\entry\build\default\outputs\default\entry-default-s107-signed.hap',
  [string]$ExpectedHapSha256 = 'c1514721e85c5d9194f49d4ef572c7366ece185ddff9b43560f1287e1e0ea2a3',
  [string]$OutputRoot = 'D:\hmprojects\HelloWorld\test\artifacts\p012-c-verify-20261021'
)

$ErrorActionPreference = 'Stop'
Remove-Item Env:HTTP_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:HTTPS_PROXY -ErrorAction SilentlyContinue
Remove-Item Env:http_proxy -ErrorAction SilentlyContinue
Remove-Item Env:https_proxy -ErrorAction SilentlyContinue

$hapSha = (Get-FileHash -LiteralPath $Hap -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hapSha -ne $ExpectedHapSha256) { throw "HAP SHA256 mismatch: $hapSha" }

function Invoke-Hdc {
  param([string[]]$Arguments)
  $output = & $Hdc -t $Target @Arguments 2>&1
  if ($LASTEXITCODE -ne 0) { throw 'hdc failed: ' + ($Arguments -join ' ') }
  return $output
}

function Capture-Frame {
  param([string]$Name)
  $devicePath = '/data/local/tmp/p012-c-' + $Name + '.jpeg'
  $localPath = Join-Path $RunDir ($Name + '.jpeg')
  Invoke-Hdc @('shell','snapshot_display','-f',$devicePath) | Out-Null
  Invoke-Hdc @('file','recv',$devicePath,$localPath) | Out-Null
  Invoke-Hdc @('shell','rm','-f',$devicePath) | Out-Null
  Write-Output ("  captured " + $Name)
}

Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Invoke-Hdc @('install','-r',$Hap) | Out-Null
Invoke-Hdc @('shell','power-shell','wakeup') | Out-Null
Invoke-Hdc @('shell','power-shell','timeout','-o','600000') | Out-Null

# 3 paired blocks x (OFF strat-ablated vs ON default); each: 25s static -> slow tilt -> return
for ($pair = 1; $pair -le 3; $pair++) {
  foreach ($mode in @('off','on')) {
    $tag = "pair$pair-$mode"
    $RunDir = Join-Path $OutputRoot $tag
    New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
    Write-Output "=== $tag ==="
    Invoke-Hdc @('shell','aa','force-stop',$Bundle)
    Start-Sleep -Seconds 1
    # OFF: p012ablate=strat (C layer disabled); ON: no ablate param (all enabled)
    # p012tier=3 (EAB) is REQUIRED: P012 renderer is gated by p012tier, not rendermode.
    # v1 batch without p012tier was invalid (both arms rendered canonical path).
    if ($mode -eq 'off') {
      Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
        '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
        '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','false',
        '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
        '--ps','p012tier','3','--ps','p012ablate','strat') | Out-Null
    } else {
      Invoke-Hdc @('shell','aa','start','-b',$Bundle,'-a',$Ability,
        '--ps','rendermode','4','--ps','testmode','true','--ps','forceinvert','false',
        '--ps','seed',([string]$Seed),'--ps','wspreset','thick','--ps','wssnapshot','false',
        '--ps','wscols','128','--ps','wscapacity','192','--ps','wssandtopmax','28',
        '--ps','p012tier','3') | Out-Null
    }
    Start-Sleep -Seconds 25   # static settle
    Capture-Frame "$tag-static-0"
    Start-Sleep -Milliseconds 500
    Capture-Frame "$tag-static-1"
    Start-Sleep -Milliseconds 500
    Capture-Frame "$tag-static-2"
    # slow tilt (full-width swipe, two segments to ~20 deg)
    Invoke-Hdc @('shell','uinput','-T','-m','100','1500','1200','1250','600') | Out-Null
    Start-Sleep -Seconds 4
    Capture-Frame "$tag-tilt-0"
    Start-Sleep -Seconds 3
    Invoke-Hdc @('shell','uinput','-T','-m','100','1500','1200','1100','600') | Out-Null
    Start-Sleep -Seconds 4
    Capture-Frame "$tag-tilt-1"
    Start-Sleep -Seconds 3
    # return to level
    Invoke-Hdc @('shell','uinput','-T','-m','1200','1100','100','1500','800') | Out-Null
    Start-Sleep -Seconds 5
    Capture-Frame "$tag-return-0"
    Start-Sleep -Seconds 3
    Capture-Frame "$tag-return-1"
  }
}
Invoke-Hdc @('shell','aa','force-stop',$Bundle)
Write-Output "ALL PAIRS DONE"
exit 0
