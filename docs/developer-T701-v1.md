# T701 GPU 两程序与持久化四臂

每条路线分别生成 `THREE_OFF`、`TWO_OFF`、`THREE_ON`、`TWO_ON`。M1 固定实际 App 父版 `620b114c`，保留 `ledgerPrefix` 和原生资源计账。M2 从 T663 → T468 → T452 生成实际 `m2route`，保留原诊断游标及 Q 工作区额外 8192 字。两条路线使用相同的 late overlay 规则，编译时固定两个枚举。生成包通过 `fullComputeVariant()` 提供自身角色和源 SHA。M2 两程序臂也在创建游标里记录角色 1 为 `NOT_USED_BY_VARIANT`。

完整 Create 将 Q 的资源计划、分配和临时程序构建拆开。三程序臂保持先构建并反射临时 Q，退役后再构建最终 Q。两程序臂保留相同分配、角表上传和 pass 表，直接构建最终 Q。独立 Q 四臂均走原源码编译。最终 Q 对实际链接程序核对活跃 uniform 类型、T341 必需输入和四个 SSBO binding，再执行原 Prepare 和 warmup。

缓存开启时复用 T475 文件存储和原生工厂，只接完整 Create 的 compute 角色。EntryAbility 从本应用 `filesDir` 初始化存储。SOURCE 和临时 Q 在资源、反射及当前 scope 检查通过后取得导出资格。最终 Q 在 T341 warmup 和 owner fence 后提取 binary。原单 dispatch warmup 的额外 fence 只用于导出资格。合法退役与 context lost 清理对应代的工厂记录。

M1 构建副本还原了 T647/G11 已批准文档的原始字节，并在四臂共用父版修正 G12 生成脚本的相对路径。该接线仅用于生成隔离产品，G12 内核和算法不变。

```text
node tools/t701/prepare.cjs M1 .codex/local-test/<fresh-m1-matrix>
node tools/t701/prepare.cjs M2 .codex/local-test/<fresh-m2-matrix>
hvigorw --mode module -p product=default -p module=entry@default assembleHap --no-daemon
python tools/t701/compile_receipt.py <matrix> <logs> <receipt.json>
```

每个臂目录单独运行构建。日志命名 `t701-<M1|M2>-<arm>.log`。编译收据核对逐对象源与产物、CMake 枚举、实际链接 BuildId、包内 ELF/ABC 和两角色 GLSL 字节。独立测试、运行期回执、真机和收益统计由后续角色执行。

M1、M2 各四臂实际 native、ArkTS 与 unsigned HAP 构建均退出 0。最终产品和原日志保留在 `.codex/local-test/t701-final`，提交的编译回执分别为 `test/reports/t701/compile-m1.json` 和 `test/reports/t701/compile-m2.json`。M1 的 GLSL 身份按编译器实际嵌入的 LF 字节计算，回执已逐臂与包内源核对。
