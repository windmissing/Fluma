# Local ANGLE only; does not install or use a device.
param([string]$AngleDirectory='C:/Users/admin/AppData/Local/Programs/CokePLUS')
$ErrorActionPreference='Stop'
Set-Location -LiteralPath (Split-Path $PSScriptRoot -Parent)
if(!(Test-Path -LiteralPath "$AngleDirectory/libEGL.dll")){throw 'ANGLE library directory missing'}
python test/p056_gpu_build.py
if($LASTEXITCODE -ne 0){throw 'shader assembly failed'}
node test/run_t56_scene_units.cjs --export
if($LASTEXITCODE -ne 0){throw 'numeric/scene preparation failed'}
$env:Path="$AngleDirectory;$env:Path"
cmd /c test\run_t56_scene_gpu.cmd
if($LASTEXITCODE -ne 0){throw 'scene GPU selfcheck failed; do not reuse old results'}
