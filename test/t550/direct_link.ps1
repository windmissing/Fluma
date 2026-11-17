# T550 direct-link runner: invokes the deduped clang++ link command for
# grsm0/grsm1 outside the cmd.exe 8191-char wrapper (OHOS toolchain
# flag-duplication workaround; zero semantic change to link inputs).
$ErrorActionPreference = 'Stop'
Set-Location 'D:\hmprojects\HelloWorld\entry\.cxx\default\default\debug\arm64-v8a'
$clang = 'C:\Users\admin\AppData\Local\OpenHarmony\Sdk\26.0.0\native\llvm\bin\clang++.exe'
$fixed = @('--target=aarch64-linux-ohos','--gcc-toolchain=C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/llvm','--sysroot=C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/sysroot','-fPIC','-fdata-sections','-ffunction-sections','-funwind-tables','-fstack-protector-strong','-no-canonical-prefixes','-fno-addrsig','-Wa,--noexecstack','-Wformat','-Werror=format-security','-D__MUSL__','-O0','-g','-fno-limit-debug-info')
$block = @('--rtlib=compiler-rt','-fuse-ld=lld','-Wl,--build-id=sha1','-Wl,--warn-shared-textrel','-Wl,--fatal-warnings','-lunwind','-Wl,--no-undefined','-Qunused-arguments','-Wl,-z,noexecstack')
$out = 'D:\hmprojects\HelloWorld\entry\build\default\intermediates\cmake\default\obj\arm64-v8a'
foreach ($pair in @(@('libgrsm0.so','grsm0.rsp'), @('libgrsm1.so','grsm1.rsp'))) {
  $so = $pair[0]; $rsp = $pair[1]
  $objs = (Get-Content "CMakeFiles\$rsp" -Raw).Trim().Split(' ') | Where-Object { $_ -ne '' }
  $args = $fixed + $block + @('-shared',"-Wl,-soname,$so","-o","$out\$so") + $objs
  & $clang @args
  Write-Host "$so link exit=$LASTEXITCODE"
  if ($LASTEXITCODE -ne 0) { exit 1 }
}
