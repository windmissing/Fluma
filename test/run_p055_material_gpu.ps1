# T55 developer selfcheck: local ANGLE only; no device/install/page.
param(
  [string]$AngleDirectory='C:/Users/admin/AppData/Local/Programs/CokePLUS',
  [ValidateSet('srgb-fbo','default-pbuffer')][string]$Target='srgb-fbo'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $root
if (!(Test-Path -LiteralPath "$AngleDirectory/libEGL.dll") -or !(Test-Path -LiteralPath "$AngleDirectory/libGLESv2.dll")) {throw 'Provide an installed ANGLE library directory; no automatic install'}
$out=Join-Path $root '.codex/local-test/t55-gpu'
$fixtures=Join-Path $root '.codex/local-test/t55-fixtures'
New-Item -ItemType Directory -Force "$out/sdk" | Out-Null
$sdk='C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/sysroot/usr/include'
foreach($directory in @('EGL','KHR','GLES3','GLES2')){Copy-Item -LiteralPath "$sdk/$directory" -Destination "$out/sdk" -Recurse -Force}
python test/p055_gpu_build.py
if($LASTEXITCODE -ne 0){throw 'shader assembly failed'}
node test/run_t48_canvas_asset_units.cjs --export-dir .codex/local-test/t55-assets
if($LASTEXITCODE -ne 0){throw 'T48 byte export failed'}
node test/p055_material_fixtures.cjs prepare
if($LASTEXITCODE -ne 0){throw 'fixture build failed'}
$batch=@'
@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
lib /nologo /machine:x64 /def:.codex/local-test/t55-gpu/libEGL.def /out:.codex/local-test/t55-gpu/libEGL.lib
if errorlevel 1 exit /b 1
lib /nologo /machine:x64 /def:.codex/local-test/t55-gpu/libGLESv2.def /out:.codex/local-test/t55-gpu/libGLESv2.lib
if errorlevel 1 exit /b 1
cl /std:c++17 /EHsc /nologo /I. /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /FItest/p013_native/host_compat/clock.h /I.codex/local-test/t55-gpu/sdk /Ientry/src/main/cpp/p013renderer /I.codex/local-test/t55-gpu test/p013_native/material_gpu_test.cpp entry/src/main/cpp/p013renderer/material_pass.cpp entry/src/main/cpp/p013renderer/material_uploader.cpp .codex/local-test/t55-gpu/libEGL.lib .codex/local-test/t55-gpu/libGLESv2.lib /Fe.codex/local-test/t55-gpu/material_gpu.exe /Fo.codex/local-test/t55-gpu/
if errorlevel 1 exit /b 1
.codex\local-test\t55-gpu\material_gpu.exe .codex/local-test/t55-fixtures %1
'@
Set-Content -LiteralPath "$out/run-selfcheck.cmd" -Encoding ascii -Value $batch
$env:Path="$AngleDirectory;$env:Path"
& cmd.exe /c "$out/run-selfcheck.cmd" $Target
if($LASTEXITCODE -ne 0){throw 'GPU execution failed; do not reuse previous readbacks'}
node test/p055_material_fixtures.cjs check $fixtures $Target
if($LASTEXITCODE -ne 0){throw 'structural readback checks failed'}
