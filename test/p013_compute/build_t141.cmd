@echo off
setlocal
call "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat" >nul 2>&1
set "KIT=C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0"
set "KITLIB=C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0"
set "INCLUDE=%INCLUDE%;%KIT%/ucrt;%KIT%/um;%KIT%/shared"
set "LIB=%LIB%;%KITLIB%/ucrt/x64;%KITLIB%/um/x64"
set "PATH=C:/Users/admin/AppData/Local/Programs/CokePLUS;%PATH%"
cd /d %~dp0../..
cl /std:c++17 /EHsc /nologo /fp:strict /FIclock.h /Itest/p013_native/host_compat /DEGL_NO_PLATFORM_SPECIFIC_TYPES /DKHRONOS_STATIC /I.codex/local-test/t55-gpu/sdk /Itest/p013_compute test/p013_compute/pair_driver_v1.cpp test/p013_compute/pair_prepare_v1.cpp test/p013_compute/pair_executor_v1.cpp test/p013_compute/pair_owner_v1.cpp .codex/local-test/t80-matrix/libEGL.lib .codex/local-test/t80-matrix/libGLESv2.lib /Fe.codex/local-test/t80-matrix/t141_pair.exe /Fo.codex/local-test/t80-matrix/
if errorlevel 1 exit /b 1
set "NATIVE=C:/Program Files/Huawei/DevEco Studio/sdk/default/openharmony/native"
"%NATIVE%/llvm/bin/clang++.exe" --target=aarch64-linux-ohos --sysroot="%NATIVE%/sysroot" -std=c++17 -fPIC -fno-fast-math -ffp-contract=off -shared -Itest/p013_compute test/p013_compute/pair_binding_v1.cpp test/p013_compute/pair_prepare_v1.cpp test/p013_compute/pair_executor_v1.cpp test/p013_compute/pair_owner_v1.cpp "%NATIVE%/sysroot/usr/lib/aarch64-linux-ohos/libEGL.so" "%NATIVE%/sysroot/usr/lib/aarch64-linux-ohos/libGLESv3.so" "%NATIVE%/sysroot/usr/lib/aarch64-linux-ohos/libace_napi.z.so" -o .codex/local-test/t141/libp011_gpu_pair_prototype.so
if errorlevel 1 exit /b 1
echo BUILD_OK
