# Owned native build policy (T666)

Build-only tools. They do not run or load a product, install SDK patches, delete caches, sign, or use devices.

Create an explicit JSON policy; all three user baseline strings are required (empty is an explicit choice). `definitions` is an ordered list of separate CMake `-D...` argv items. Compiler/tool paths and the SDK toolchain must be explicit. Example:

```json
{"schema":"GRS_EXPLICIT_LINK_BASELINE_V1","source":"D:/hmprojects/HelloWorld/tools/build/broker_compile","cmake":"C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/build-tools/cmake/bin/cmake.exe","ninja":"C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/build-tools/cmake/bin/ninja.exe","sdkToolchain":"C:/Users/admin/AppData/Local/OpenHarmony/Sdk/26.0.0/native/build/cmake/ohos.toolchain.cmake","baseline":{"SHARED":"-Wl,--as-needed -Wl,--no-as-needed","MODULE":"","EXE":""},"definitions":["-DOHOS_ARCH=arm64-v8a","-DCMAKE_BUILD_TYPE=Debug","-DCMAKE_CXX_FLAGS=-fno-fast-math -ffp-contract=off"]}
```

```text
python tools/build/owned_build.py configure <policy.json> <fresh-build>
python tools/build/owned_build.py prepare-link <policy.json> <build> grs_cold_slots
<explicit-ninja.exe> -C <build> grsT666Consumer
python tools/build/build_receipt.py <build> <receipt-directory> --static-artifact-only
```

For the actual App source tree, `source` can name its native CMake directory and Ninja can build the actual App target after the same target-limited patch. `broker_compile` provides a compile/link-only consumer of all eight real repository broker ABI symbols; it is never loaded.

The first configure requires a fresh directory. A later configure requires the same policy identity and flags equal to either its declared baseline or the previous known SDK result. Unknown flags or changed policy require a new isolated output, not guessed removal. Before each first SDK evaluation in a configure, SHARED/MODULE/EXE caches are restored from the declared original baselines. Re-entry respects the SDK's existing evaluated guard. The installed SDK remains read-only. Target options and ordered libraries/groups are not deduplicated.

`prepare-link` uses Ninja's own `compdb -x` expansion and Windows CommandLineToArgvW, not POSIX shlex. It accepts only the observed no-op `cmd /C "cd . && clang ... && cd ."` envelope; other pre/post actions or unknown targets fail explicitly. Only `grs_cold_slots` is patched. A short launcher command replaces its link rule; original output, explicit/implicit/order-only inputs remain, with manifest/rsp/launcher dependencies added. No other target rule is modified.

The complete driver flags/objects/search paths/libraries remain in order in UTF8-no-BOM LF rsp. Known path operands are canonicalized separately from rsp quoting. The launcher verifies compiler/rsp identity, records `clang -### @rsp` diagnostics, then links with `shell=False` and one `@absolute-path` argument. It does not expand the full command back into cmd. Stable manifests are not rewritten, preserving incremental no-work behavior. Configure/prepare/build must be serialized for one build directory; independent sessions use distinct directories.

Receipts contain original and effective argv, original command, cwd, flag provenance, toolchain/compiler/linker hashes, complete rsp SHA, source/object hashes, ELF/SONAME/BuildId and consumer DT_NEEDED. Historical polluted caches stay intact. No change to FP/physical gates; relaxing fast-math/contract definitions is rejected.

For a receipt that attributes results to the current invocation, start with a
fresh directory. The recorder runs the broker consumer build and prints the
sealed manifest path. Use `--incremental` to also observe configure replay,
Ninja no-work, a response-file-only relink, and a second no-work invocation.

```text
python tools/build/build_invocation.py <policy.json> <fresh-build> --incremental
python tools/build/build_receipt.py <build> <receipt-directory> --invocation-manifest <printed-manifest>
```

Keep the build directory and invocation history together. The receipt checks
the recorded child commands, original streams, and current artifact closure.
Static mode inspects artifacts without claiming compile or incremental results.
