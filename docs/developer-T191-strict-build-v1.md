# T191：隔离 STRICT_O0 / STRICT_O2 构建交付

日期：2026-10-28。原卡 `6ae1451be4b0d25fef541f37`。仅实施 `architecture-T183-gpu-cost-next-v1.md` §6 H1 / §7B。不使用真机，不包含 H2 仪表、算法优化或设备收益结论。

## 代码与身份

新增 `test/p013_compute/tools/generate_transaction_project_v4.cjs`，先调用原 V3/V2 生成器，再施加 10 项可逆白名单变更（5 个已有文件）并增加隔离 CMake 模块。原 V2/V3 入口和生产 entry 未修改。只允许新建 `.codex/local-test` 内目录，显式指定 `STRICT_O0` / `STRICT_O2`；拒绝覆盖旧生成目录，避免混入旧对象/包。

`strict_variant_v1.cmake` 最后加入且仅作用于 `p011_gpu_pair_napi`：分别追加 `-O0` / `-O2`，共同保留 `-fno-fast-math -ffp-contract=off`。实际四个编译单元均验证最终有效优化选项。全工程保持 Debug，其余 target 不变；不启用 fast-math、Ofast、LTO、FMA contraction 或 NDEBUG，不改变注错宏。变体与生成器/配置编辑纳入 `T191_SOURCE_V4` 的非自指 SHA，保留嵌套 V3/V2 source manifest。

生成的 BuildIdentity 绑定 variant，配置身份改为 `T191_STRICT_CONFIG_V1|variant:STRICT_Ox|...`；启动器生成 `config.buildVariant`，协议验证其与内建值一致，micro 配置保留这一身份。COMMAND/ATTEMPT 原有 config 输出自然携带 variant。没有修改计时边界/槽位或 H2 仪表。

原 native 算法、wire、ABI 源码、shader、四 fixture、每 mutation freeze/SHA/sort/range、权威消费、物理/guard/audit、oracle 正确性臂及事件/退款/lineage 证据逻辑字节保持。可逆检查对 Hot/M2/WaterSandField/Worker/Evidence/Native/Selector 逐文件与同输入 V3 基线比较，没有将整个 source/snapshot 证据删减。

## 真实构建与离线自测

- 开发检查 `node test/p013_compute/run_t191_strict_build_units.cjs`：**12/12 PASS**。含 manifest SHA 复算、白名单逐字节逆向、native/物理/仪表未改、两档实际 flags 与宏、obj/包 SO/build-id 关联、7 项 raw resource 相同、variant 配置拒绝及再次生成得到相同 source 身份。
- `python test/p013_compute/run_t191_cmake_guards.py`：**6/6 PASS**。真实 CMake 配置拒绝 variant 冲突、Release、LTO、NDEBUG、fast-math、contraction=fast；该负例 harness 无编译器，不替代真实 ARM 编译证据。
- `test/p013_compute/build_t191_host.cmd`：原 T159/T171 绑定断言分别以 MSVC `/Od /fp:strict`、`/O2 /fp:strict` 通过；NAPI/executor 是 host doubles，**不是 ARM 或 GPU 执行证据**。
- 两档真实 OHOS arm64 native 编译、ArkTS、unsigned PackageHap 均通过。一键脚本在另一组全新目录完整重建，两档 `compilePackageExitCode=0 / BUILD SUCCESSFUL`，source 身份分别完全一致。
- 首次 `assembleHap` 的代码编译和打包成功，SignHap 被既有 `00303116`（签名密码长度不足32）拒绝；后续一键脚本明确调用 `default@PackageHap`，退出码为0。没有修改签名材料。**交付两个 unsigned 包，signed=NOT_BUILT**。

工具链为 OHOS clang 15.0.4，target `aarch64-linux-ohos`，SDK 26.0.0，Debug；真实 CMake/Ninja 版本、toolchain 文件 SHA、compiler SHA、四单元全部编译参数、宏 dump、源文件/对象 SHA、真实 Ninja link 命令均在各 post-build manifest/归档中。宏差异只允许 source literal 及编译器优化内建宏；NDEBUG 和注错定义保持一致。

## 双包与产物链

正式交付见 `test/reports/t191/artifacts-o0.zip` / `artifacts-o2.zip`，各含 unsigned HAP、四个对象文件、未剥离 object SO、包内 SO、宏、Ninja 命令及两层 manifest。`delivery-manifest.json` 记录 ZIP 自身 SHA 和逐成员 size/SHA。

| 身份 | STRICT_O0 | STRICT_O2 |
| --- | --- | --- |
| sourceManifestSha256 | `6ed2d9eaeb37853d7ddb64d7999a586078e424916ab15a80c822065a9137d15d` | `e48340d397bf407e69ce4d8ffb97e3d9b224e571c7a38a7e2fcdd36b74631cde` |
| unsigned HAP SHA256 | `df833640d89ad77b231be25268b36c8715e792b19d779d133e5cf80ad915e253` | `7a574b52e4da7363ee0e31bb4dddfd5cb428bf2c97fb5dd5d92eab13d63f69a3` |
| 包内 SO SHA256 | `d144dfe8a8cb7ecf90150d4f09cd149a4391db227f9f82d58112b1d2b839da9b` | `ac8be9769f01c9e9834a516497debce8944d4b68e4581ca987e717ccd086e908` |
| ELF build-id | `a1d2adbf719b9845c9e12a65625516f9b0419d89` | `26bdb6a4467330364b59a3aeabc758fe06c7875d` |

各档 object SO 与包内 SO 的整体 SHA 不同，但 `.text/.rodata/.data/.dynsym/.note.gnu.build-id` 逐段 size/SHA 及 build-id 相同，按实际剥离链区分登记。不得把 object SO SHA 填成安装包内 SO SHA。launch 的 `t171buildManifestSha256` 仍须传**实际使用的 unsigned HAP SHA**，签名之后单独登记 signed SHA；不能把 ZIP、source 或 post-build JSON 文件 SHA 当成这一参数。

重建方法（项目根目录，使用全新 name-prefix）：

```powershell
python test/p013_compute/tools/build_t191_strict_variants.py --name-prefix t191-new-run --reports test/reports/t191-new-run
```

生成目录与本地依赖缓存按原 V3 方式隔离。实际第二轮记录在 `reproduction/build-summary.json` 及两份 package 日志。生成 source 身份可重复；不同输出路径/打包时刻的产物 SHA 分别归档，**不声明跨目录 HAP 逐字节确定性**，不覆盖旧 O0、旧 raw 或旧 run 身份。正式 ZIP 对应上表首次构建，第二轮另列真实 SHA。

## 边界与接力

本卡交付构建实验，不授权生产迁移。设备运行、GPU timer、性能收益、热状态/UI 竞争均 **NOT_RUN / NOT_MEASURED**。后续独立 tester 按 T183 §7C 验证优化等价与边界；设备 tester 按 §7D 使用具体包/source/variant/flags/进程/run 身份测试。两项不作为本开发卡的新增关闭门。
