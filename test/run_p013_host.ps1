# P-013 developer selfcheck, no device connection or application installation.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $root
$sdk = 'C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native'
$out = Join-Path $root '.codex/local-test/p013-host'
New-Item -ItemType Directory -Force "$out/sdk" | Out-Null
# Only vendor API headers are copied: never inject the OHOS C library headers into the Windows compiler.
foreach ($directory in @('EGL', 'KHR', 'GLES3', 'GLES2', 'napi', 'info')) {
  Copy-Item -LiteralPath "$sdk/sysroot/usr/include/$directory" -Destination "$out/sdk" -Recurse -Force
}
foreach ($file in @('node_api.h', 'node_api_types.h', 'js_native_api.h', 'js_native_api_types.h')) {
  Copy-Item -LiteralPath "$sdk/sysroot/usr/include/$file" -Destination "$out/sdk" -Force
}
Set-Content -Encoding utf8 -LiteralPath "$out/sdk/p013_shader_sources.h" -Value '#pragma once'
Add-Content -Encoding utf8 -LiteralPath "$out/sdk/p013_shader_sources.h" -Value 'const char* kP013Vertex="host stub";'
Add-Content -Encoding utf8 -LiteralPath "$out/sdk/p013_shader_sources.h" -Value 'const char* kP013Fragment="host stub";'
Add-Content -Encoding utf8 -LiteralPath "$out/sdk/p013_shader_sources.h" -Value 'const char* kP013ShaderHash="HOST_BOUNDARY_DOUBLE_NOT_GPU";'
& cmd.exe /c test\run_p013_host.cmd
if ($LASTEXITCODE -ne 0) { throw "P013 host selfcheck failed: $LASTEXITCODE" }
